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
int CurrentFloorId();                   // dungeon floor id, or -1
void GotoRelativeFloor(int delta);      // dungeon only: +1 next floor, -1 previous

}  // namespace FieldQol
