#pragma once
// What can be done in the current time slot: jobs, clubs and activities by weekday. Who is
// around for social links comes from Features/WhoIsAround (the game's own schedule flags).

#include <cstdint>

namespace Data {

enum class Slot : uint8_t { Day, Night };

enum class Tier : uint8_t {
    LastChance = 0,  // missable soon (deadlines, cutoffs)
    Romance = 1,     // dating / romance-route hangouts
    Links = 2,       // social links, hangouts, bike trips
    ClubsJobs = 3,
    Other = 4,
};

// Evening jobs and activities by weekday (community schedules; bit 0 = Sunday).
struct Activity {
    const char* what;
    const char* where;
    Slot slot;
    uint8_t weekdays;   // bitmask
    Tier tier;
    int16_t needStat;   // social stat index needed (-1 none)
    uint8_t needRank;
    uint16_t link;      // linked social link (0 = none)
    int16_t fromMonth, fromDay;
};

inline constexpr Activity kActivities[] = {
    {"Day-care job (Understanding, ¥)", "Day-care, shopping district", Slot::Day, 0b1100010, Tier::ClubsJobs, -1, 0, 29, 4, 23},
    {"Hospital janitor (Courage)", "Inaba Hospital", Slot::Night, 0b0111000, Tier::ClubsJobs, 2, 3, 9, 5, 25},
    {"Tutoring (Knowledge, Expression, ¥)", "Shu's house", Slot::Night, 0b1010100, Tier::ClubsJobs, 3, 5, 23, 5, 25},
    {"Envelope folding (Diligence)", "At home", Slot::Night, 0b1111111, Tier::ClubsJobs, -1, 0, 0, 4, 23},
    {"Translation (Expression)", "At home", Slot::Night, 0b1111111, Tier::ClubsJobs, -1, 0, 0, 4, 23},
    {"Study at your desk (Knowledge)", "Your room", Slot::Night, 0b1111111, Tier::Other, -1, 0, 0, 0, 0},
    {"Read a book", "Your room", Slot::Night, 0b1111111, Tier::Other, -1, 0, 0, 0, 0},
    {"Fishing", "Samegawa riverbank", Slot::Day, 0b1111111, Tier::Other, -1, 0, 0, 0, 0},
    {"Velvet Room (fuse, requests)", "Shopping district", Slot::Day, 0b1111111, Tier::Other, -1, 0, 0, 0, 0},
    {"Garden / bug hunting", "Your room / shrine & riverbank", Slot::Day, 0b1111111, Tier::Other, -1, 0, 0, 0, 0},
    {"Aiya's Rainy Day Challenge (Courage; rainy days only)", "Aiya Chinese Diner", Slot::Day, 0b1111111, Tier::Other, -1, 0, 0, 0, 0},
};

}  // namespace Data
