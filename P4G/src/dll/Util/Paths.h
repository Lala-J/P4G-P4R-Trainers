#pragma once
// Every location the trainer touches is discovered at run time. Nothing is hardcoded.

#include <string>

namespace Paths {
// %LOCALAPPDATA%\P4GTrainer (created on demand)
std::wstring DataDir();
// DataDir()\<name> (created on demand)
std::wstring DataSub(const wchar_t* name);
// The Windows Fonts folder
std::wstring FontsDir();

std::string Narrow(const std::wstring& w);
std::wstring Widen(const std::string& s);
}  // namespace Paths
