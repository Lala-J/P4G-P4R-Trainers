#pragma once
// Copies the player's save folder to %LOCALAPPDATA%\P4GTrainer\backups\<timestamp>\ once per
// game session (a hot-swapped build does not back up again). The trainer never writes saves.

#include <string>

namespace SaveBackup {

// Asks Steam (in-process) where this user's data lives for P4G. Empty if unavailable.
std::wstring SaveFolder();

// Backs up once per process. Returns the backup folder, or empty if nothing was done.
std::wstring RunOnce();

}  // namespace SaveBackup
