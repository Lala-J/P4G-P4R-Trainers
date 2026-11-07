#include "Engine/Names.h"

#include <cctype>
#include <cstring>

#include "Engine/Addresses.h"
#include "Engine/HostCheck.h"
#include "Engine/SigScan.h"

namespace {

std::string ReadName(uintptr_t base, uint32_t id, uint32_t count, uint32_t stride) {
    if (!base || id >= count || !HostCheck::GroupOk("names")) return {};
    uintptr_t a = base + uintptr_t(id) * stride;
    if (!Sig::Readable(a, stride)) return {};
    char buf[32] = {};
    std::memcpy(buf, reinterpret_cast<const void*>(a), stride < 31 ? stride : 31);
    std::string s(buf);
    // Unused slots hold their own id in hex ("00", "0FF", "1A0") or "blank" / "Blank".
    // (Matching the id exactly keeps real names made of hex letters, such as "Bead".)
    if (s.empty() || s == "blank" || s == "Blank") return {};
    for (int width = 2; width <= 4; ++width) {
        char hex[8];
        snprintf(hex, sizeof hex, "%0*X", width, id);
        if (s == hex) return {};
    }
    for (char c : s)
        if (static_cast<unsigned char>(c) < 0x20) return {};
    return s;
}

std::string Or(const std::string& s, const char* kind, uint32_t id) {
    if (!s.empty()) return s;
    char buf[32];
    snprintf(buf, sizeof buf, "%s #%u", kind, id);
    return buf;
}

uintptr_t ItemBase() {
    if (!Sig::Readable(Addr::pItemNames, 8)) return 0;
    return *reinterpret_cast<const uintptr_t*>(Addr::pItemNames);
}

}  // namespace

namespace Names {

std::string Persona(uint16_t id) { return ReadName(Addr::PersonaNames, id, 256, 0x15); }
std::string Skill(uint16_t id) { return ReadName(Addr::SkillNames, id, 624, 0x17); }
std::string Enemy(uint16_t id) { return ReadName(Addr::EnemyNames, id, 368, 0x15); }
std::string Arcana(uint8_t id) { return ReadName(Addr::ArcanaNames, id, 32, 0x15); }
std::string Item(uint16_t id) { return ReadName(ItemBase(), id, 2560, 0x18); }

std::string PersonaOr(uint16_t id) { return Or(Persona(id), "Persona", id); }
std::string SkillOr(uint16_t id) { return Or(Skill(id), "Skill", id); }
std::string ItemOr(uint16_t id) { return Or(Item(id), "Item", id); }

}  // namespace Names
