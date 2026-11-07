#include <windows.h>
#include <cstdio>

#include "Core/Trainer.h"

namespace {
HANDLE g_instance = nullptr;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        // One trainer per process: a second copy refuses to load (the injector reloads by
        // ejecting the old build first).
        wchar_t name[64];
        swprintf_s(name, L"Local\\P4GTrainer.Instance.%lu", GetCurrentProcessId());
        g_instance = CreateMutexW(nullptr, FALSE, name);
        if (!g_instance) return FALSE;
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(g_instance);
            g_instance = nullptr;
            return FALSE;
        }
        Trainer::Start(module);
    } else if (reason == DLL_PROCESS_DETACH) {
        if (g_instance) CloseHandle(g_instance);
    }
    return TRUE;
}
