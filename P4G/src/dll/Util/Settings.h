#pragma once
// Persistent settings in %LOCALAPPDATA%\P4GTrainer\settings.ini.

#include <string>

namespace Settings {

void Load();
int GetInt(const char* section, const char* key, int def);
void SetInt(const char* section, const char* key, int value);
float GetFloat(const char* section, const char* key, float def);
void SetFloat(const char* section, const char* key, float value);

}  // namespace Settings
