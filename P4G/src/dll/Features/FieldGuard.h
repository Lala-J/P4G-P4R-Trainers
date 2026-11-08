#pragma once
// Crash guard for the dungeon field.
//
// The game moves field objects (the party leader, followers, shadows) with a collision step,
// 0x14032A200 (object, room, ...), where `room` is the room model of the cell the object stands
// in. If an object ends up in a cell that belongs to no room (a wall cell; seen when the leader got
// trapped behind the end of a staircase), the room is null and the game dereferences it and
// crashes (P4G.exe+0x32A94B). The guard skips that step (returns 0) so nothing crashes, and Tick()
// moves the party back to where it last stood on the floor. Both are logged.

namespace FieldGuard {

bool InstallHooks();
void Tick();            // every frame (game thread): moves the party back if it left the floor
int Fired();            // times the guard has stopped a crash this session
int Rescues();          // times the party was moved back onto the floor
bool LeaderOutsideRoom();  // the leader stands in a cell that belongs to no room (render thread)

}  // namespace FieldGuard
