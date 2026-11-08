#include "Features/EnemyIntel.h"

#include "Engine/Addresses.h"
#include "Engine/HostCheck.h"
#include "Engine/SigScan.h"
#include "Features/BattleOverrides.h"
#include "Util/Settings.h"

namespace Intel {

namespace {
bool g_hud = false;
bool g_loaded = false;

uintptr_t Ptr(uintptr_t cell) {
    if (!Sig::Readable(cell, 8)) return 0;
    uintptr_t v = *reinterpret_cast<const uintptr_t*>(cell);
    return (v && Sig::Readable(v, 8)) ? v : 0;
}
}  // namespace

const char* AffShort(Aff a) {
    switch (a) {
        case Aff::Weak: return "Wk";
        case Aff::Resist: return "Rs";
        case Aff::Null: return "Nu";
        case Aff::Repel: return "Rp";
        case Aff::Drain: return "Dr";
        default: return "-";
    }
}

const char* ElementName(int e) {
    static const char* n[] = {"Phys", "Fire", "Ice", "Elec", "Wind", "Almighty", "Light", "Dark"};
    return (e >= 0 && e < 8) ? n[e] : "?";
}

const char* AilmentName(int a) {
    static const char* n[] = {"Panic", "Poison", "Fear", "Rage", "Dizzy", "Exhaust", "Enervate", "Silence"};
    return (a >= 0 && a < 8) ? n[a] : "?";
}

bool Available() { return HostCheck::GroupOk("intel") && Ptr(Addr::pEnemyAffinity) != 0; }

Aff Affinity(uint16_t id, int elem) {
    uintptr_t t = Ptr(Addr::pEnemyAffinity);
    if (!t || id >= 368 || elem < 0 || elem > 15) return Aff::Normal;
    uintptr_t a = t + (uintptr_t(id) * 16 + elem) * 2;
    if (!Sig::Readable(a, 2)) return Aff::Normal;
    uint16_t raw = *reinterpret_cast<const uint16_t*>(a);
    uint8_t f = raw >> 8;  // flags; the low byte is the damage percentage for some entries
    if (f & 0x04) return Aff::Drain;
    if (f & 0x02) return Aff::Repel;
    if (f & 0x01) return Aff::Null;
    if (f & 0x08) return Aff::Weak;
    if (f & 0x10) return Aff::Resist;
    return Aff::Normal;
}

bool Seen(uint16_t id) {
    // The game's own check, called directly (bypasses the trainer's reveal switch).
    for (uint16_t e = 0; e < 8; ++e)
        if (BattleFx::KnownUnhooked(id, e)) return true;
    return false;
}

std::vector<Foe> CurrentEnemies() {
    std::vector<Foe> out;
    uintptr_t bm = Ptr(Addr::pBattleMgr);
    if (!bm) return out;
    uintptr_t n = Sig::Readable(bm + 0x1D8, 8) ? *reinterpret_cast<const uintptr_t*>(bm + 0x1D8) : 0;
    for (int i = 0; n && i < 12; ++i) {
        if (!Sig::Readable(n + 0xCF0, 0x18)) break;
        uintptr_t u = *reinterpret_cast<const uintptr_t*>(n + 0xCF0);
        if (u && Sig::Readable(u, 0x10)) {
            Foe f{};
            f.id = *reinterpret_cast<const uint16_t*>(u + 4);
            f.hp = *reinterpret_cast<const uint16_t*>(u + 8);
            f.sp = *reinterpret_cast<const uint16_t*>(u + 0xA);
            uint32_t st = *reinterpret_cast<const uint32_t*>(u + 0xC);
            if (!(st & 0x80000)) out.push_back(f);  // skip defeated / gone
        }
        n = *reinterpret_cast<const uintptr_t*>(n + 0xD00);
    }
    return out;
}

bool& ShowHud() {
    if (!g_loaded) {
        g_loaded = true;
        g_hud = Settings::GetInt("intel", "hud", 0) != 0;
    }
    return g_hud;
}

void SaveOptions() { Settings::SetInt("intel", "hud", g_hud ? 1 : 0); }

Details Info(uint16_t id) {
    Details d;
    uintptr_t t = Ptr(Addr::pEnemyTable);
    if (!t || id >= 368 || !Sig::Readable(t + id * 0x3C, 0x3C)) return d;
    const uint8_t* e = reinterpret_cast<const uint8_t*>(t + id * 0x3C);
    auto u16 = [e](int off) { return *reinterpret_cast<const uint16_t*>(e + off); };
    d.ok = true;
    d.arcana = e[2];
    d.level = e[3];
    d.hp = u16(4);
    d.sp = u16(6);
    for (int i = 0; i < 5; ++i) d.stats[i] = e[8 + i];
    for (int i = 0; i < 8; ++i) d.skills[i] = u16(0x0E + i * 2);
    d.exp = u16(0x1E);
    d.yen = u16(0x20);
    for (int i = 0; i < 3; ++i) {
        d.drop[i] = u16(0x22 + i * 4);
        d.dropRate[i] = u16(0x24 + i * 4);
    }
    return d;
}

}  // namespace Intel
