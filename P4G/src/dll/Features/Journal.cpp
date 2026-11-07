#include "Features/Journal.h"

#include <windows.h>
#include <cstdio>

#include "Engine/Calendar.h"
#include "Engine/GameState.h"
#include "Engine/GameThread.h"
#include "Util/Log.h"

namespace {
std::vector<Journal::Entry> g_entries;
int g_next = 1;
}  // namespace

namespace Journal {

void Record(std::string what, std::function<void()> undo) {
    const auto& s = Game::Current();
    SYSTEMTIME t;
    GetLocalTime(&t);
    char when[64];
    if (s.valid) {
        auto d = Calendar::FromIndex(s.day);
        snprintf(when, sizeof when, "%d/%d %s, %02u:%02u", d.month, d.day, Calendar::TimeName(s.time),
                 t.wHour, t.wMinute);
    } else {
        snprintf(when, sizeof when, "%02u:%02u", t.wHour, t.wMinute);
    }
    LOGI("change: %s", what.c_str());
    g_entries.push_back({g_next++, std::move(what), when, std::move(undo), false});
}

const std::vector<Entry>& Entries() { return g_entries; }

void Undo(int id) {
    for (auto& e : g_entries) {
        if (e.id != id || e.undone || !e.undo) continue;
        e.undone = true;
        auto fn = e.undo;
        std::string what = e.what;
        GameThread::Post([fn, what] {
            fn();
            LOGI("undone: %s", what.c_str());
        });
    }
}

void UndoAll() {
    for (auto it = g_entries.rbegin(); it != g_entries.rend(); ++it)
        if (!it->undone) Undo(it->id);
}

}  // namespace Journal
