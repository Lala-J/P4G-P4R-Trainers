#pragma once
// Levels and EXP through the game's own level-up sequence (the one battle results use):
//   changes = GenerateLevelUpPersona(persona, gainedExp)   — stat growth + skills learned
//   persona->exp += gainedExp
//   ApplyLevelUp(persona, changes)                         — applies level, stats, skills
// so a Persona levelled by the trainer grows exactly as it would after a battle.

#include <cstdint>

#include "Engine/GameTypes.h"

namespace Levels {

bool Available();

// Persona targets: stock slot 0..11, or a party member (character id 2..8).
struct Target {
    bool member;  // false = Yu's stock slot
    int index;    // slot, or character id
};
const char* TargetName(const Target& t, char* buf, int n);

uint32_t ExpForPersonaLevel(const Game::Persona* p, int level);  // the game's own curve

// Queued on the game thread, journaled.
void AddExp(Target t, uint32_t exp);
void LevelUp(Target t, int levels);                // through to the next level(s), never down
void SetYuLevel(int level);                        // protagonist level + EXP on the game's curve

struct Options {
    int expMultiplier = 1;       // battle EXP x1..x10 (party and Personas)
    bool benchShare = false;     // members not in the active party gain the same EXP
};
Options& Opts();
void SaveOptions();
bool InstallHooks();

}  // namespace Levels
