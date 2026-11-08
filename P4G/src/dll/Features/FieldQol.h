#pragma once
// Field & dungeon shortcuts through the game's own script functions (the same calls the town's
// event scripts make), run only when the player is free-roaming.

#include <cstdint>

namespace FieldQol {

enum class Facility : uint8_t { Velvet, Daidara, Shiroku, Crocofur, Books, Produce };
const char* FacilityName(Facility f);

bool CanRunNow(const char** why);       // free-roaming in the field or a dungeon, no menu / battle
void OpenFacility(Facility f);
void SaveAnywhere();
bool InDungeon();
int CurrentFloorId();                   // dungeon floor id, or -1 (game thread; elsewhere the last value)
int CurrentFloorIdCached();
const char* WhyNotChangeFloor(int delta);  // null when the floor can change now
void GotoRelativeFloor(int delta);      // dungeon only: +1 next floor, -1 previous (the stairs' own steps)

// Minimap auto-reveal: every walkable cell of the current floor is passed to the game's own
// per-cell reveal (what walking into it does), so the whole floor shows on the minimap.
bool RevealMapAvailable();              // host probes passed
bool RevealMapEnabled();
void SetRevealMap(bool on);
int RevealedCells(int* total);          // current floor: cells shown / walkable cells (-1 if unknown)
void Tick();                            // every frame (game thread)

}  // namespace FieldQol
