#pragma once
// Thread-safe line logger. One file per game session under %LOCALAPPDATA%\P4GTrainer\logs.

#include <string>

namespace Log {
void Init(const std::wstring& path);
void Shutdown();
void Write(const char* level, const char* fmt, ...);
}

#define LOGI(...) ::Log::Write("info", __VA_ARGS__)
#define LOGW(...) ::Log::Write("WARN", __VA_ARGS__)
#define LOGE(...) ::Log::Write("FAIL", __VA_ARGS__)
