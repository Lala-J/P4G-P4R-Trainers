#pragma once
// Equipment for Yu and the party, changed the way the game's equip menu does it:
//   - only items the member can wield (the game's own check, 0x1401A0220);
//   - the new piece comes out of the bag and the old one goes back in (equipped items are not
//     counted in the bag); outfits are a wardrobe and are only required to be owned;
//   - the swap itself is the game's SET_EQUIPMENT script function.
// Every change is journaled with an exact undo.

#include <cstdint>

namespace Equipment {

enum Slot : int { Weapon = 0, Armour = 1, Accessory = 2, Outfit = 3 };

bool Available();
const char* SlotName(int slot);

// The item id range a slot takes (weapons also include the Golden-only range).
bool ItemFitsSlot(uint16_t item, int slot);
bool CanWield(uint16_t item, int charId);  // the game's own rule

struct Stats {
    int attack = 0, hit = 0, defence = 0, evasion = 0;
};
Stats ItemStats(uint16_t item);

const char* WhyNot(int charId, int slot, uint16_t item);  // null when allowed now
void Equip(int charId, int slot, uint16_t item);

}  // namespace Equipment
