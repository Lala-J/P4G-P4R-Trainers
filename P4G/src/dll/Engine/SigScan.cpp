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

namespace {

// VirtualQuery inside P4G's (very large, protected) image is slow: the kernel walks the whole
// region to report its size. Image sections are never unmapped and their protection is fixed
// once the game runs, so readable regions inside the image are remembered. Heap memory is
// always queried afresh.
struct Region {
    uintptr_t lo, hi;
};
SRWLOCK g_lock = SRWLOCK_INIT;
Region g_cache[64];
int g_cached = 0, g_next = 0;
uintptr_t g_imgLo = 0, g_imgHi = 0;

void InitImage() {
    if (g_imgHi) return;
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    g_imgLo = base;
    g_imgHi = base + nt->OptionalHeader.SizeOfImage;
}

bool InImage(uintptr_t a) { return a >= g_imgLo && a < g_imgHi; }

bool CachedReadable(uintptr_t a, uintptr_t* regionEnd) {
    AcquireSRWLockShared(&g_lock);
    bool hit = false;
    for (int i = 0; i < g_cached; ++i)
        if (a >= g_cache[i].lo && a < g_cache[i].hi) {
            *regionEnd = g_cache[i].hi;
            hit = true;
            break;
        }
    ReleaseSRWLockShared(&g_lock);
    return hit;
}

void Remember(uintptr_t lo, uintptr_t hi) {
    AcquireSRWLockExclusive(&g_lock);
    g_cache[g_next] = {lo, hi};
    g_next = (g_next + 1) % 64;
    if (g_cached < 64) ++g_cached;
    ReleaseSRWLockExclusive(&g_lock);
}

}  // namespace

bool Readable(uintptr_t addr, size_t len) {
    InitImage();
    uintptr_t end = addr + len;
    if (end < addr) return false;
    while (addr < end) {
        uintptr_t regionEnd;
        if (InImage(addr) && CachedReadable(addr, &regionEnd)) {
            addr = regionEnd;
            continue;
        }
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<void*>(addr), &mbi, sizeof mbi)) return false;
        if (mbi.State != MEM_COMMIT) return false;
        DWORD prot = mbi.Protect & 0xFF;
        bool ok = prot == PAGE_READONLY || prot == PAGE_READWRITE || prot == PAGE_WRITECOPY ||
                  prot == PAGE_EXECUTE_READ || prot == PAGE_EXECUTE_READWRITE ||
                  prot == PAGE_EXECUTE_WRITECOPY;
        if (!ok || (mbi.Protect & PAGE_GUARD)) return false;
        uintptr_t lo = reinterpret_cast<uintptr_t>(mbi.BaseAddress), hi = lo + mbi.RegionSize;
        if (InImage(lo) && hi <= g_imgHi) Remember(lo, hi);
        addr = hi;
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
