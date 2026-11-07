#include "Engine/HostCheck.h"

#include <windows.h>
#include <cstring>
#include <map>
#include <mutex>

#include "Engine/Addresses.h"
#include "Engine/SigScan.h"
#include "Util/Log.h"

namespace {

HostCheck::Result g_result;
std::map<std::string, bool> g_groups;
std::string g_failed;
std::once_flag g_once;

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
        case K::TableFn:
            return Sig::Readable(p.addr, 8) &&
                   InRange(*reinterpret_cast<const uint64_t*>(p.addr), Addr::kSharedStart,
                           Addr::kSharedSize);
    }
    return false;
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

    for (const auto& p : Addr::kProbes) {
        bool ok = CheckProbe(p);
        ++r.probesTotal;
        if (ok) ++r.probesPassed;
        auto it = g_groups.find(p.group);
        g_groups[p.group] = (it == g_groups.end() ? true : it->second) && ok;
        if (!ok) {
            LOGW("probe failed: %s @ 0x%llX (group '%s' disabled)", p.name,
                 (unsigned long long)p.addr, p.group);
            g_failed += std::string(p.name) + " [" + p.group + "]\n";
        }
    }
    LOGI("host check: P4G.exe v1.0.5, %d/%d probes passed", r.probesPassed, r.probesTotal);
}

}  // namespace

namespace HostCheck {

const Result& Run() {
    std::call_once(g_once, DoRun);
    return g_result;
}

const Result& Get() { return Run(); }

bool GroupOk(const char* group) {
    if (!Run().supported) return false;
    auto it = g_groups.find(group);
    return it != g_groups.end() && it->second;
}

std::string FailedProbes() {
    Run();
    return g_failed;
}

}  // namespace HostCheck
