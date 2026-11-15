#include "Features/WhoIsAround.h"

#include "Engine/Calendar.h"

namespace Around {

namespace {

// Flag sections: S1 starts at 0x400, S2 at 0xC00 (absolute flag ids).
constexpr int S1 = 0x400, S2 = 0xC00;

constexpr const char* kNorth = "Shopping district, north side";
constexpr const char* kSouth = "Shopping district, south side";
constexpr const char* kBike = "Shopping district, south side, by the scooters";
constexpr const char* kHome = "Dojima residence";

struct Ctx {
    const Game::Snapshot& s;
    bool evening;   // time slot 5
    bool holiday;   // Sundays and holidays: the daytime slot is "Afternoon" (3); 4/12 too
    int month, day;

    bool F(int abs) const { return s.Flag(abs); }
    bool A(int n) const { return s.Flag(S1 + n); }  // S1-relative
    bool B(int n) const { return s.Flag(S2 + n); }  // S2-relative
    int L(uint16_t id) const {
        const Game::Link* e = s.FindLink(id);
        return e ? e->rank : 0;
    }
    bool Started(uint16_t a, uint16_t b = 0) const { return L(a) > 0 || (b && L(b) > 0); }
    bool Maxed(uint16_t a, uint16_t b = 0) const { return L(a) >= 10 || (b && L(b) >= 10); }
    bool On(int m, int d) const { return month == m && day == d; }
    bool Span(int m1, int d1, int m2, int d2) const {
        int a = Calendar::ToIndex(m1, d1), b = Calendar::ToIndex(m2, d2);
        return a >= 0 && b >= 0 && s.day >= a && s.day <= b;
    }
    bool Ready(uint16_t id) const {
        const Game::Link* e = s.FindLink(id);
        return e && e->rank && e->rank < 10 && e->points >= Game::LinkThreshold(e->id, e->rank);
    }
    bool Day() const { return !evening; }
    bool Blocked() const { return F(87); }       // nobody is around
    bool SetA() const { return A(1569); }        // the alternate set of day flags is in use
};

// The main friends: two sets of day flags (S1+1569 picks set A), an evening hangout with a
// block flag, a "busy after a Marie outing" flag, and a scooter trip flag.
struct Friend {
    const char* who;
    uint16_t link, link2;
    int16_t dayA, dayB, night, block, busy, bike;
    bool bikeSpot;  // with a scooter trip on, Find-a-Friend finds them by the scooters
    const char *weekday, *holiday, *evening;
};

const Friend kFriends[] = {
    {"Yosuke", 7, 0, 432, 454, 1152, 883, 1216, 626, true, "Learning Building 2F (school)", "Junes", kNorth},
    {"Chie", 11, 12, 434, 458, 1153, 884, 1217, 628, false, "School rooftop", kSouth, kNorth},
    {"Yukiko", 5, 6, 435, 452, 1154, 885, 1218, 625, true, "Learning Building 1F (school)", kSouth, kSouth},
    {"Kanji", 10, 0, 433, 457, 1155, 886, 1219, 627, false, "Practice Building 1F (school)", kNorth, kNorth},
    {"Rise", 3, 4, 436, 451, 1156, 887, 1220, 624, true, "Learning Building 1F (school)", kSouth, kSouth},
    {"Naoto", 14, 15, 461, 462, 1157, 889, 1221, 629, true, "Learning Building 1F (school)", "Samegawa flood plain", kSouth},
};

void Friends(const Ctx& c, std::vector<Person>& out) {
    for (const auto& f : kFriends) {
        bool away =
            (c.Day() && !c.holiday && ((!c.A(f.dayA) && c.SetA()) || (!c.SetA() && !c.A(f.dayB)))) ||
            (c.Day() && c.holiday && !c.A(f.dayA) && !c.A(f.dayB)) || c.Maxed(f.link, f.link2) ||
            (c.evening && (!c.A(f.night) || c.B(f.block) || !c.Started(f.link, f.link2))) || c.Blocked() ||
            c.A(f.busy);
        if (f.link == 14 && c.SetA() && c.Span(11, 6, 11, 20)) away = true;  // Naoto
        bool bike = c.Day() && c.A(f.bike) && !c.Blocked() && !c.SetA();
        // The hangout and the scooter trip are separate offers, as in Find-a-Friend. Its places
        // check a holiday before the evening (which only matters on 4/12).
        if (!away) {
            const char* where = bike && f.bikeSpot ? kBike : c.holiday ? f.holiday : c.evening ? f.evening : f.weekday;
            out.push_back({f.who, f.link, false, c.evening, where});
        }
        if (bike) out.push_back({f.who, f.link, true, false, kBike});
    }
}

void Others(const Ctx& c, std::vector<Person>& out) {
    bool ev = c.evening, day = c.Day();
    // People with Golden's evening hangouts: in the evening that's what they offer.
    auto add = [&](bool unavailable, const char* who, uint16_t link, const char* where) {
        if (!unavailable) out.push_back({who, link, false, ev, where});
    };
    // People whose own link events happen in the evening (or who are only around by day).
    auto addLink = [&](bool unavailable, const char* who, uint16_t link, const char* where) {
        if (!unavailable) out.push_back({who, link, false, false, where});
    };

    // Ai needs the Strength link at rank 4 before hers can start; flag 2512 keeps her away on
    // the day it does.
    add((day && !c.A(468)) || c.Maxed(21, 22) || (ev && (!c.A(1163) || c.B(894) || !c.Started(21, 22))) ||
            (!c.Started(21, 22) && c.L(16) < 4 && c.L(17) < 4) || c.F(2512),
        "Ai", 21, ev ? kSouth : "Learning Building 1F (school)");
    // Yumi and Ayane: one or the other, chosen by the club.
    add((day && !c.A(475)) || c.Maxed(25, 26) || c.Started(27, 28) ||
            (ev && (!c.A(1160) || c.B(896) || !c.Started(25, 26))),
        "Yumi", 25, ev ? kSouth : c.Started(25, 26) ? "Learning Building 2F (school)" : "Practice Building 1F (school)");
    add((day && !c.A(475)) || c.Maxed(27, 28) || c.Started(25, 26) ||
            (ev && (!c.A(1161) || c.B(896) || !c.Started(27, 28))),
        "Ayane", 27, ev ? kSouth : c.Started(27, 28) ? "Learning Building 2F (school)" : "Practice Building 1F (school)");
    // Kou and Daisuke: one or the other; no evening hangouts on 10/3 (day set B) or 10/5 to 1/1.
    bool strengthEveningOff = ev && ((c.On(10, 3) && !c.SetA()) || c.Span(10, 5, 1, 1));
    add((day && !c.A(463)) || c.L(16) >= 10 || c.L(17) > 0 || (ev && (!c.A(1158) || c.L(16) <= 0 || c.B(895))) ||
            strengthEveningOff,
        "Kou", 16, ev ? kNorth : c.Started(16) ? "Learning Building 2F (school)" : "Learning Building 1F (school)");
    add((day && !c.A(463)) || c.L(17) >= 10 || c.L(16) > 0 || (ev && (!c.A(1159) || c.L(17) <= 0 || c.B(895))) ||
            strengthEveningOff,
        "Daisuke", 17, ev ? kNorth : c.Started(17) ? "Learning Building 2F (school)" : "Learning Building 1F (school)");
    add((day && !c.A(465)) || c.L(18) >= 10 || (ev && (!c.A(1162) || c.B(891) || c.L(18) <= 0)), "Naoki", 18,
        (ev || c.holiday || c.Started(18)) ? kNorth : "Learning Building 1F (school)");
    // Hisano's link needs the Devil link (Sayoko) at rank 4.
    addLink(!c.A(466) || c.L(19) >= 10 || ev || (c.L(19) <= 0 && c.L(9) < 4), "Hisano", 19, "Samegawa riverbank");
    addLink(!c.A(449) || c.L(2) >= 10 || !ev, "Nanako", 2, kHome);
    addLink(!c.A(455) || c.SetA() || c.L(8) >= 10 || !ev, "Dojima", 8, kHome);
    addLink(!c.A(456) || !c.A(1640) || !ev || c.L(9) >= 10, "Sayoko", 9, kSouth);
    addLink(!c.A(470) || !c.A(1638) || c.L(23) >= 10 || !ev, "Shu", 23, kSouth);
    addLink(!c.A(1641) || !c.A(476) || c.L(29) >= 10 || ev, "Eri", 29, kSouth);
    // Marie. Find-a-Friend also checks a script function here whose meaning isn't known; this
    // treats it as "her link has started". Before 7/23 her rank 4 needs an outing first.
    addLink(!c.A(712) || c.Maxed(33, 34) || ev || c.Blocked() || (!c.Started(33, 34) && c.A(527)) ||
                (c.Span(4, 11, 7, 23) && (c.Ready(33) || c.Ready(34)) && (c.L(33) == 4 || c.L(34) == 4)),
            "Marie", 33, kSouth);
    // Adachi: up to rank 6. A hangout by day earns points; once one would rank up, ranks 1 and 5
    // still happen by day and ranks 2 to 4 in the evening.
    {
        int r = c.L(31);
        bool ready = c.Ready(31);
        addLink(!c.A(692) || c.A(702) || c.SetA() || r >= 6 || (!ready && ev) ||
                    (ready && ((ev && (r == 1 || r == 5)) || (day && r >= 2 && r <= 4))),
                "Adachi", 31, ev ? kSouth : "Junes");
    }
    // The fox ranks up through its shrine requests, not points: there's something to do at the
    // shrine when its next request hasn't been taken yet, or the current one is done and the
    // rank-up hasn't happened. Ranks 1 to 9, with that rank's request (quest number) each.
    if (day && c.L(13) >= 1 && c.L(13) < 10) {
        static const int kFoxQuest[] = {6, 13, 20, 26, 30, 36, 41, 42, 48};
        int r = c.L(13);
        bool done = false;
        for (int i = 0; i < 9; ++i)
            if (c.F(319 + kFoxQuest[i]) && !c.A(1696 + i)) done = true;
        bool notTaken = !c.F(255 + kFoxQuest[r - 1]);
        if (done || notTaken) {
            Person p{"The fox", 13, false, false, "Tatsuhime Shrine (shopping district)"};
            p.ready = done ? 1 : 0;
            p.note = done ? "Its request is done: visit to rank up." : "Visit the shrine for its next request.";
            out.push_back(p);
        }
    }
    // Teddie: scooter trips only.
    if (day && c.A(630) && !c.SetA() && !c.Blocked()) out.push_back({"Teddie", 24, true, false, kBike});
}

}  // namespace

std::vector<Person> Now(const Game::Snapshot& s) {
    auto d = Calendar::FromIndex(s.day);
    Ctx c{s, s.time == 5, s.time == 3 || (d.month == 4 && d.day == 12), d.month, d.day};
    std::vector<Person> out;
    Friends(c, out);
    Others(c, out);
    return out;
}

const std::vector<FlagUse>& Flags() {
    static std::vector<FlagUse> v;
    if (!v.empty()) return v;
    for (const auto& f : kFriends) {
        v.push_back({int16_t(S1 + f.dayA), f.link, f.who, "around today (day set A)"});
        v.push_back({int16_t(S1 + f.dayB), f.link, f.who, "around today (day set B)"});
        v.push_back({int16_t(S1 + f.night), f.link, f.who, "evening hangout on"});
        v.push_back({int16_t(S2 + f.block), f.link, f.who, "evening hangout blocked"});
        v.push_back({int16_t(S1 + f.busy), f.link, f.who, "busy (after an outing with Marie)"});
        v.push_back({int16_t(S1 + f.bike), f.link, f.who, "scooter trip available"});
    }
    const FlagUse more[] = {
        {S1 + 1569, 0, "Daytime", "day set A in use (instead of set B)"},
        {87, 0, "Everyone", "nobody is around"},
        {S1 + 468, 21, "Ai", "around today"},              {S1 + 1163, 21, "Ai", "evening hangout on"},
        {S2 + 894, 21, "Ai", "evening hangout blocked"},   {2512, 21, "Ai", "away (the day her link starts)"},
        {S1 + 475, 25, "Yumi / Ayane", "around today"},    {S1 + 1160, 25, "Yumi", "evening hangout on"},
        {S1 + 1161, 27, "Ayane", "evening hangout on"},    {S2 + 896, 25, "Yumi / Ayane", "evening hangout blocked"},
        {S1 + 463, 16, "Kou / Daisuke", "around today"},   {S1 + 1158, 16, "Kou", "evening hangout on"},
        {S1 + 1159, 17, "Daisuke", "evening hangout on"},  {S2 + 895, 16, "Kou / Daisuke", "evening hangout blocked"},
        {S1 + 465, 18, "Naoki", "around today"},           {S1 + 1162, 18, "Naoki", "evening hangout on"},
        {S2 + 891, 18, "Naoki", "evening hangout blocked"}, {S1 + 466, 19, "Hisano", "around today"},
        {S1 + 449, 2, "Nanako", "around this evening"},    {S1 + 455, 8, "Dojima", "home this evening"},
        {S1 + 456, 9, "Sayoko", "around this evening"},    {S1 + 1640, 9, "Sayoko", "around this evening (2)"},
        {S1 + 470, 23, "Shu", "around this evening"},      {S1 + 1638, 23, "Shu", "around this evening (2)"},
        {S1 + 476, 29, "Eri", "around today"},             {S1 + 1641, 29, "Eri", "around today (2)"},
        {S1 + 712, 33, "Marie", "around today"},           {S1 + 527, 33, "Marie", "away"},
        {S1 + 630, 24, "Teddie", "scooter trip available"},
        {S1 + 692, 31, "Adachi", "can be found (Find-a-Friend check)"},
        {S1 + 702, 31, "Adachi", "can't be found (Find-a-Friend check)"},
    };
    for (const auto& m : more) v.push_back(m);
    for (int i = 0; i < 9; ++i) v.push_back({int16_t(S1 + 1696 + i), 13, "The fox", "ranked up after a shrine request"});
    return v;
}

}  // namespace Around
