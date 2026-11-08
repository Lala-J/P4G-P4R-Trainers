#pragma once
// What can be done in the current time slot. Availability comes from the game's own "who is
// around today" flags, which the scheduler sets each morning (the same flags the Find-a-Friend
// mod reads); places are where the game puts each person on those days.

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

struct Availability {
    uint16_t link;      // base commu id (0 = not a link)
    const char* who;
    Slot slot;
    int16_t flag;       // the game's availability flag (absolute id)
    const char* where;
};

inline constexpr Availability kAvailability[] = {
    {7, "Yosuke", Slot::Day, 1478, "School rooftop / Junes food court on days off"},
    {11, "Chie", Slot::Day, 1482, "School rooftop on weekdays; Daidara area on days off"},
    {5, "Yukiko", Slot::Day, 1476, "1F classroom hallway; bookstore on Sundays"},
    {10, "Kanji", Slot::Day, 1481, "School front gate / Tatsumi Textiles"},
    {3, "Rise", Slot::Day, 1460, "1F hallway; Marukyu Tofu on days off"},
    {2, "Nanako", Slot::Day, 1473, "Dojima residence living room"},
    {8, "Dojima", Slot::Day, 1479, "Dojima residence (evenings he's home)"},
    {16, "Kou / Daisuke (Strength)", Slot::Day, 1487, "Gym (basketball) or the field (soccer) after school"},
    {18, "Naoki", Slot::Day, 1489, "Konishi Liquors (shopping district)"},
    {19, "Hisano", Slot::Day, 1490, "Samegawa riverbank"},
    {21, "Ai", Slot::Day, 1492, "Classroom 2-1, 2F"},
    {25, "Yumi / Ayane (Sun)", Slot::Day, 1499, "Drama room / music room, 3F"},
    {14, "Naoto", Slot::Day, 1486, "School 1F / northern shopping district"},
    {33, "Marie", Slot::Day, 1736, "Velvet Room door (shopping district)"},
    {7, "Yosuke", Slot::Night, 2176, "Central shopping district at night (hangout)"},
    {11, "Chie", Slot::Night, 2177, "Central shopping district at night (hangout)"},
    {5, "Yukiko", Slot::Night, 2178, "Central shopping district at night (hangout)"},
    {10, "Kanji", Slot::Night, 2179, "Central shopping district at night (hangout)"},
    {3, "Rise", Slot::Night, 2180, "Central shopping district at night (hangout)"},
    {14, "Naoto", Slot::Night, 2181, "Central shopping district at night (hangout)"},
    {16, "Kou", Slot::Night, 2182, "Central shopping district at night (hangout)"},
    {17, "Daisuke", Slot::Night, 2183, "Central shopping district at night (hangout)"},
    {25, "Yumi", Slot::Night, 2184, "Central shopping district at night (hangout)"},
    {27, "Ayane", Slot::Night, 2185, "Central shopping district at night (hangout)"},
    {18, "Naoki", Slot::Night, 2186, "Central shopping district at night (hangout)"},
    {21, "Ai", Slot::Night, 2187, "Central shopping district at night (hangout)"},
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
