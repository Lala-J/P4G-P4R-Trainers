#pragma once
// Read-only snapshot of the game state, captured once per frame on the render thread while
// the trainer needs it. The UI reads only the snapshot, never live game memory.

#include <cstdint>

#include "Engine/GameTypes.h"

namespace Game {

enum class Mode { Unknown, Title, Field, Dungeon, Battle, Event, Menu };
const char* ModeName(Mode m);

struct Snapshot {
    bool valid = false;        // a save is loaded
    uint64_t frame = 0;

    // calendar
    uint16_t day = 0;          // 0 = April 1st
    uint8_t time = 0;          // 0..6 (see Calendar::TimeName)

    uint32_t yen = 0;
    uint16_t party[3] = {};
    int16_t equippedSlot = 0;
    Unit units[kUnits] = {};
    Persona stock[kStock] = {};
    Link links[kLinks] = {};
    uint8_t items[kItems] = {};
    uint8_t flags[kFlagBytes] = {};
    uint32_t counters[kCounters] = {};

    // context
    int fieldMajor = 0, fieldMinor = 0;
    bool inBattle = false;
    bool campMenu = false;
    Mode mode = Mode::Unknown;

    bool Flag(int n) const { return n >= 0 && n < kFlags && (flags[n >> 3] >> (n & 7)) & 1; }
    const Unit& U(int charId) const { return units[(charId - 1) & 7]; }
    const Link* FindLink(uint16_t id) const {
        for (const auto& l : links)
            if (l.id == id) return &l;
        return nullptr;
    }
};

// Capture (render thread). Cheap enough to run every frame the overlay is visible.
void Capture();
const Snapshot& Current();

// Live reads of static tables (safe on the render thread).
uint16_t LinkThreshold(uint16_t commuId, uint16_t rank);  // points needed at `rank` to rank up
uint8_t LinkArcana(uint16_t commuId);                     // arcana id from the commu table
uint32_t ExpForLevel(int level);                          // protagonist curve; 0 if out of range
uint8_t PersonaArcana(uint16_t personaId);
uint8_t PersonaBaseLevel(uint16_t personaId);

}  // namespace Game
