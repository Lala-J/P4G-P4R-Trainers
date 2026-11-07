#include "Engine/SigScan.h"

#include <windows.h>
#include <cstring>

namespace Sig {

Pattern Parse(const char* text) {
    Pattern p;
    const char* s = text;
    while (*s) {
        while (*s == ' ') ++s;
        if (!*s) break;
        if (s[0] == '?') {
            p.bytes.push_back(0);
            p.mask.push_back(0);
            s += (s[1] == '?') ? 2 : 1;
            continue;
        }
        auto hex = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        int hi = hex(s[0]), lo = hex(s[1]);
        if (hi < 0 || lo < 0) return Pattern{};
        p.bytes.push_back(uint8_t(hi * 16 + lo));
        p.mask.push_back(1);
        s += 2;
    }
    p.valid = !p.bytes.empty();
    return p;
}

bool Readable(uintptr_t addr, size_t len) {
    uintptr_t end = addr + len;
    while (addr < end) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<void*>(addr), &mbi, sizeof mbi)) return false;
        if (mbi.State != MEM_COMMIT) return false;
        DWORD prot = mbi.Protect & 0xFF;
        bool ok = prot == PAGE_READONLY || prot == PAGE_READWRITE || prot == PAGE_WRITECOPY ||
                  prot == PAGE_EXECUTE_READ || prot == PAGE_EXECUTE_READWRITE ||
                  prot == PAGE_EXECUTE_WRITECOPY;
        if (!ok || (mbi.Protect & PAGE_GUARD)) return false;
        addr = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    }
    return true;
}

bool MatchAt(uintptr_t addr, const Pattern& p) {
    if (!p.valid || !Readable(addr, p.bytes.size())) return false;
    auto* m = reinterpret_cast<const uint8_t*>(addr);
    for (size_t i = 0; i < p.bytes.size(); ++i)
        if (p.mask[i] && m[i] != p.bytes[i]) return false;
    return true;
}

bool MatchAt(uintptr_t addr, const char* text) { return MatchAt(addr, Parse(text)); }

std::vector<uintptr_t> Find(uintptr_t start, size_t size, const Pattern& p, size_t maxHits) {
    std::vector<uintptr_t> hits;
    if (!p.valid || size < p.bytes.size()) return hits;
    // Walk region by region so unreadable pages are skipped instead of faulting.
    uintptr_t cur = start, end = start + size;
    while (cur < end && hits.size() < maxHits) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<void*>(cur), &mbi, sizeof mbi)) break;
        uintptr_t rEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (rEnd > end) rEnd = end;
        if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
            auto* m = reinterpret_cast<const uint8_t*>(cur);
            size_t n = rEnd - cur;
            size_t pl = p.bytes.size();
            uint8_t first = p.bytes[0];
            bool firstFixed = p.mask[0] != 0;
            for (size_t i = 0; i + pl <= n && hits.size() < maxHits; ++i) {
                if (firstFixed && m[i] != first) continue;
                size_t k = 0;
                while (k < pl && (!p.mask[k] || m[i + k] == p.bytes[k])) ++k;
                if (k == pl) hits.push_back(cur + i);
            }
        }
        cur = rEnd;
    }
    return hits;
}

uintptr_t RipTarget(uintptr_t insn, int dispOffset, int len) {
    int32_t disp;
    std::memcpy(&disp, reinterpret_cast<const void*>(insn + dispOffset), 4);
    return insn + len + disp;
}

}  // namespace Sig
