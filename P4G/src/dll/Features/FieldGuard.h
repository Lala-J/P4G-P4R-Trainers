#pragma once
// Crash guard for the dungeon field.
//
// The game moves field objects (the party leader, followers, shadows) with a collision step,
// 0x14032A200 (object, room, ...), where `room` is the room model of the cell the object stands
// in. If an object ends up in a cell that belongs to no room (a wall cell; seen when the leader got
// trapped behind the end of a staircase), the room is null and the game dereferences it and
// crashes (P4G.exe+0x32A94B). The guard makes that step return 0, which the game's own code already
// returns when a probe blocks the move: the object simply doesn't move that frame. Each time it
// fires it is logged with the leader's position, and the Field page offers the way out.

namespace FieldGuard {

bool InstallHooks();
int Fired();            // times the guard has stopped a crash this session
bool LeaderOutsideRoom();  // the leader stands in a cell that belongs to no room (render thread)

}  // namespace FieldGuard
