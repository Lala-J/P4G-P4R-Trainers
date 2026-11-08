#pragma once
// Social link prerequisites and story constraints used by the Rules engine
// (docs/research/mechanics.md, docs/research/romance_portraits_activities.md).

#include <cstdint>

namespace Data {

struct LinkRule {
    uint16_t id;
    uint8_t fromMonth, fromDay;   // earliest start (0 = no date gate)
    uint8_t untilMonth, untilDay; // last day rank-ups are possible (0 = none)
    uint8_t stat[5];              // minimum social stat RANK needed to start (0 = none)
    uint16_t needLink;            // another link that must reach needRank first (0 = none)
    uint8_t needRank;
    uint8_t romance[2];           // ranks whose scene holds a romance choice (0 = none)
    const char* note;             // shown with the warnings
};

inline constexpr LinkRule kLinkRules[] = {
    {2, 0, 0, 10, 31, {}, 0, 0, {}, "Nanako's link can't advance after 10/31 until the story allows it."},
    {3, 7, 23, 0, 0, {}, 0, 0, {8, 0}, "Rise's link starts 7/23."},
    {5, 5, 17, 0, 0, {}, 0, 0, {9, 0}, nullptr},
    {9, 0, 0, 0, 0, {0, 0, 3, 0, 0}, 0, 0, {}, "Needs Diligence rank 3 (hospital job)."},
    {11, 4, 18, 0, 0, {}, 0, 0, {9, 0}, nullptr},
    {14, 10, 21, 0, 0, {5, 5, 0, 0, 0}, 0, 0, {6, 8},
     "Needs Courage and Knowledge rank 5 and starts 10/21. Romance needs the right answers at ranks 6 and 8."},
    {16, 0, 0, 0, 0, {}, 0, 0, {}, "Kou and Daisuke are either/or: joining one sports club locks out the other."},
    {17, 0, 0, 0, 0, {}, 0, 0, {}, "Kou and Daisuke are either/or: joining one sports club locks out the other."},
    {19, 0, 0, 0, 0, {}, 9, 4, {}, "Hisano needs Sayoko's link at rank 4."},
    {20, 0, 0, 0, 0, {0, 3, 0, 0, 0}, 0, 0, {}, "Margaret needs Knowledge rank 3."},
    {21, 0, 0, 0, 0, {3, 0, 0, 0, 0}, 16, 4, {6, 10},
     "Ai needs Courage rank 3 and the Strength link at rank 4. Choosing to date her at rank 6 is temporary and "
     "closes her real route; the real romance is at rank 10."},
    {23, 0, 0, 0, 0, {0, 0, 0, 5, 0}, 0, 0, {}, "Shu needs Understanding rank 5 (tutoring job)."},
    {25, 0, 0, 0, 0, {}, 0, 0, {10, 0}, "Yumi and Ayane are either/or: one culture club locks out the other."},
    {27, 0, 0, 0, 0, {}, 0, 0, {8, 0}, "Yumi and Ayane are either/or: one culture club locks out the other."},
    {29, 0, 0, 0, 0, {0, 0, 0, 2, 0}, 0, 0, {}, "Eri needs Understanding rank 2 (day-care job)."},
    {33, 0, 0, 12, 22, {}, 0, 0, {9, 0},
     "Marie must reach rank 10 before Magatsu Inaba is cleared (12/22 is the safe last day). Rank 4 to 5 needs "
     "Rise on the team."},
};

inline const LinkRule* Rule(uint16_t id) {
    for (const auto& r : kLinkRules)
        if (r.id == id) return &r;
    return nullptr;
}

// Mined rank-up effects (tools/datamine/gen_link_effects.py)
enum class LinkFx : uint8_t { BitOn, BitOff, LearnSkill, AcceptQuest, CompleteQuest, AddYen };
struct LinkEffect {
    uint16_t link;
    uint8_t rank;
    LinkFx kind;
    int32_t a, b;
};
inline constexpr LinkEffect kLinkEffects[] = {
#include "Data/LinkEffects.inc"
};
struct LinkRank {
    uint16_t link;
    uint8_t rank;
};
inline constexpr LinkRank kLinkRanksKnown[] = {
#include "Data/LinkRanksKnown.inc"
};

}  // namespace Data
