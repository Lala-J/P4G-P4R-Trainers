#pragma once
// Changes to the game are queued here and applied at the start of the next frame, on the game's
// main thread (Present runs on the window's own thread, which is P4G's main loop), so nothing is
// ever modified while the game is halfway through updating it.

#include <functional>

namespace GameThread {

void Post(std::function<void()> fn);
void Drain();          // called by the Present hook before anything else
bool IsGameThread();
void BindToCurrent();  // records the main thread id (first Present)

}  // namespace GameThread
