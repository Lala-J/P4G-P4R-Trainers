#include "Util/Cpk.h"

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>

namespace Cpk {

namespace {

uint16_t Be16(const uint8_t* p) { return uint16_t(p[0] << 8 | p[1]); }
uint32_t Be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3]; }
uint64_t Be64(const uint8_t* p) { return uint64_t(Be32(p)) << 32 | Be32(p + 4); }

std::string Lower(std::string s) {
    for (auto& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// A parsed @UTF table: column values per row, as integers or strings.
struct Utf {
    struct Col {
        std::string name;
        uint8_t storage, type;
        uint64_t constNum = 0;
        std::string constStr;
    };
    std::vector<Col> cols;
    std::vector<std::vector<uint64_t>> nums;
    std::vector<std::vector<std::string>> strs;

    int Index(const char* name) const {
        for (size_t i = 0; i < cols.size(); ++i)
            if (cols[i].name == name) return int(i);
        return -1;
    }
};

bool ParseUtf(const uint8_t* buf, size_t len, Utf& u) {
    if (len < 32 || std::memcmp(buf, "@UTF", 4)) return false;
    uint32_t size = Be32(buf + 4);
    if (size + 8 > len) return false;
    const uint8_t* t = buf + 8;
    uint16_t rowsOff = Be16(t + 2);
    uint32_t strOff = Be32(t + 4), dataOff = Be32(t + 8);
    uint16_t ncols = Be16(t + 16), rowLen = Be16(t + 18);
    uint32_t nrows = Be32(t + 20);
    (void)dataOff;
    auto cstr = [&](uint32_t o) -> std::string {
        if (strOff + o >= size) return {};
        return std::string(reinterpret_cast<const char*>(t + strOff + o));
    };
    auto valueSize = [](uint8_t type) -> int {
        switch (type) {
            case 0: case 1: return 1;
            case 2: case 3: return 2;
            case 4: case 5: case 8: case 0xA: return 4;
            case 6: case 7: case 9: case 0xB: return 8;
        }
        return -1;
    };
    auto readNum = [&](uint8_t type, const uint8_t* p) -> uint64_t {
        switch (type) {
            case 0: case 1: return p[0];
            case 2: case 3: return Be16(p);
            case 4: case 5: case 8: return Be32(p);
            case 6: case 7: case 9: return Be64(p);
            case 0xB: return Be32(p);  // data blob offset (unused here)
        }
        return 0;
    };
    size_t p = 24;
    for (uint16_t c = 0; c < ncols; ++c) {
        Utf::Col col;
        uint8_t flags = t[p];
        col.name = cstr(Be32(t + p + 1));
        p += 5;
        col.storage = flags & 0xF0;
        col.type = flags & 0x0F;
        int vs = valueSize(col.type);
        if (vs < 0) return false;
        if (col.storage == 0x30 || col.storage == 0x70) {
            if (col.type == 0xA) col.constStr = cstr(Be32(t + p));
            else col.constNum = readNum(col.type, t + p);
            p += vs;
        }
        u.cols.push_back(col);
    }
    u.nums.assign(nrows, std::vector<uint64_t>(ncols, 0));
    u.strs.assign(nrows, std::vector<std::string>(ncols));
    for (uint32_t r = 0; r < nrows; ++r) {
        size_t q = rowsOff + size_t(r) * rowLen;
        for (uint16_t c = 0; c < ncols; ++c) {
            const auto& col = u.cols[c];
            if (col.storage == 0x50) {
                int vs = valueSize(col.type);
                if (q + vs > size) return false;
                if (col.type == 0xA) u.strs[r][c] = cstr(Be32(t + q));
                else u.nums[r][c] = readNum(col.type, t + q);
                q += vs;
            } else if (col.storage == 0x30 || col.storage == 0x70) {
                u.nums[r][c] = col.constNum;
                u.strs[r][c] = col.constStr;
            }
        }
    }
    return true;
}

}  // namespace

bool Archive::Open(const std::wstring& path) {
    path_ = path;
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<uint8_t> head(0x800);
    f.read(reinterpret_cast<char*>(head.data()), head.size());
    if (std::memcmp(head.data(), "CPK ", 4)) return false;
    Utf h;
    if (!ParseUtf(head.data() + 0x10, head.size() - 0x10, h) || h.nums.empty()) return false;
    int ti = h.Index("TocOffset");
    if (ti < 0) return false;
    toc_ = h.nums[0][ti];
    uint8_t th[16];
    f.seekg(std::streamoff(toc_));
    f.read(reinterpret_cast<char*>(th), 16);
    uint64_t tocSize;
    std::memcpy(&tocSize, th + 8, 8);
    if (tocSize > (64u << 20)) return false;
    std::vector<uint8_t> tocBuf(static_cast<size_t>(tocSize));
    f.read(reinterpret_cast<char*>(tocBuf.data()), tocBuf.size());
    Utf toc;
    if (!ParseUtf(tocBuf.data(), tocBuf.size(), toc)) return false;
    int iDir = toc.Index("DirName"), iName = toc.Index("FileName"), iOff = toc.Index("FileOffset"),
        iSize = toc.Index("FileSize"), iExt = toc.Index("ExtractSize");
    if (iName < 0 || iOff < 0 || iSize < 0 || iExt < 0) return false;
    for (size_t r = 0; r < toc.nums.size(); ++r) {
        std::string dir = iDir >= 0 ? toc.strs[r][iDir] : std::string();
        std::string name = (dir.empty() ? "" : dir + "/") + toc.strs[r][iName];
        files_[Lower(name)] = {toc.nums[r][iOff], uint32_t(toc.nums[r][iSize]), uint32_t(toc.nums[r][iExt])};
    }
    return !files_.empty();
}

bool Archive::Read(const std::string& name, std::vector<uint8_t>& out) {
    auto it = files_.find(Lower(name));
    if (it == files_.end()) return false;
    std::ifstream f(path_, std::ios::binary);
    if (!f) return false;
    std::vector<uint8_t> raw(it->second.size);
    f.seekg(std::streamoff(toc_ + it->second.offset));
    f.read(reinterpret_cast<char*>(raw.data()), raw.size());
    if (!f) return false;
    if (it->second.size < it->second.extractSize) return Crilayla(raw, out);
    out.swap(raw);
    return true;
}

bool Crilayla(const std::vector<uint8_t>& in, std::vector<uint8_t>& out) {
    if (in.size() < 0x110 || std::memcmp(in.data(), "CRILAYLA", 8)) return false;
    uint32_t usize, hdr;
    std::memcpy(&usize, in.data() + 8, 4);
    std::memcpy(&hdr, in.data() + 12, 4);
    if (0x10 + size_t(hdr) + 0x100 > in.size() || usize > (256u << 20)) return false;
    out.assign(size_t(usize) + 0x100, 0);
    std::memcpy(out.data(), in.data() + 0x10 + hdr, 0x100);

    int64_t src = int64_t(in.size()) - 0x100 - 1;
    uint32_t pool = 0;
    int left = 0;
    auto bits = [&](int n) -> uint32_t {
        uint32_t v = 0;
        while (n > 0) {
            if (left == 0) {
                if (src < 0) return v;
                pool = in[size_t(src--)];
                left = 8;
            }
            int take = n < left ? n : left;
            v = (v << take) | ((pool >> (left - take)) & ((1u << take) - 1));
            left -= take;
            n -= take;
        }
        return v;
    };
    const size_t dstEnd = 0x100 + size_t(usize) - 1;
    size_t done = 0;
    static const int vle[4] = {2, 3, 5, 8};
    while (done < usize) {
        if (bits(1)) {
            size_t ref = dstEnd - done + bits(13) + 3;
            size_t length = 3;
            int i = 0;
            for (; i < 4; ++i) {
                uint32_t x = bits(vle[i]);
                length += x;
                if (x != (1u << vle[i]) - 1) break;
            }
            if (i == 4) {
                uint32_t x;
                do {
                    x = bits(8);
                    length += x;
                } while (x == 255);
            }
            for (size_t k = 0; k < length && done < usize; ++k) {
                if (ref >= out.size()) return false;
                out[dstEnd - done] = out[ref--];
                ++done;
            }
        } else {
            out[dstEnd - done] = uint8_t(bits(8));
            ++done;
        }
    }
    return true;
}

bool PakEntry(const std::vector<uint8_t>& pak, const char* prefix, std::vector<uint8_t>& out) {
    if (pak.size() < 4) return false;
    uint32_t count;
    std::memcpy(&count, pak.data(), 4);
    size_t p = 4;
    size_t plen = std::strlen(prefix);
    for (uint32_t i = 0; i < count && p + 36 <= pak.size(); ++i) {
        char name[33] = {};
        std::memcpy(name, pak.data() + p, 32);
        uint32_t size;
        std::memcpy(&size, pak.data() + p + 32, 4);
        p += 36;
        if (p + size > pak.size()) return false;
        if (std::strncmp(name, prefix, plen) == 0) {
            out.assign(pak.begin() + p, pak.begin() + p + size);
            return true;
        }
        p += size;
    }
    return false;
}

bool TmxToRgba(const std::vector<uint8_t>& tmx, std::vector<uint8_t>& rgba, int& w, int& h) {
    if (tmx.size() < 0x40 || std::memcmp(tmx.data() + 8, "TMX0", 4)) return false;
    // Header: +0x10 palette count, +0x11 palette format, +0x12 u16 width, +0x14 u16 height,
    // +0x16 pixel format (0 = PSMCT32), pixels from +0x40.
    uint8_t paletteCount = tmx[0x10];
    uint16_t width, height;
    std::memcpy(&width, tmx.data() + 0x12, 2);
    std::memcpy(&height, tmx.data() + 0x14, 2);
    uint8_t pixFmt = tmx[0x16];
    if (paletteCount != 0 || pixFmt != 0 || !width || !height) return false;
    size_t need = 0x40 + size_t(width) * height * 4;
    if (tmx.size() < need) return false;
    w = width;
    h = height;
    rgba.assign(tmx.begin() + 0x40, tmx.begin() + need);
    for (size_t i = 3; i < rgba.size(); i += 4) rgba[i] = uint8_t(std::min(255, rgba[i] * 2));
    return true;
}

std::wstring GameDir() {
    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring d = exe;
    return d.substr(0, d.find_last_of(L'\\'));
}

}  // namespace Cpk
