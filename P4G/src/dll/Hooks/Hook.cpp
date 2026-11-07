#include "Hooks/Hook.h"

#include <windows.h>
#include <MinHook.h>
#include <mutex>
#include <vector>

#include "Util/Log.h"

namespace Hooks {

std::atomic<int> g_inflight{0};

namespace {
std::mutex g_mu;
std::vector<void*> g_targets;
bool g_init = false;
}  // namespace

bool Init() {
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_init) return true;
    MH_STATUS s = MH_Initialize();
    if (s != MH_OK) {
        LOGE("MinHook init failed: %s", MH_StatusToString(s));
        return false;
    }
    g_init = true;
    return true;
}

bool CreateRaw(const char* name, void* target, void* detour, void** original) {
    std::lock_guard<std::mutex> lk(g_mu);
    if (!g_init || !target) {
        LOGE("hook %s: %s", name, target ? "MinHook not initialised" : "no target");
        return false;
    }
    MH_STATUS s = MH_CreateHook(target, detour, original);
    if (s != MH_OK) {
        LOGE("hook %s @ %p: %s", name, target, MH_StatusToString(s));
        return false;
    }
    g_targets.push_back(target);
    LOGI("hook %s @ %p", name, target);
    return true;
}

bool EnableAll() {
    std::lock_guard<std::mutex> lk(g_mu);
    return MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
}

bool DisableAll() {
    std::lock_guard<std::mutex> lk(g_mu);
    return MH_DisableHook(MH_ALL_HOOKS) == MH_OK;
}

int Count() {
    std::lock_guard<std::mutex> lk(g_mu);
    return (int)g_targets.size();
}

bool WaitIdle(unsigned timeoutMs) {
    ULONGLONG until = GetTickCount64() + timeoutMs;
    // A thread may have just entered a detour and not yet reached its Guard, so require the
    // counter to read zero across a short settle window.
    int quiet = 0;
    while (GetTickCount64() < until) {
        if (g_inflight.load(std::memory_order_acquire) == 0) {
            if (++quiet >= 5) return true;
        } else {
            quiet = 0;
        }
        Sleep(10);
    }
    return false;
}

void Shutdown() {
    std::lock_guard<std::mutex> lk(g_mu);
    if (!g_init) return;
    MH_DisableHook(MH_ALL_HOOKS);
    for (void* t : g_targets) MH_RemoveHook(t);
    g_targets.clear();
    MH_Uninitialize();
    g_init = false;
}

}  // namespace Hooks
