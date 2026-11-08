#pragma once
// Yu's Persona stock and the compendium, changed only through the game's own routines:
//   give      CreatePersona (the base form a Shuffle Time card or a fresh fusion starts from), then
//             the same "add to stock" the Velvet Room uses, which also registers new Personas;
//   remove    the routine fusion uses to take ingredients out of the stock;
//   summon    "add to stock" with the compendium's saved copy, as the Velvet Room does;
//   register  the compendium's own register.
// Every change is journaled with an exact undo.

#include <cstdint>
#include <string>

#include "Engine/GameTypes.h"

namespace PersonaStock {

bool Available();          // host probes passed

// What the Persona table says about an id.
struct Info {
    bool exists = false;       // a real Persona (not an unused placeholder)
    bool yours = false;        // Yu can hold it (not a party member's or a story-only Persona)
    bool ultimate = false;     // normally needs the arcana's link at max rank
    bool special = false;      // special fusion
    bool ngPlus = false;       // New Game+ only
    uint8_t arcana = 0;
    uint8_t baseLevel = 0;
};
Info Describe(uint16_t id);

// Golden's two new arcana belong to links that are spoilers until met: Jester (Adachi) and Aeon
// (Marie). True when the arcana may be shown (its link has started, it was revealed, or
// spoilers are off).
int ArcanaLink(uint8_t arcana);
bool ArcanaVisible(uint8_t arcana);

// Live (game thread or UI; plain reads).
int Capacity();                                  // the game's own limit right now
int Count();
int SlotOf(uint16_t id);                         // -1 if not in stock
bool Registered(uint16_t id);
const Game::Persona* CompendiumEntry(uint16_t id);  // null if not registered
int Price(uint16_t id);                          // the Velvet Room's summon fee, -1 if unknown

// Why an action can't run now, or null.
const char* WhyNotGive(uint16_t id);
const char* WhyNotRemove(int slot);
const char* WhyNotSummon(uint16_t id, bool pay);

void Give(uint16_t id);
void Remove(int slot);
void Summon(uint16_t id, bool pay);
void Register(int slot);       // save this stock Persona's current form to the compendium
// Register the base form of every Persona not yet registered (skipping arcana that are still
// spoilers and the New Game+ Persona). CompendiumGaps counts what it would add.
void FillCompendium();
int CompendiumGaps();

// Fusion preview through the game's own two-Persona fusion (0x140240A90): the result's base form
// (id, level, skills before inheritance). kind 0 = no result, 1 = normal, 2 = special fusion.
struct Fusion {
    int kind = 0;
    Game::Persona result{};
};
Fusion Preview(uint16_t a, uint16_t b);   // game thread only

// Velvet Room rule: ultimate Personas can be fused before their arcana's link is maxed. The game
// keeps a table of {unlock flag, Persona}; this answers "unlocked" for every entry.
bool UnlockOverrideAvailable();
bool UltimatesUnlocked();
void SetUltimatesUnlocked(bool on);
bool InstallHooks();

// The game's unlock table, for labels: flag -> Persona id (0 if none).
int UnlockFlagCount();
bool UnlockEntry(int i, int* flag, uint16_t* persona);

}  // namespace PersonaStock
