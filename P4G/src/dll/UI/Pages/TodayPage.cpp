#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "Data/Cases.h"
#include "Data/LinkRules.h"
#include "Data/Links.h"
#include "Data/Romance.h"
#include "Data/Today.h"
#include "Engine/Calendar.h"
#include "Engine/GameState.h"
#include "Features/Activities.h"
#include "Features/CaseFile.h"
#include "Features/SocialLinks.h"
#include "Features/Spoilers.h"
#include "Features/WhoIsAround.h"
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

// A romance (from the game's lover flag), or a romance route approaching its deciding rank. Once
// that rank has passed without the flag the two are just friends.
bool RomanceTier(const Game::Snapshot& s, uint16_t link, int rank) {
    if (Links::RomanceBond(s, link) != Links::Bond::None) return true;
    for (const auto& r : Data::kRoutes)
        if (r.link == link) return rank >= 7 && rank < r.loverRank;
    return false;
}

std::vector<Entry> Build(const Game::Snapshot& s) {
    std::vector<Entry> out;

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

    // Social links available now: who the game's schedule puts where (Features/WhoIsAround).
    for (const auto& a : Around::Now(s)) {
        const Game::Link* e = s.FindLink(a.link);
        const auto* info = Data::Link(a.link);
        if (info && info->romanceRow && (!e || !e->rank)) {
            const Game::Link* r = s.FindLink(info->romanceRow);
            if (r && r->rank) e = r;
        }
        bool started = e && e->rank;
        if (!started && !Spoilers::Visible(Spoilers::Kind::Link, a.link, false)) continue;
        bool ready = a.ready >= 0 ? a.ready == 1
                                  : started && e->rank < 10 && e->points >= Game::LinkThreshold(e->id, e->rank);
        bool rankUp = ready && !a.bike && !a.nightOut;
        Entry en;
        en.tier = RomanceTier(s, a.link, started ? e->rank : 0) ? Data::Tier::Romance : Data::Tier::Links;
        en.order = rankUp ? 0 : (started ? 10 + e->rank : 50) + (a.bike ? 1 : 0);
        char b[160];
        if (a.bike) snprintf(b, sizeof b, "%s — scooter trip", a.who);
        else if (!started) snprintf(b, sizeof b, "%s — start the link", a.who);
        else if (rankUp) snprintf(b, sizeof b, "%s — RANK UP READY (rank %d → %d)", a.who, e->rank, e->rank + 1);
        else snprintf(b, sizeof b, "%s — rank %d", a.who, e->rank);
        en.what = b;
        en.where = a.where;
        if (a.note) en.note = a.note;
        else if (a.bike) en.note = "Ride somewhere together on your scooters.";
        else if (a.nightOut) en.note = "Evening hangout: points and an item, no rank-up.";
        out.push_back(en);
    }

    // Jobs and other things to do: the game's own job and schedule flags, and its weather
    // (Features/Activities).
    for (auto& a : Activities::Now(s)) out.push_back({a.tier, a.order, std::move(a.what), std::move(a.where), std::move(a.note)});
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
    ImGui::Text("%d/%d (%s) — %s%s", d.month, d.day, Calendar::WeekdayShort(d.weekday), Calendar::TimeName(s.time),
                s.Raining() ? " · Rain" : "");
    ImGui::PopFont();
    if (s.time < 3)
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
            ImGui::PushTextWrapPos(0.0f);
            W::Dim("%s", e.note.c_str());
            ImGui::PopTextWrapPos();
            ImGui::Unindent();
        }
    }
    ImGui::Spacing();
    ImGui::PushTextWrapPos(0.0f);
    W::Dim("Who's around comes from the game's own daily schedule flags, checked the way the Find-a-Friend "
           "mod checks them; jobs from the same schedule and the jobs you've signed up for, and the "
           "weather from the game itself. Order: last chances, romance, social links (ready to rank up "
           "first), clubs and jobs, then everything else.");
    ImGui::PopTextWrapPos();
}

}  // namespace Pages
