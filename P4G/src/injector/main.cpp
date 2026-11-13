// P4GTrainer.exe — loads the trainer into a running Persona 4 Golden.
//
//   P4GTrainer.exe              inject into the running game
//   P4GTrainer.exe --wait       wait for the game to start, then inject
//   P4GTrainer.exe --reload     unload the loaded build and inject this one (hot swap)
//   P4GTrainer.exe --eject      unload the trainer
//   P4GTrainer.exe --dll <file> use a specific trainer DLL (default: P4GTrainer.dll beside this exe)
//
// The DLL is copied to %LOCALAPPDATA%\P4GTrainer\bin\P4GTrainer-<n>.dll before loading, so the
// original can be rebuilt while the game holds the copy. Copies rotate through a few names and
// are overwritten, never deleted.

#include <windows.h>
#include <shlobj.h>
#include <tlhelp32.h>

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#if __has_include("BuildId.h")
#include "BuildId.h"
#endif
#ifndef P4G_VERSION
#define P4G_VERSION "0.0.0"
#endif

namespace {

constexpr int kSlots = 6;

void Say(const wchar_t* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfwprintf(stdout, fmt, ap);
    va_end(ap);
    fwprintf(stdout, L"\n");
    fflush(stdout);
}

DWORD FindGame() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W e{sizeof e};
    DWORD pid = 0;
    for (BOOL ok = Process32FirstW(snap, &e); ok; ok = Process32NextW(snap, &e))
        if (_wcsicmp(e.szExeFile, L"P4G.exe") == 0) {
            pid = e.th32ProcessID;
            break;
        }
    CloseHandle(snap);
    return pid;
}

struct Mod {
    std::wstring name, path;
    uintptr_t base;
};

std::vector<Mod> TrainerModules(DWORD pid) {
    std::vector<Mod> out;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return out;
    MODULEENTRY32W m{sizeof m};
    for (BOOL ok = Module32FirstW(snap, &m); ok; ok = Module32NextW(snap, &m))
        if (_wcsnicmp(m.szModule, L"P4GTrainer", 10) == 0)
            out.push_back({m.szModule, m.szExePath, reinterpret_cast<uintptr_t>(m.modBaseAddr)});
    CloseHandle(snap);
    return out;
}

std::wstring DataDir(const wchar_t* sub) {
    PWSTR p = nullptr;
    std::wstring d;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &p))) d = p;
    CoTaskMemFree(p);
    d += L"\\P4GTrainer";
    CreateDirectoryW(d.c_str(), nullptr);
    d += L"\\";
    d += sub;
    CreateDirectoryW(d.c_str(), nullptr);
    return d;
}

// RVA of an exported function, read from the DLL file on disk.
DWORD ExportRva(const std::wstring& file, const char* name) {
    HANDLE f = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (f == INVALID_HANDLE_VALUE) return 0;
    DWORD size = GetFileSize(f, nullptr), got = 0;
    std::vector<uint8_t> img(size);
    ReadFile(f, img.data(), size, &got, nullptr);
    CloseHandle(f);
    if (got < sizeof(IMAGE_DOS_HEADER)) return 0;
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(img.data());
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(img.data() + dos->e_lfanew);
    auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    auto* sec = IMAGE_FIRST_SECTION(nt);
    auto toOff = [&](DWORD rva) -> DWORD {
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i)
            if (rva >= sec[i].VirtualAddress && rva < sec[i].VirtualAddress + sec[i].Misc.VirtualSize)
                return rva - sec[i].VirtualAddress + sec[i].PointerToRawData;
        return 0;
    };
    if (!dir.VirtualAddress) return 0;
    auto* ex = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(img.data() + toOff(dir.VirtualAddress));
    auto* names = reinterpret_cast<DWORD*>(img.data() + toOff(ex->AddressOfNames));
    auto* ords = reinterpret_cast<WORD*>(img.data() + toOff(ex->AddressOfNameOrdinals));
    auto* funcs = reinterpret_cast<DWORD*>(img.data() + toOff(ex->AddressOfFunctions));
    for (DWORD i = 0; i < ex->NumberOfNames; ++i)
        if (strcmp(reinterpret_cast<char*>(img.data() + toOff(names[i])), name) == 0)
            return funcs[ords[i]];
    return 0;
}

bool RunRemote(HANDLE proc, uintptr_t fn, void* arg, DWORD timeout, DWORD* exitCode) {
    HANDLE t = CreateRemoteThread(proc, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(fn), arg,
                                  0, nullptr);
    if (!t) return false;
    bool done = WaitForSingleObject(t, timeout) == WAIT_OBJECT_0;
    if (exitCode) GetExitCodeThread(t, exitCode);
    CloseHandle(t);
    return done;
}

