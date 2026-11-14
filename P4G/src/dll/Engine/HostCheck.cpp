#include "Engine/HostCheck.h"

#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>

#include "Engine/Addresses.h"
#include "Engine/SigScan.h"
#include "Util/Log.h"

namespace {

HostCheck::Result g_result;
std::map<std::string, bool> g_groups;  // group -> every probe in it passed
std::string g_failed;
std::once_flag g_once;

// Each probe passes, fails, or is still waiting for the game: a pointer the game fills in
// during its own start-up can still be null when the trainer loads (the Autoload loader starts
// it as soon as the game window exists, and on slower machines the game sets these pointers a
// little later). That isn't a mismatch, so the probe waits and is checked again every frame
// (Recheck) until the game has set it; its group stays off until then.
enum State : uint8_t { Pass, Fail, Waiting };
constexpr size_t kProbeCount = sizeof Addr::kProbes / sizeof Addr::kProbes[0];
State g_state[kProbeCount] = {};
std::mutex g_mu;  // g_groups, g_state, g_failed, g_result.probesPassed
std::atomic<int> g_waiting{0};
ULONGLONG g_started = 0;

bool InRange(uintptr_t a, uintptr_t start, size_t size) { return a >= start && a < start + size; }

bool CheckProbe(const Addr::Probe& p) {
    using K = Addr::ProbeKind;
    switch (p.kind) {
        case K::Code:
            return Sig::MatchAt(p.addr, p.sig);
        case K::Thunk: {
            if (!Sig::Readable(p.addr, 5)) return false;
            auto* b = reinterpret_cast<const uint8_t*>(p.addr);
            if (b[0] != 0xE9) return false;
            return InRange(Sig::RipTarget(p.addr, 1, 5), Addr::kArchStart, Addr::kArchSize);
        }
        case K::PtrEq:
            return Sig::Readable(p.addr, 8) && *reinterpret_cast<const uint64_t*>(p.addr) == p.value;
        case K::LazyPtrEq:
            if (!Sig::Readable(p.addr, 8)) return false;
            return *reinterpret_cast<const uint64_t*>(p.addr) == p.value ||
                   *reinterpret_cast<const uint64_t*>(p.addr) == 0;
        case K::LazyCode: {
            if (Sig::MatchAt(p.addr, p.sig)) return true;
            auto pat = Sig::Parse(p.sig);
            if (!Sig::Readable(p.addr, pat.bytes.size())) return false;
            auto* b = reinterpret_cast<const uint8_t*>(p.addr);
            for (size_t i = 0; i < pat.bytes.size(); ++i)
                if (b[i]) return false;
            return true;
        }
        case K::TableFn:
            return Sig::Readable(p.addr, 8) &&
                   InRange(*reinterpret_cast<const uint64_t*>(p.addr), Addr::kSharedStart,
                           Addr::kSharedSize);
    }
    return false;
}

State Evaluate(const Addr::Probe& p) {
    if (p.kind == Addr::ProbeKind::PtrEq && Sig::Readable(p.addr, 8) &&
        *reinterpret_cast<const uint64_t*>(p.addr) == 0)
        return Waiting;
    return CheckProbe(p) ? Pass : Fail;
}

// Groups, the pass count and the failed list, from g_state (g_mu held).
void Rebuild() {
    g_groups.clear();
    g_failed.clear();
    int passed = 0;
    for (size_t i = 0; i < kProbeCount; ++i) {
        const auto& p = Addr::kProbes[i];
        bool ok = g_state[i] == Pass;
        passed += ok;
        auto it = g_groups.find(p.group);
        g_groups[p.group] = (it == g_groups.end() ? true : it->second) && ok;
        if (g_state[i] == Fail) g_failed += std::string(p.name) + " [" + p.group + "]\n";
        if (g_state[i] == Waiting)
            g_failed += std::string(p.name) + " [" + p.group + "] (waiting for the game to set it up)\n";
    }
    g_result.probesPassed = passed;
}

bool SectionMatches(const IMAGE_SECTION_HEADER* s, const char* name, uintptr_t va, size_t size) {
    char n[9] = {};
    std::memcpy(n, s->Name, 8);
    return std::strcmp(n, name) == 0 && Addr::kImageBase + s->VirtualAddress == va &&
           s->Misc.VirtualSize == size;
}

void DoRun() {
    auto& r = g_result;
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    const wchar_t* leaf = wcsrchr(exe, L'\\');
    leaf = leaf ? leaf + 1 : exe;

    if (_wcsicmp(leaf, L"P4G.exe") != 0) {
        r.reason = "host is not P4G.exe";
    } else if (base != Addr::kImageBase) {
        r.reason = "P4G.exe is not at its preferred base (relocated build?)";
    } else {
        auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->FileHeader.TimeDateStamp != Addr::kTimestamp) {
            char buf[128];
            snprintf(buf, sizeof buf, "unsupported build (timestamp 0x%08lX, expected 0x%08X)",
                     nt->FileHeader.TimeDateStamp, Addr::kTimestamp);
            r.reason = buf;
        } else {
            // The protector rewrites SizeOfImage in memory, so check the section layout instead.
            bool shared = false, arch = false;
            auto* sec = IMAGE_FIRST_SECTION(nt);
            for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
                shared |= SectionMatches(&sec[i], ".shared", Addr::kSharedStart, Addr::kSharedSize);
                arch |= SectionMatches(&sec[i], ".arch", Addr::kArchStart, Addr::kArchSize);
            }
            if (!shared || !arch) r.reason = "section layout differs from v1.0.5";
            else r.supported = true;
        }
    }

    if (!r.supported) {
        LOGE("host check: %s. The trainer stays dormant.", r.reason.c_str());
        return;
    }

    g_started = GetTickCount64();
    std::lock_guard<std::mutex> lk(g_mu);
    int waiting = 0;
    for (size_t i = 0; i < kProbeCount; ++i) {
        const auto& p = Addr::kProbes[i];
        g_state[i] = Evaluate(p);
        if (g_state[i] == Fail)
            LOGW("probe failed: %s @ 0x%llX (group '%s' disabled)", p.name, (unsigned long long)p.addr, p.group);
        else if (g_state[i] == Waiting) {
            LOGI("probe waiting: %s (the game hasn't set it up yet; group '%s' waits for it)", p.name, p.group);
            ++waiting;
        }
    }
    r.probesTotal = int(kProbeCount);
    Rebuild();
    g_waiting = waiting;
    LOGI("host check: P4G.exe v1.0.5, %d/%d probes passed%s", r.probesPassed, r.probesTotal,
         waiting ? ", the rest waiting for the game to finish starting" : "");
}

}  // namespace

