#include "Engine/SaveBackup.h"

#include <windows.h>
#include <cstdio>

#include "Util/Log.h"
#include "Util/Paths.h"

namespace {

using SteamUser_t = void* (*)();
using GetUserDataFolder_t = bool (*)(void* self, char* buf, int cb);

std::wstring ParentOf(const std::wstring& p) {
    size_t s = p.find_last_of(L"\\/");
    return s == std::wstring::npos ? std::wstring() : p.substr(0, s);
}

}  // namespace

namespace SaveBackup {

std::wstring SaveFolder() {
    HMODULE steam = GetModuleHandleW(L"steam_api64.dll");
    if (!steam) return {};
    // steam_api64 flat API. The versioned accessor name matches the SDK the game shipped with.
    auto user = reinterpret_cast<SteamUser_t>(GetProcAddress(steam, "SteamAPI_SteamUser_v021"));
    auto folder = reinterpret_cast<GetUserDataFolder_t>(
        GetProcAddress(steam, "SteamAPI_ISteamUser_GetUserDataFolder"));
    if (!user || !folder) return {};
    void* self = user();
    if (!self) return {};
    char buf[MAX_PATH * 2] = {};
    if (!folder(self, buf, sizeof buf)) return {};
    // Steam returns ...\userdata\<account>\<appid>\local; Steam Cloud saves sit in ...\remote.
    std::wstring local = Paths::Widen(buf);
    while (!local.empty() && (local.back() == L'\\' || local.back() == L'/')) local.pop_back();
    std::wstring remote = ParentOf(local) + L"\\remote";
    DWORD a = GetFileAttributesW(remote.c_str());
    return (a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY)) ? remote : std::wstring();
}

std::wstring RunOnce() {
    // A named event lives as long as the process (the handle is deliberately kept), so a
    // reloaded build in the same session sees it and skips the backup.
    wchar_t name[64];
    swprintf_s(name, L"Local\\P4GTrainer.Backup.%lu", GetCurrentProcessId());
    HANDLE ev = CreateEventW(nullptr, TRUE, FALSE, name);
    if (!ev) return {};
    std::wstring marker = Paths::DataSub(L"backups") + L"\\.session";
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(ev);
        // Reloaded build: report the backup the first build made in this session.
        char buf[256] = {};
        if (FILE* f = _wfopen(marker.c_str(), L"rb")) {
            size_t n = fread(buf, 1, sizeof buf - 1, f);
            buf[n] = 0;
            fclose(f);
        }
        char* tab = strchr(buf, '\t');
        if (tab && strtoul(buf, nullptr, 10) == GetCurrentProcessId())
            return Paths::DataSub(L"backups") + L"\\" + Paths::Widen(tab + 1);
        return {};
    }
    // ev is intentionally not closed.

    std::wstring src = SaveFolder();
    if (src.empty()) {
        LOGW("save backup: could not locate the save folder through Steam; skipped");
        return {};
    }

    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t stamp[32];
    swprintf_s(stamp, L"%04u%02u%02u-%02u%02u%02u", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute,
               t.wSecond);
    std::wstring dst = Paths::DataSub(L"backups") + L"\\" + stamp;
    CreateDirectoryW(dst.c_str(), nullptr);

    int copied = 0, failed = 0;
    WIN32_FIND_DATAW fd;
    HANDLE f = FindFirstFileW((src + L"\\*").c_str(), &fd);
    if (f != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            std::wstring from = src + L"\\" + fd.cFileName, to = dst + L"\\" + fd.cFileName;
            if (CopyFileW(from.c_str(), to.c_str(), TRUE)) ++copied;
            else ++failed;
        } while (FindNextFileW(f, &fd));
        FindClose(f);
    }
    LOGI("save backup: %d file(s) copied to backups\\%ls%s", copied, stamp,
         failed ? " (some failed)" : "");
    if (FILE* m = _wfopen(marker.c_str(), L"wb")) {
        fprintf(m, "%lu\t%s", GetCurrentProcessId(), Paths::Narrow(stamp).c_str());
        fclose(m);
    }
    return dst;
}

}  // namespace SaveBackup
