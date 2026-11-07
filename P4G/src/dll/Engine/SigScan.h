#pragma once
// Byte-signature helpers. Signatures are IDA-style strings: "48 8B 05 ?? ?? ?? ?? 90".

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Sig {

struct Pattern {
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> mask;  // 1 = must match, 0 = wildcard
    bool valid = false;
};

Pattern Parse(const char* text);

// True if [addr, addr+len) is committed, readable memory.
bool Readable(uintptr_t addr, size_t len);

// True if the bytes at addr match the pattern (memory is checked for readability first).
bool MatchAt(uintptr_t addr, const Pattern& p);
bool MatchAt(uintptr_t addr, const char* text);

// All matches inside [start, start+size), up to maxHits.
std::vector<uintptr_t> Find(uintptr_t start, size_t size, const Pattern& p, size_t maxHits = 8);

// Resolve a RIP-relative operand: instruction at insn, disp32 at insn+dispOffset, insn length len.
uintptr_t RipTarget(uintptr_t insn, int dispOffset, int len);

}  // namespace Sig
