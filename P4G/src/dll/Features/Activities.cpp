#include "Features/Activities.h"

#include <cstdio>

#include "Data/Stats.h"
#include "Engine/Calendar.h"

namespace Activities {

namespace {

constexpr const char* kBusStop = "Bus stop, shopping district south side";
constexpr const char* kDesk = "Your desk at home";
constexpr const char* kBoard = "Job board, shopping district";
constexpr const char* kAiya = "Aiya Chinese Diner, shopping district";

constexpr int kStoryDay1205 = 1551;  // 12/5: "You have to gather information" (town closed by day)
constexpr int kStoryDay0213 = 1544;  // 2/13: "go to the mountain cabin" (town closed by day)
constexpr int kNewYear = 1542;       // New Year greetings in town (no bugs, Aiya greets instead)
constexpr int kWentInTv = 3074;      // been into the TV world today (set on arrival, cleared overnight)

// The jobs on the board (field/script/f008.bf `keijiban`), with what the board itself says they
// pay and raise. `today` is the scheduler's day flag that the bus stop (f008 `basutei`) checks
// before it offers a shift; the desk jobs (f007 `check_mini_desk`) and the pub have none.
struct Job {
    const char* name;
    const char* pay;
    int8_t raises;            // stat index (0 Courage .. 4 Expression), -1 = the board doesn't say
    int16_t taken;            // set when you sign up at the board
    int16_t today;            // -1: any evening
    bool evening;
    int8_t needStat, needRank;  // what the board checks before signing you up
    int8_t fromMonth, fromDay;  // first day it's posted
    const char* days;         // the board's own "Work Days", for the "no shift today" note
    uint8_t weekdays;         // the same, bit 0 = Sunday (0 = any day)
    const char* where;
};

const Job kJobs[] = {
    {"Day-care assistant", "¥4,000+", -1, 2665, 1500, false, -1, 0, 4, 23, "Mon/Fri/Sat when it isn't raining",
     0b1100010, kBusStop},
    {"Hospital janitor", "¥5,000+", 0, 2664, 1480, true, 2, 3, 5, 25, "Wed/Thu/Fri evenings", 0b0111000, kBusStop},
    {"Tutor", "¥10,000", 4, 2662, 1494, true, 3, 5, 5, 25, "Tue/Thu/Sat evenings", 0b1010100, kBusStop},
    // The pub is the Shiroku store's door at night (f008 `item_shop` calls the game's own pub
    // function, which runs the job itself; no script checks the job flag).
    {"Shiroku Pub dishwasher", "¥1,500+", 2, 2080, -1, true, 3, 2, 5, 25, "any evening", 0,
     "Shiroku Pub (the Shiroku store at night), shopping district south side"},
    {"Envelope making", "¥1,000+", 2, 2661, -1, true, -1, 0, 4, 23, "any evening", 0, kDesk},
    {"Translation", "¥1,500+", 4, 2660, -1, true, 1, 2, 4, 23, "any evening", 0, kDesk},
    {"Origami cranes", "unpaid (volunteer)", 3, 2659, -1, true, 2, 2, 4, 23, "any evening", 0, kDesk},
};

constexpr uint16_t kShu = 23;  // the bus stop stops offering tutoring once Shu's link is maxed

struct Ctx {
    const Game::Snapshot& s;
    Calendar::Date d;
    bool day;      // after school, or the daytime of a day off
    bool evening;
    bool rain;

