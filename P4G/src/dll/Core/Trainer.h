#pragma once
// Lifecycle: start on injection, tear down cleanly on request, expose status to the UI.

#include <windows.h>
#include <string>

namespace Trainer {

void Start(HMODULE self);
void RequestUnload();

const char* BuildId();
const char* Version();   // semantic version, from P4G/VERSION
std::wstring BackupDir();     // this session's save backup (empty if none)
std::wstring LogPath();
bool HooksInstalled();

}  // namespace Trainer
