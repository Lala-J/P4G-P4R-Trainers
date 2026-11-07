#include "Util/Settings.h"

#include <windows.h>
#include <cstdio>
#include <cstdlib>

#include "Util/Paths.h"

namespace {
std::wstring g_file;

const std::wstring& File() {
    if (g_file.empty()) g_file = Paths::DataDir() + L"\\settings.ini";
    return g_file;
}
}  // namespace

namespace Settings {

void Load() { File(); }

int GetInt(const char* section, const char* key, int def) {
    return (int)GetPrivateProfileIntW(Paths::Widen(section).c_str(), Paths::Widen(key).c_str(), def,
                                      File().c_str());
}

void SetInt(const char* section, const char* key, int value) {
    wchar_t buf[32];
    swprintf_s(buf, L"%d", value);
    WritePrivateProfileStringW(Paths::Widen(section).c_str(), Paths::Widen(key).c_str(), buf,
                               File().c_str());
}

float GetFloat(const char* section, const char* key, float def) {
    wchar_t buf[64] = {};
    GetPrivateProfileStringW(Paths::Widen(section).c_str(), Paths::Widen(key).c_str(), L"", buf, 64,
                             File().c_str());
    return buf[0] ? (float)_wtof(buf) : def;
}

void SetFloat(const char* section, const char* key, float value) {
    wchar_t buf[64];
    swprintf_s(buf, L"%.3f", value);
    WritePrivateProfileStringW(Paths::Widen(section).c_str(), Paths::Widen(key).c_str(), buf,
                               File().c_str());
}

}  // namespace Settings