    bool F(int n) const { return s.Flag(n); }
    bool On(int m, int dd) const { return d.month == m && d.day == dd; }
    bool Span(int m1, int d1, int m2, int d2) const {
        int a = Calendar::ToIndex(m1, d1), b = Calendar::ToIndex(m2, d2);
        return a >= 0 && b >= 0 && s.day >= a && s.day <= b;
    }
    bool From(int m, int dd) const {
        int a = Calendar::ToIndex(m, dd);
        return a >= 0 && s.day >= a;
    }
    int Rank(uint16_t link) const {
        const Game::Link* e = s.FindLink(link);
        return e ? e->rank : 0;
    }
    int StatRank(int stat) const { return Data::StatRank(stat, s.U(Game::Yu).social[stat]); }
    bool StoryDay() const { return F(kStoryDay1205) || F(kStoryDay0213); }
};

std::string Raises(const Job& j) {
    return j.raises >= 0 ? std::string("Raises ") + Data::kStats[j.raises].name + "." : std::string();
}

void Jobs(const Ctx& c, std::vector<Item>& out) {
    if (!c.day && !c.evening) return;
    std::string notTaken;
    for (const auto& j : kJobs) {
        bool posted = c.From(j.fromMonth, j.fromDay);
        if (!c.F(j.taken)) {
            // Still on the board (it's taken down in March).
            if (!posted || c.Span(3, 1, 3, 31)) continue;
            if (!notTaken.empty()) notTaken += ", ";
            notTaken += j.name;
            if (j.needStat >= 0 && c.StatRank(j.needStat) < j.needRank) {
                char b[64];
                snprintf(b, sizeof b, " (needs %s %d)", Data::kStats[j.needStat].name, j.needRank);
                notTaken += b;
            }
            continue;
        }
        if (j.evening != c.evening) continue;
        bool shift = j.today < 0 || c.F(j.today);
        if (j.today >= 0 && !j.evening && c.StoryDay()) shift = false;  // the bus stop is closed by day
        if (j.taken == 2662 && c.Rank(kShu) >= 10) shift = false;
        char what[96];
        snprintf(what, sizeof what, "%s (%s)", j.name, j.pay);
        if (shift) {
            out.push_back({Data::Tier::ClubsJobs, 100, what, j.where, Raises(j)});
        } else if (j.weekdays & (1 << c.d.weekday)) {
            // A work day by the board, but the game has no shift today: say why, when we know.
            std::string why = j.taken == 2665 && c.rain ? "It's raining, so the day-care is closed."
                              : j.taken == 2662 && c.Rank(kShu) >= 10
                                  ? "No more tutoring: Shu's link is maxed."
                                  : "No shift today (the game's schedule has none).";
            out.push_back({Data::Tier::ClubsJobs, 190, std::string(j.name) + " — not today", j.where,
                           why + " Works " + j.days + "."});
        }
    }
    if (!notTaken.empty())
        out.push_back({Data::Tier::ClubsJobs, 180, "Jobs you haven't signed up for", kBoard, notTaken + "."});
}

// Aiya (f008 `tyuuka`): a different ¥800 special each weekday, or the Mega Beef Bowl challenge
// on rainy days and winter Fridays; closed on rainy and winter-Friday evenings, and "too late
// to eat here" once you've been into the TV world that day (flag 3074).
void Aiya(const Ctx& c, std::vector<Item>& out) {
    if (!c.day && !c.evening) return;
    bool winterFriday = c.d.weekday == 5 && c.Span(1, 1, 2, 29);
    bool closed = c.Span(4, 11, 4, 12) || c.On(4, 17) || (c.evening && (c.rain || winterFriday)) || c.StoryDay() ||
                  c.F(kNewYear) || c.F(kWentInTv);
    if (closed) return;
    if (winterFriday || c.rain) {
        out.push_back({Data::Tier::Other, 10, winterFriday ? "Aiya's winter challenge: Mega Beef Bowl (¥3,000)"
                                                           : "Aiya's rainy-day challenge: Mega Beef Bowl (¥3,000)",
                       kAiya, "Free if you finish it. Raises several social stats."});
        return;
    }
    int w = c.d.weekday;
    const char* dish = (w == 1 || w == 2) ? "Mabo Tofu Combo" : (w == 3 || w == 4) ? "Chow Mein set" : "Aiya Steak Bowl";
    int stat = (w == 1 || w == 2) ? 0 : (w == 3 || w == 4) ? 2 : 3;
    char what[80];
    snprintf(what, sizeof what, "Aiya's special: %s (¥800)", dish);
    out.push_back({Data::Tier::Other, 20, what, kAiya, std::string("Raises ") + Data::kStats[stat].name + "."});
}

void Other(const Ctx& c, std::vector<Item>& out) {
    if (c.evening) {
        out.push_back({Data::Tier::Other, 50, "Study at your desk", "Your room", "Raises Knowledge."});
        out.push_back({Data::Tier::Other, 51, "Read a book", "Your room", ""});
    }
    if (!c.day) return;
    // Bugs at the shrine (f008 `musi_ap_shrine`): once you can catch them, once a day, never in
    // the rain or from 11/28 to 2/14.
    if (c.F(3347) && !c.rain && !c.Span(11, 28, 2, 14) && !c.F(kNewYear) && !c.StoryDay() && c.s.counters[215] == 0)
        out.push_back({Data::Tier::Other, 40, "Catch bugs", "Tatsuhime Shrine", "Once a day; not in the rain."});
    // The garden at home (f007 `check_farmfield`) once it's been set up.
    if (c.F(3280) && !c.On(3, 20)) out.push_back({Data::Tier::Other, 41, "Tend the garden", "Dojima residence", ""});
    out.push_back({Data::Tier::Other, 60, "Fishing", "Samegawa riverbank", ""});
    out.push_back({Data::Tier::Other, 61, "Velvet Room (fuse, requests)", "Shopping district", ""});
}

}  // namespace

std::vector<Item> Now(const Game::Snapshot& s) {
    Ctx c{s, Calendar::FromIndex(s.day), s.time == 3 || s.time == 4, s.time == 5, s.Raining()};
    std::vector<Item> out;
    Jobs(c, out);
    Aiya(c, out);
    Other(c, out);
    return out;
}

const std::vector<FlagUse>& Flags() {
    static std::vector<FlagUse> v;
    if (!v.empty()) return v;
    for (const auto& j : kJobs) {
        v.push_back({j.taken, j.name, "signed up at the job board"});
        if (j.today >= 0) v.push_back({j.today, j.name, "a shift today (the scheduler's day flag)"});
    }
    v.push_back({kStoryDay1205, "Town", "closed by day (12/5 story day)"});
    v.push_back({kStoryDay0213, "Town", "closed by day (2/13 story day)"});
    v.push_back({kNewYear, "Town", "New Year greetings (no bugs; Aiya greets instead of serving)"});
    v.push_back({kWentInTv, "Aiya", "too late to eat here (been into the TV world today)"});
    v.push_back({3347, "Bug catching", "unlocked"});
    v.push_back({3280, "Garden", "set up"});
    return v;
}

}  // namespace Activities
