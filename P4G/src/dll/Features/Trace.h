#pragma once
// Reverse-engineering aids, active only when %LOCALAPPDATA%\P4GTrainer\debug-trace exists:
// a per-frame flag/counter change monitor, battle start/end dumps, and logging hooks on battle
// resolvers. Nothing here changes game behaviour.

namespace Trace {

bool Enabled();
bool InstallHooks();  // no-op unless enabled
void Tick();          // every frame on the game thread

}  // namespace Trace
