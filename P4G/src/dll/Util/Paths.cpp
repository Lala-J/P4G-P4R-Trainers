#include "Util/Paths.h"

#include <windows.h>
#include <shlobj.h>

namespace {
std::wstring KnownFolder(REFKNOWNFOLDERID id) {
    PWSTR p = nullptr;
    std::wstring out;
    if (SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_CREATE, nullptr, &p))) out = p;
    CoTaskMemFree(p);
    return out;
}
}  // namespace

namespace Paths {

std::wstring DataDir() {
    std::wstring d = KnownFolder(FOLDERID_LocalAppData) + L"\\P4GTrainer";
    CreateDirectoryW(d.c_str(), nullptr);
    return d;
}

std::wstring DataSub(const wchar_t* name) {
    std::wstring d = DataDir() + L"\\" + name;
    CreateDirectoryW(d.c_str(), nullptr);
    return d;
}

std::wstring FontsDir() { return KnownFolder(FOLDERID_Fonts); }

std::string Narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring Widen(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

}  // namespace Paths
