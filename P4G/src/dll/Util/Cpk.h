#pragma once
// Read-only access to the game's CRI CPK archives (UTF tables, CRILAYLA), plus the Atlus PAK and
// TMX formats used for character portraits. Used to show the player's own game art at run time;
// nothing extracted is ever written to the repository.

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Cpk {

class Archive {
public:
    bool Open(const std::wstring& path);
    bool Read(const std::string& name, std::vector<uint8_t>& out);  // decompressed
    size_t FileCount() const { return files_.size(); }

private:
    struct Entry {
        uint64_t offset;
        uint32_t size, extractSize;
    };
    std::wstring path_;
    uint64_t toc_ = 0;
    std::unordered_map<std::string, Entry> files_;
};

bool Crilayla(const std::vector<uint8_t>& in, std::vector<uint8_t>& out);

// Atlus PAK (v2): u32 count, then {char name[32]; u32 size; data}. Returns the named entry, or the
// first entry whose name starts with `prefix`.
bool PakEntry(const std::vector<uint8_t>& pak, const char* prefix, std::vector<uint8_t>& out);

// TMX (PSMCT32, no palette) -> RGBA8 with alpha doubled (PS2 range 0..128).
bool TmxToRgba(const std::vector<uint8_t>& tmx, std::vector<uint8_t>& rgba, int& w, int& h);

// The game folder, from the running executable.
std::wstring GameDir();

}  // namespace Cpk
