#pragma once
// Typed entry points into the game's own functions. Call only on the game thread.

#include <cstdint>

#include "Engine/GameTypes.h"

namespace Natives {

void SetItemCount(uint16_t id, uint8_t count);  // clamps to 99, runs the game's item bookkeeping
uint16_t GetMaxHP(const Game::Unit* u);
uint16_t GetMaxSP(const Game::Unit* u);
void AddHP(Game::Unit* u, int delta);           // clamps 0..max (the game's own helper)
void AddSP(Game::Unit* u, int delta);

// Set while the trainer itself calls AddHP/AddSP, so its own hooks let the call through.
extern thread_local bool g_trainerCall;

}  // namespace Natives
