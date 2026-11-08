#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "Data/Cases.h"
#include "Data/LinkRules.h"
#include "Data/Links.h"
#include "Data/Romance.h"
#include "Data/Stats.h"
#include "Data/Today.h"
#include "Engine/Calendar.h"
#include "Engine/GameState.h"
#include "Features/CaseFile.h"
#include "Features/Spoilers.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace {

struct Entry {
    Data::Tier tier;
    int order;           // tie-break inside a tier (lower first)
    std::string what, where, note;
};

const char* TierName(Data::Tier t) {
    switch (t) {
        case Data::Tier::LastChance: return "LAST CHANCE";
        case Data::Tier::Romance: return "ROMANCE";
        case Data::Tier::Links: return "SOCIAL LINKS";
        case Data::Tier::ClubsJobs: return "CLUBS & JOBS";
        default: return "EVERYTHING ELSE";
    }
}

ImU32 TierColor(Data::Tier t) {
    switch (t) {
        case Data::Tier::LastChance: return C::Red;
        case Data::Tier::Romance: return IM_COL32(240, 90, 150, 255);
        case Data::Tier::Links: return C::Yellow;
        case Data::Tier::ClubsJobs: return C::Blue;
        default: return C::Grey;
    }
}

bool Dating(const Game::Snapshot& s, uint16_t link) {
    for (const auto& r : Data::kRoutes)
        if (r.link == link) {
            for (const auto& l : s.links)
                if (l.id == r.loverRow && l.rank) return true;
            return r.loverFlag >= 0 && s.Flag(r.loverFlag);
        }
    return false;
}

bool RomanceRoute(uint16_t link) {
    for (const auto& r : Data::kRoutes)
        if (r.link == link) return true;
    return false;
}

std::vector<Entry> Build(const Game::Snapshot& s) {
    std::vector<Entry> out;
    bool night = s.time >= 5;
    auto d = Calendar::FromIndex(s.day);

    // Last chances: the case deadline and link cutoffs that are close.
    auto st = CaseFile::Current(s);
    if ((st.state == CaseFile::State::Active || st.state == CaseFile::State::LastDay) && st.daysLeft <= 3) {
        const auto& c = Data::kCases[st.index];
        char b[160];
        snprintf(b, sizeof b, "%s: %s", c.dungeon, st.daysLeft == 0 ? "today is the deadline" : "deadline very close");
        out.push_back({Data::Tier::LastChance, 0, b, "TV world (Junes electronics)",
                       st.todayUsable ? "You can still go in today." : "The TV world is closed for today."});
    }
    for (const auto& r : Data::kLinkRules) {
        if (!r.untilMonth) continue;
        int until = Calendar::ToIndex(r.untilMonth, r.untilDay);
        if (until < 0 || s.day > until || until - s.day > 7) continue;
        const Game::Link* e = s.FindLink(r.id);
        if (!e || !e->rank || e->rank >= 10) continue;
        const auto* info = Data::Link(r.id);
        char b[160];
        snprintf(b, sizeof b, "%s's link: %d day(s) left (until %d/%d)", info ? info->person : "?", until - s.day,
                 r.untilMonth, r.untilDay);
        out.push_back({Data::Tier::LastChance, 1, b, "", r.note ? r.note : ""});
    }

    // Social links available now, from the game's own availability flags.
    for (const auto& a : Data::kAvailability) {
        if ((a.slot == Data::Slot::Night) != night) continue;
        if (!s.Flag(a.flag)) continue;
        const Game::Link* e = s.FindLink(a.link);
        const auto* info = Data::Link(a.link);
        if (info && info->romanceRow && !e) e = s.FindLink(info->romanceRow);
        bool started = e && e->rank;
        if (!started && !Spoilers::Visible(Spoilers::Kind::Link, a.link, false)) continue;
        bool ready = started && e->rank < 10 && e->points >= Game::LinkThreshold(e->id, e->rank);
        Entry en;
        en.tier = Dating(s, a.link) || (RomanceRoute(a.link) && started && e->rank >= 7) ? Data::Tier::Romance
                                                                                          : Data::Tier::Links;
        en.order = ready ? 0 : (started ? 10 + e->rank : 50);
        char b[160];
        if (!started) snprintf(b, sizeof b, "%s — start the link", a.who);
        else if (e->rank >= 10) snprintf(b, sizeof b, "%s (max rank) — hang out for points / items", a.who);
        else if (ready) snprintf(b, sizeof b, "%s — RANK UP READY (rank %d → %d)", a.who, e->rank, e->rank + 1);
        else snprintf(b, sizeof b, "%s — rank %d", a.who, e->rank);
        en.what = b;
        en.where = a.where;
        if (a.slot == Data::Slot::Night) en.note = "Evening hangout: points and an item, no rank-up.";
        out.push_back(en);
    }

    // Clubs / jobs / other activities for this weekday and slot.
    const auto& yu = s.U(Game::Yu);
    for (const auto& a : Data::kActivities) {
        if ((a.slot == Data::Slot::Night) != night) continue;
        if (!(a.weekdays & (1 << d.weekday))) continue;
        if (a.fromMonth) {
            int from = Calendar::ToIndex(a.fromMonth, a.fromDay);
            if (from >= 0 && s.day < from) continue;
        }
        Entry en{a.tier, 100, a.what, a.where, ""};
        if (a.needStat >= 0) {
            int have = Data::StatRank(a.needStat, yu.social[a.needStat]);
            if (have < a.needRank) {
                char b[96];
                snprintf(b, sizeof b, "Needs %s rank %d (you have %d).", Data::kStats[a.needStat].name, a.needRank, have);
                en.note = b;
                en.order = 200;
            }
        }
        out.push_back(en);
    }
    std::stable_sort(out.begin(), out.end(), [](const Entry& a, const Entry& b) {
        return a.tier != b.tier ? a.tier < b.tier : a.order < b.order;
    });
    return out;
}

}  // namespace

namespace Pages {

void Today() {
    if (!RequireSave()) return;
    const auto& s = Game::Current();
    auto d = Calendar::FromIndex(s.day);
    ImGui::PushFont(g_fonts.title);
    ImGui::Text("%d/%d (%s) — %s", d.month, d.day, Calendar::WeekdayShort(d.weekday), Calendar::TimeName(s.time));
    ImGui::PopFont();
    if (s.time < 4)
        W::Dim("During the school day most people are busy; this fills in after school and in the evening.");

    auto list = Build(s);
    if (list.empty()) {
        ImGui::TextWrapped("Nothing to list for this time slot.");
        return;
    }
    Data::Tier cur = Data::Tier(255);
    for (const auto& e : list) {
        if (e.tier != cur) {
            cur = e.tier;
            ImGui::Spacing();
            W::Chip(TierName(cur), TierColor(cur));
        }
        ImGui::BulletText("%s", e.what.c_str());
        if (!e.where.empty()) {
            ImGui::SameLine();
            W::Dim("— %s", e.where.c_str());
        }
        if (!e.note.empty()) {
            ImGui::Indent();
            W::Dim("%s", e.note.c_str());
            ImGui::Unindent();
        }
    }
    ImGui::Spacing();
    W::Dim("Who's around comes from the game's own daily schedule flags. Order: last chances, romance, "
           "social links (ready to rank up first), clubs and jobs, then everything else.");
}

}  // namespace Pages
