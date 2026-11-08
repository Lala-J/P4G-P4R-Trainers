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
void FillCompendium();         // register the base form of every Persona not yet registered

}  // namespace PersonaStock