namespace HostCheck {

const Result& Run() {
    std::call_once(g_once, DoRun);
    return g_result;
}

const Result& Get() { return Run(); }

void Recheck() {
    if (!g_waiting.load(std::memory_order_relaxed) || !Run().supported) return;
    std::lock_guard<std::mutex> lk(g_mu);
    bool changed = false;
    int waiting = 0;
    double after = (GetTickCount64() - g_started) / 1000.0;
    for (size_t i = 0; i < kProbeCount; ++i) {
        if (g_state[i] != Waiting) continue;
        const auto& p = Addr::kProbes[i];
        g_state[i] = Evaluate(p);
        if (g_state[i] == Waiting) {
            ++waiting;
            continue;
        }
        changed = true;
        if (g_state[i] == Pass)
            LOGI("probe passed: %s, %.1f s after the trainer loaded", p.name, after);
        else
            LOGW("probe failed: %s @ 0x%llX (group '%s' disabled)", p.name, (unsigned long long)p.addr, p.group);
    }
    if (changed) {
        Rebuild();
        LOGI("host check: %d/%d probes passed", g_result.probesPassed, g_result.probesTotal);
    }
    g_waiting = waiting;
}

bool GroupOk(const char* group) {
    if (!Run().supported) return false;
    std::lock_guard<std::mutex> lk(g_mu);
    auto it = g_groups.find(group);
    return it != g_groups.end() && it->second;
}

std::string FailedProbes() {
    Run();
    std::lock_guard<std::mutex> lk(g_mu);
    return g_failed;
}

}  // namespace HostCheck
