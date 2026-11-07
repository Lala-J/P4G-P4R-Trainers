#pragma once
// In-memory layouts of the save block (P4G.exe v1.0.5). Offsets verified against the live
// game and against the Steam save files; see docs/research/memory_layout.md.

#include <cstddef>
#include <cstdint>

namespace Game {

#pragma pack(push, 1)

struct Persona {             // 0x30
    uint8_t valid;           // +00 registered / slot in use
    uint8_t _01;
    uint16_t id;             // +02
    uint8_t level;           // +04
    uint8_t _05[3];
    uint32_t exp;            // +08
    uint16_t skills[8];      // +0C
    uint8_t stats[5];        // +1C St Ma En Ag Lu (base)
    uint8_t bonus[5];        // +21 (incense / growth)
    uint8_t other[5];        // +26
    uint8_t _2B[5];
};
static_assert(sizeof(Persona) == 0x30);

struct Unit {                // 0x84, index = character id - 1
    uint16_t flags;          // +00 bit0 = in party
    uint16_t _02, _04;
    uint8_t level;           // +06 (meaningful for the protagonist)
    uint8_t _07;
    uint16_t hp;             // +08
    uint16_t sp;             // +0A
    uint32_t ailments;       // +0C
    uint8_t _10[0x24];       // buffs / battle scratch
    uint16_t social[5];      // +34 Courage, Knowledge, Diligence, Understanding, Expression
    uint8_t _3E[2];
    uint32_t exp;            // +40 protagonist total EXP
    uint8_t _44[8];
    uint16_t equip[4];       // +4C weapon, armour, accessory, outfit (item ids)
    Persona persona;         // +54 party member's own Persona (level = member level)
};
static_assert(sizeof(Unit) == 0x84);

struct Link {                // 0x10
    uint16_t id;             // commu id (0 = empty slot)
    uint16_t rank;
    uint16_t points;
    uint8_t _06[10];
};
static_assert(sizeof(Link) == 0x10);

#pragma pack(pop)

constexpr int kUnits = 8;
constexpr int kStock = 12;
constexpr int kLinks = 23;
constexpr int kItems = 2560;
constexpr int kFlagBytes = 0x340;  // 0x1A00 flags
constexpr int kFlags = kFlagBytes * 8;
constexpr int kCounters = 256;
constexpr int kCompendium = 256;

// Offsets inside the save block
namespace Off {
constexpr size_t Yen = 0x0000;
constexpr size_t Party = 0x0004;
constexpr size_t Items = 0x0030;
constexpr size_t EquippedSlot = 0x0A30;
constexpr size_t Stock = 0x0A34;
constexpr size_t Units = 0x0C74;
constexpr size_t Flags = 0x119C;
constexpr size_t Counters = 0x14DC;
constexpr size_t Date = 0x18DC;
constexpr size_t Links = 0x18F0;
constexpr size_t Compendium = 0x2510;
}  // namespace Off

enum class Stat { Courage, Knowledge, Diligence, Understanding, Expression };

// Character ids (Unit index = id - 1)
enum Char : uint8_t { Yu = 1, Yosuke, Chie, Yukiko, Rise, Kanji, Naoto, Teddie };

}  // namespace Game
