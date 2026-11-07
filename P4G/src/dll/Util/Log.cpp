#include "Util/Log.h"

#include <windows.h>
#include <share.h>
#include <cstdarg>
#include <cstdio>
#include <mutex>

namespace {
std::mutex g_mu;
FILE* g_file = nullptr;
}

namespace Log {

void Init(const std::wstring& path) {
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_file) return;
    // Append, so a hot-swapped build keeps writing to the same log; start over past 4 MB.
    WIN32_FILE_ATTRIBUTE_DATA fa{};
    bool big = GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fa) &&
               (fa.nFileSizeHigh != 0 || fa.nFileSizeLow > 4u * 1024 * 1024);
    // Shared so the log can be read (and tailed) while the game runs.
    g_file = _wfsopen(path.c_str(), big ? L"wb" : L"ab", _SH_DENYNO);
}

void Shutdown() {
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_file) {
        fclose(g_file);
        g_file = nullptr;
    }
}

void Write(const char* level, const char* fmt, ...) {
    char msg[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    SYSTEMTIME t;
    GetLocalTime(&t);
    char line[2200];
    snprintf(line, sizeof line, "%02u:%02u:%02u.%03u [%s] %s\n", t.wHour, t.wMinute, t.wSecond,
             t.wMilliseconds, level, msg);

    OutputDebugStringA(line);
    std::lock_guard<std::mutex> lk(g_mu);
    if (g_file) {
        fputs(line, g_file);
        fflush(g_file);
    }
}

}  // namespace Log
