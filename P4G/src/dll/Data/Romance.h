#pragma once
// Romance routes. Dates, ranks and the choices that matter come from community guides
// (docs/research/romance_portraits_activities.md) cross-checked with the game's own scripts: the
// "become lovers" procedures (koibito-ka) and romance flags were mined from the rank events
// (docs/data/link_events.md). Wording of choices is paraphrased, not quoted.

#include <cstdint>

namespace Data {

struct RomanceStep {
    uint8_t rank;          // 0 = before the link starts; 11 = after maxing the link
    int16_t byMonth, byDay;  // a date limit for this step (0 = none)
    const char* text;
    const char* avoid;     // choices to avoid here (nullptr = none)
};

struct RomanceRoute {
    const char* name;
    uint16_t link;         // base commu id
    uint16_t loverRow;     // commu id once dating
    int16_t loverFlag;     // set when the relationship becomes official
    uint8_t bustup;        // bustup character id (portraits)
    uint8_t bustupOutfit;
    uint16_t metFlag;      // party-join flag that also means "met" (0xFFFF = none)
    uint8_t loverRank;     // the rank whose scene makes it official
    const RomanceStep* steps;
    int stepCount;
    int16_t metMonth, metDay;  // the story introduces them by this date (0 = only via the link)
};

inline constexpr RomanceStep kChie[] = {
    {0, 0, 0, "Starts on 4/18 automatically. Find her on the school rooftop on weekdays, or around "
              "Daidara / the shopping district on days off (not in rain).", nullptr},
    {9, 0, 0, "Rank 9: when she asks what she means to you, choose the answer that asks her to be "
              "your girlfriend.", "\"I'm counting on you\" keeps you friends."},
    {10, 0, 0, "Rank 10 plays the romance version of her final scene.", nullptr},
    {11, 12, 22, "Defeat the final December dungeon by 12/22 to keep your Christmas Eve date.", nullptr},
};
inline constexpr RomanceStep kYukiko[] = {
    {0, 0, 0, "Starts 5/17. Classroom 1F on weekdays (Mon-Thu) or the bookstore on Sundays; not in rain.", nullptr},
    {9, 0, 0, "Rank 9: tell her you really like her.", "Calling her a classmate or a friend keeps you friends."},
    {11, 12, 22, "Defeat the final December dungeon by 12/22 to keep your Christmas Eve date.", nullptr},
};
inline constexpr RomanceStep kRise[] = {
    {0, 0, 0, "Starts 7/23. 1F hallway on Fri-Sun, or the tofu shop on days off; not in rain.", nullptr},
    {8, 0, 0, "Rank 8: when the moment comes, hold her.", "\"Stand there\" keeps you friends."},
    {11, 12, 22, "Defeat the final December dungeon by 12/22 to keep your Christmas Eve date.", nullptr},
};
inline constexpr RomanceStep kNaoto[] = {
    {0, 10, 21, "Needs Knowledge and Courage rank 5. From 10/21, talk to the man in black in the north "
                "shopping district, then to her.", nullptr},
    {5, 0, 0, "Rank 5: don't leave her on her own.", "Telling her she's on her own reverses the link."},
    {6, 0, 0, "Rank 6: tell her you're glad she's a girl.", "\"Your gender doesn't matter\" closes the romance."},
    {8, 0, 0, "Rank 8: choose to protect her, then tell her it's because you love her (this only appears if "
              "both earlier answers were right).", "Running with her or fighting back closes the romance."},
    {11, 12, 22, "Defeat the final December dungeon by 12/22 to keep your Christmas Eve date.", nullptr},
};
inline constexpr RomanceStep kMarie[] = {
    {0, 0, 0, "From 4/18: in the Velvet Room, check on the dwellers and agree to Marie's request.", nullptr},
    {4, 0, 0, "Rank 5 needs Rise on the team (from late July).", nullptr},
    {9, 0, 0, "Rank 9: tell her you love her, then hug her.", "Not hugging her keeps you friends."},
    {10, 12, 22, "Reach rank 10 before the final December dungeon is cleared (12/22 to be safe). Ranks 6 and 10 "
                 "each need one extra hangout.", nullptr},
    {11, 0, 0, "After rank 10 she disappears; agree to look for her on 1/2, and the ski trip in February "
               "leads to her.", nullptr},
};
inline constexpr RomanceStep kAi[] = {
    {0, 0, 0, "From 4/27, talk to her after school; then she needs the Strength link at rank 4 and Courage "
              "rank 3.", nullptr},
    {3, 0, 0, "Rank 3: don't call her cruel.", "That answer reverses the link."},
    {6, 0, 0, "Rank 6: when she asks you out, stay silent and question whether she means it — staying "
              "friends here is what keeps the real romance open.",
     "Accepting here starts a temporary relationship she ends at rank 9."},
    {10, 0, 0, "Rank 10: accept her feelings.", nullptr},
};
inline constexpr RomanceStep kYumi[] = {
    {0, 4, 25, "Join the Drama club on 4/25 (this rules out Ayane). Mon/Tue/Thu after school.", nullptr},
    {10, 0, 0, "Rank 10: hug her.", "Leaving keeps you friends."},
};
inline constexpr RomanceStep kAyane[] = {
    {0, 4, 25, "Join the Music club (band) on 4/25 (this rules out Yumi). Mon/Tue/Thu after school.", nullptr},
    {8, 0, 0, "Rank 8: tell her how you feel.", "Changing the subject keeps you friends."},
};

#define ROUTE(n, link, row, flag, bu, bo, met, lr, steps, mm, md)     {n, link, row, flag, bu, bo, met, lr, steps, int(sizeof(steps) / sizeof(steps[0])), mm, md}
// Introduction dates: when the story first puts them in front of you (community calendar).
inline constexpr RomanceRoute kRoutes[] = {
    ROUTE("Chie", 11, 12, 2384, 3, 1, 49, 9, kChie, 4, 11),
    ROUTE("Yukiko", 5, 6, 2385, 4, 1, 50, 9, kYukiko, 4, 11),
    ROUTE("Rise", 3, 4, 2386, 5, 1, 52, 8, kRise, 6, 24),
    ROUTE("Naoto", 14, 15, 2621, 7, 1, 54, 8, kNaoto, 6, 24),
    ROUTE("Marie", 33, 34, 2383, 60, 0, 0xFFFF, 9, kMarie, 4, 18),
    ROUTE("Ai", 21, 22, 2388, 28, 1, 0xFFFF, 10, kAi, 4, 27),
    ROUTE("Yumi", 25, 26, 2389, 30, 1, 0xFFFF, 10, kYumi, 0, 0),
    ROUTE("Ayane", 27, 28, 2390, 31, 1, 0xFFFF, 8, kAyane, 0, 0),
};
#undef ROUTE
inline constexpr int kRouteCount = int(sizeof(kRoutes) / sizeof(kRoutes[0]));

// Shown once a romance is official, in order (spoiler-hidden per step).
inline constexpr const char* kAfterSteps[] = {
    "7/25 and 8/21: summer events where saying who you're looking forward to seeing adds points.",
    "12/22: last day to clear the December dungeon if you want Christmas Eve.",
    "12/23: each girlfriend calls. Decline until the one you want asks, then accept.",
    "12/24: Christmas Eve date and a gift. (Marie has no Christmas date.)",
    "1/1: New Year's shrine visit.",
    "2/14: Valentine's Day — with more than one girlfriend, you pick one and the others are hurt.",
    "3/20: farewells before you leave town.",
};

}  // namespace Data
