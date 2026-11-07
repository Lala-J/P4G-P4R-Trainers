#include "Engine/GameThread.h"

#include <windows.h>
#include <mutex>
#include <vector>

#include "Util/Log.h"

namespace {
std::mutex g_mu;
std::vector<std::function<void()>> g_queue;
DWORD g_tid = 0;
}  // namespace

namespace GameThread {

void Post(std::function<void()> fn) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_queue.push_back(std::move(fn));
}

void Drain() {
    std::vector<std::function<void()>> q;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        q.swap(g_queue);
    }
    for (auto& fn : q) {
        try {
            fn();
        } catch (...) {
            LOGE("a queued game change threw an exception and was skipped");
        }
    }
}

bool IsGameThread() { return g_tid && GetCurrentThreadId() == g_tid; }

void BindToCurrent() {
    if (!g_tid) g_tid = GetCurrentThreadId();
}

}  // namespace GameThread
