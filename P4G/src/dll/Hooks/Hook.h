#pragma once
// Thin MinHook wrapper with in-flight tracking, so unloading can wait until no thread is
// still executing one of our detours before the DLL's code is freed.

#include <atomic>
#include <cstdint>

namespace Hooks {

bool Init();
void Shutdown();  // disables + removes every hook, then MH_Uninitialize

// Creates (but does not enable) a hook. `original` receives the trampoline.
bool CreateRaw(const char* name, void* target, void* detour, void** original);

template <typename F>
bool Create(const char* name, void* target, F detour, F* original) {
    return CreateRaw(name, target, reinterpret_cast<void*>(detour),
                     reinterpret_cast<void**>(original));
}

bool EnableAll();
bool DisableAll();
int Count();

// Place at the top of every detour.
extern std::atomic<int> g_inflight;
struct Guard {
    Guard() { g_inflight.fetch_add(1, std::memory_order_acq_rel); }
    ~Guard() { g_inflight.fetch_sub(1, std::memory_order_acq_rel); }
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
};

// Waits until no detour is running (or the timeout passes). Returns true when idle.
bool WaitIdle(unsigned timeoutMs);

}  // namespace Hooks