bool Eject(DWORD pid, HANDLE proc) {
    auto mods = TrainerModules(pid);
    if (mods.empty()) {
        Say(L"The trainer is not loaded.");
        return true;
    }
    for (const auto& m : mods) {
        DWORD rva = ExportRva(m.path, "TrainerUnload");
        if (!rva) {
            Say(L"%ls has no TrainerUnload export; cannot unload it safely.", m.name.c_str());
            return false;
        }
        Say(L"Unloading %ls ...", m.name.c_str());
        if (!RunRemote(proc, m.base + rva, nullptr, 5000, nullptr)) {
            Say(L"  unload request failed (%lu)", GetLastError());
            return false;
        }
    }
    // The trainer frees itself only once none of its hooks is running (it waits up to ~2 min).
    for (int i = 0; i < 1300; ++i) {
        if (TrainerModules(pid).empty()) {
            Say(L"Unloaded.");
            return true;
        }
        if (i == 30) Say(L"  waiting for the game to finish what it's doing ...");
        Sleep(100);
    }
    Say(L"The trainer did not unload. See the log in %%LOCALAPPDATA%%\\P4GTrainer\\logs.");
    return false;
}

bool Inject(DWORD pid, HANDLE proc, const std::wstring& dll) {
    if (!TrainerModules(pid).empty()) {
        Say(L"The trainer is already loaded. Use --reload to swap in this build.");
        return false;
    }
    if (GetFileAttributesW(dll.c_str()) == INVALID_FILE_ATTRIBUTES) {
        Say(L"Trainer DLL not found: %ls", dll.c_str());
        return false;
    }
    std::wstring bin = DataDir(L"bin"), target;
    for (int i = 0; i < kSlots && target.empty(); ++i) {
        wchar_t p[MAX_PATH];
        swprintf_s(p, L"%ls\\P4GTrainer-%d.dll", bin.c_str(), i);
        if (CopyFileW(dll.c_str(), p, FALSE)) target = p;  // fails only while a copy is in use
    }
    if (target.empty()) {
        Say(L"Could not stage the DLL (all %d copies are in use?).", kSlots);
        return false;
    }

    size_t bytes = (target.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote || !WriteProcessMemory(proc, remote, target.c_str(), bytes, nullptr)) {
        Say(L"Could not write into the game process (%lu).", GetLastError());
        return false;
    }
    auto loadLib = reinterpret_cast<uintptr_t>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
    DWORD code = 0;
    bool ran = RunRemote(proc, loadLib, remote, 15000, &code);
    VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
    if (!ran || code == 0) {
        Say(L"LoadLibrary failed inside the game. See %%LOCALAPPDATA%%\\P4GTrainer\\logs.");
        return false;
    }
    Say(L"Trainer loaded. Press Insert (or L3 + R3 on a controller) in game, unless you changed them in Settings.");
    return true;
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    Say(L"P4G Trainer v%hs", P4G_VERSION);
    bool wait = false, reload = false, eject = false;
    std::wstring dll;
    for (int i = 1; i < argc; ++i) {
        std::wstring a = argv[i];
        if (a == L"--wait") wait = true;
        else if (a == L"--reload") reload = true;
        else if (a == L"--eject") eject = true;
        else if (a == L"--dll" && i + 1 < argc) dll = argv[++i];
        else {
            Say(L"usage: P4GTrainer.exe [--wait | --reload | --eject] [--dll <file>]");
            return 2;
        }
    }
    if (dll.empty()) {
        wchar_t self[MAX_PATH];
        GetModuleFileNameW(nullptr, self, MAX_PATH);
        dll = self;
        dll = dll.substr(0, dll.find_last_of(L'\\') + 1) + L"P4GTrainer.dll";
    }

    DWORD pid = FindGame();
    if (!pid && wait) {
        Say(L"Waiting for Persona 4 Golden to start...");
        while (!(pid = FindGame())) Sleep(1000);
        Sleep(8000);  // let the game create its window and device first
    }
    if (!pid) {
        Say(L"Persona 4 Golden (P4G.exe) is not running. Start it, or use --wait.");
        return 1;
    }
    HANDLE proc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                  PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
                              FALSE, pid);
    if (!proc) {
        Say(L"Could not open the game process (%lu). Run as the same user as the game.", GetLastError());
        return 1;
    }
    bool ok;
    if (eject) ok = Eject(pid, proc);
    else if (reload) ok = Eject(pid, proc) && Inject(pid, proc, dll);
    else ok = Inject(pid, proc, dll);
    CloseHandle(proc);
    return ok ? 0 : 1;
}
