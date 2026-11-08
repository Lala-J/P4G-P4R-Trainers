#include "Features/FlagLabels.h"

#include <cstdarg>
#include <cstdio>
#include <map>

#include "Data/Cases.h"
#include "Data/LinkRules.h"
#include "Data/Links.h"
#include "Data/Romance.h"
#include "Data/Today.h"

namespace FlagLabels {

namespace {

std::string Fmt(const char* fmt, ...) {
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    return buf;
}

// People whose very presence is a story reveal early on.
bool SpoilerPerson(uint16_t link) {
    switch (link) {
        case 3: case 4:    // Rise
        case 14: case 15:  // Naoto
        case 10:           // Kanji
        case 33: case 34:  // Marie
        case 31: case 32:  // Adachi
            return true;
    }
    return false;
}

const std::map<int, Label>& Table() {
    static std::map<int, Label> t;
    if (!t.empty()) return t;
    auto add = [](int n, std::string text, bool spoiler) {
        auto it = t.find(n);
        if (it == t.end()) t[n] = {std::move(text), spoiler};
        else it->second.text += " / " + text;  // one flag, several meanings
    };

    static const char* party[] = {"Yosuke", "Chie", "Yukiko", "Kanji", "Rise", "Teddie", "Naoto"};
    for (int i = 0; i < 7; ++i) add(48 + i, Fmt("%s can join the party", party[i]), i >= 3);

    for (int i = 0; i < Data::kCaseCount; ++i) {
        const auto& c = Data::kCases[i];
        if (c.rescueFlag >= 0) add(c.rescueFlag, Fmt("%s cleared (%s rescued)", c.dungeon, c.victim), i > 0);
    }
    for (int i = 0; i < Data::kRouteCount; ++i) {
        const auto& r = Data::kRoutes[i];
        if (r.loverFlag >= 0) add(r.loverFlag, Fmt("Dating %s", r.name), true);
    }
    for (const auto& a : Data::kAvailability)
        add(a.flag, Fmt("%s is around today (%s)", a.who, a.slot == Data::Slot::Day ? "daytime" : "evening"),
            SpoilerPerson(a.link));
    for (int q = 1; q <= 63; ++q) {
        add(255 + q, Fmt("Quest %d available", q), false);
        add(319 + q, Fmt("Quest %d completed", q), false);
    }
    add(2048, "Custom difficulty setting", false);
    add(2049, "Custom difficulty setting", false);
    add(1054, "Jester link turns into Hunger", true);
    add(1055, "Jester link turns into Hunger", true);
    add(2069, "Jester link turns into Hunger", true);
    for (const auto& e : Data::kLinkEffects) {
        if (e.kind != Data::LinkFx::BitOn) continue;
        const auto* info = Data::Link(e.link);
        add(e.a, Fmt("Set by %s's rank %u event", info ? info->person : "a social link", e.rank), SpoilerPerson(e.link));
    }
    return t;
}

}  // namespace

const Label* Flag(int n) {
    const auto& t = Table();
    auto it = t.find(n);
    return it == t.end() ? nullptr : &it->second;
}

int LabelledFlagCount() { return int(Table().size()); }

const char* Counter(int n) {
    switch (n) {
        case 124: return "Play time";
        case 127: return "Difficulty";
    }
    return nullptr;
}

}  // namespace FlagLabels
