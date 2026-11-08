// dinput8.dll for the game folder: loads the trainer automatically every time P4G starts.
//
// P4G.exe imports dinput8.dll, and Windows looks in the game's own folder first, so this DLL is
// loaded at startup instead of the system one. It:
//   1. forwards every dinput8 export to the real System32\dinput8.dll, so the game's input works
//      exactly as without it (also when the trainer is missing, disabled or unsupported);
//   2. on a background thread, waits until the game window is up, then loads P4GTrainer.dll from
//      the same folder. The trainer then checks the game build itself and stays dormant on an
//      unknown one.
// Turning it off: Settings page "Load automatically with the game", stored in
// %LOCALAPPDATA%\P4GTrainer\settings.ini [autoload] enabled=0, or remove the two files.

#include <windows.h>
#include <unknwn.h>

namespace {

HMODULE g_self = nullptr;
HMODULE g_real = nullptr;
INIT_ONCE g_once = INIT_ONCE_STATIC_INIT;

BOOL CALLBACK LoadReal(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t path[MAX_PATH];
    UINT n = GetSystemDirectoryW(path, MAX_PATH);
    if (n == 0 || n > MAX_PATH - 16) return TRUE;
    lstrcatW(path, L"\\dinput8.dll");
    g_real = LoadLibraryW(path);
    return TRUE;
}

FARPROC Real(const char* name) {
    InitOnceExecuteOnce(&g_once, LoadReal, nullptr, nullptr);
    return g_real ? GetProcAddress(g_real, name) : nullptr;
}

// --- the trainer -------------------------------------------------------------------------------

bool EndsWithI(const wchar_t* s, const wchar_t* tail) {
    size_t a = lstrlenW(s), b = lstrlenW(tail);
    return a >= b && lstrcmpiW(s + a - b, tail) == 0;
}

struct FindCtx {
    DWORD pid;
    bool found;
};

BOOL CALLBACK FindGameWindow(HWND h, LPARAM l) {
    auto* c = reinterpret_cast<FindCtx*>(l);
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid != c->pid || !IsWindowVisible(h)) return TRUE;
    wchar_t cls[64];
    if (GetClassNameW(h, cls, 64) && lstrcmpW(cls, L"DX11_P4G__app") == 0) {
        c->found = true;
        return FALSE;
    }
    return TRUE;
}

bool AutoloadEnabled() {
    wchar_t ini[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", ini, MAX_PATH);
    if (n == 0 || n > MAX_PATH - 40) return true;
    lstrcatW(ini, L"\\P4GTrainer\\settings.ini");
    return GetPrivateProfileIntW(L"autoload", L"enabled", 1, ini) != 0;
}

DWORD WINAPI Loader(LPVOID) {
    // Only inside the game itself.
    wchar_t exe[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, exe, MAX_PATH) || !EndsWithI(exe, L"\\P4G.exe")) return 0;
    if (!AutoloadEnabled()) return 0;

    // P4GTrainer.dll next to this file.
    wchar_t dll[MAX_PATH];
    DWORD n = GetModuleFileNameW(g_self, dll, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return 0;
    wchar_t* slash = dll;
    for (wchar_t* p = dll; *p; ++p)
        if (*p == L'\\' || *p == L'/') slash = p;
    if (slash == dll || (slash - dll) > MAX_PATH - 20) return 0;
    lstrcpyW(slash + 1, L"P4GTrainer.dll");
    if (GetFileAttributesW(dll) == INVALID_FILE_ATTRIBUTES) return 0;

    // Wait for the game window (the same moment the injector's --wait attaches), then a little
    // longer so the renderer is up. Give up quietly after 10 minutes.
    FindCtx c{GetCurrentProcessId(), false};
    for (int i = 0; i < 1200 && !c.found; ++i) {
        EnumWindows(FindGameWindow, reinterpret_cast<LPARAM>(&c));
        if (!c.found) Sleep(500);
    }
    if (!c.found) return 0;
    Sleep(3000);

    SetEnvironmentVariableW(L"P4GTRAINER_AUTOLOAD", L"1");  // so the trainer can show how it was loaded
    if (!LoadLibraryW(dll)) {
        wchar_t msg[96];
        wsprintfW(msg, L"P4G trainer autoload: LoadLibrary failed (%lu)\n", GetLastError());
        OutputDebugStringW(msg);
    }
    return 0;
}

}  // namespace

// --- dinput8 exports, forwarded ------------------------------------------------------------------

extern "C" {

HRESULT WINAPI P_DirectInput8Create(HINSTANCE inst, DWORD ver, REFIID riid, LPVOID* out, IUnknown* outer) {
    using F = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, IUnknown*);
    auto f = reinterpret_cast<F>(Real("DirectInput8Create"));
    return f ? f(inst, ver, riid, out, outer) : E_FAIL;
}
HRESULT WINAPI P_DllCanUnloadNow() {
    auto f = reinterpret_cast<HRESULT(WINAPI*)()>(Real("DllCanUnloadNow"));
    return f ? f() : S_FALSE;
}
HRESULT WINAPI P_DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID* out) {
    auto f = reinterpret_cast<HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*)>(Real("DllGetClassObject"));
    return f ? f(clsid, riid, out) : E_FAIL;
}
HRESULT WINAPI P_DllRegisterServer() {
    auto f = reinterpret_cast<HRESULT(WINAPI*)()>(Real("DllRegisterServer"));
    return f ? f() : E_FAIL;
}
HRESULT WINAPI P_DllUnregisterServer() {
    auto f = reinterpret_cast<HRESULT(WINAPI*)()>(Real("DllUnregisterServer"));
    return f ? f() : E_FAIL;
}
LPCVOID WINAPI P_GetdfDIJoystick() {
    auto f = reinterpret_cast<LPCVOID(WINAPI*)()>(Real("GetdfDIJoystick"));
    return f ? f() : nullptr;
}

}  // extern "C"

#pragma comment(linker, "/EXPORT:DirectInput8Create=P_DirectInput8Create")
#pragma comment(linker, "/EXPORT:DllCanUnloadNow=P_DllCanUnloadNow,PRIVATE")
#pragma comment(linker, "/EXPORT:DllGetClassObject=P_DllGetClassObject,PRIVATE")
#pragma comment(linker, "/EXPORT:DllRegisterServer=P_DllRegisterServer,PRIVATE")
#pragma comment(linker, "/EXPORT:DllUnregisterServer=P_DllUnregisterServer,PRIVATE")
#pragma comment(linker, "/EXPORT:GetdfDIJoystick=P_GetdfDIJoystick")

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = module;
        DisableThreadLibraryCalls(module);
        // Nothing heavy under the loader lock: the thread starts once the game's loader is done.
        if (HANDLE t = CreateThread(nullptr, 0, Loader, nullptr, 0, nullptr)) CloseHandle(t);
    }
    return TRUE;
}
