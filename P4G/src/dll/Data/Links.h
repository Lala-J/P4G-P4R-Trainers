#pragma once
// Social link catalogue, keyed by the game's commu id (the value stored in the link array).
// Labels are our own; arcana names are read from the game. Facts: docs/research/mechanics.md.

#include <cstdint>

#include "Engine/Calendar.h"

namespace Data {

enum class LinkKind : uint8_t {
    Normal,
    Story,      // advances only through the story (Fool, Star, Judgement, Jester 7+, Hunger)
    RomanceRow, // the record a romanceable link switches to once you're dating
    Unused,
};

struct LinkInfo {
    uint16_t id;
    const char* person;   // who the link is with
    LinkKind kind;
    uint16_t romanceRow;  // commu id used once dating (0 = not romanceable)
    uint16_t baseRow;     // for a romance row: the normal row
    uint16_t exclusive;   // the other link of an either/or pair (0 = none)
    int16_t revealDay;    // day index from which the name is shown before the link starts;
                          // -1 = only once the link has started (avoids spoilers)
};

// Day indices (Calendar::ToIndex) are written out so the table stays constexpr.
inline constexpr LinkInfo kLinks[] = {
    {0, "", LinkKind::Unused, 0, 0, 0, -1},
    {1, "Investigation Team", LinkKind::Story, 0, 0, 0, 0},
    {2, "Nanako Dojima", LinkKind::Normal, 0, 0, 0, 0},
    {3, "Rise Kujikawa", LinkKind::Normal, 4, 0, 0, 80},       // 6/20
    {4, "Rise Kujikawa", LinkKind::RomanceRow, 0, 3, 0, -1},
    {5, "Yukiko Amagi", LinkKind::Normal, 6, 0, 0, 0},
    {6, "Yukiko Amagi", LinkKind::RomanceRow, 0, 5, 0, -1},
    {7, "Yosuke Hanamura", LinkKind::Normal, 0, 0, 0, 0},
    {8, "Ryotaro Dojima", LinkKind::Normal, 0, 0, 0, 0},
    {9, "Sayoko Uehara", LinkKind::Normal, 0, 0, 0, 0},
    {10, "Kanji Tatsumi", LinkKind::Normal, 0, 0, 0, 42},      // 5/13
    {11, "Chie Satonaka", LinkKind::Normal, 12, 0, 0, 0},
    {12, "Chie Satonaka", LinkKind::RomanceRow, 0, 11, 0, -1},
    {13, "The Fox", LinkKind::Normal, 0, 0, 0, 0},
    {14, "Naoto Shirogane", LinkKind::Normal, 15, 0, 0, 84},   // 6/24
    {15, "Naoto Shirogane", LinkKind::RomanceRow, 0, 14, 0, -1},
    {16, "Kou Ichijo", LinkKind::Normal, 0, 0, 17, 0},
    {17, "Daisuke Nagase", LinkKind::Normal, 0, 0, 16, 0},
    {18, "Naoki Konishi", LinkKind::Normal, 0, 0, 0, 0},
    {19, "Hisano Kuroda", LinkKind::Normal, 0, 0, 0, 0},
    {20, "Margaret", LinkKind::Normal, 0, 0, 0, 0},
    {21, "Ai Ebihara", LinkKind::Normal, 22, 0, 0, 0},
    {22, "Ai Ebihara", LinkKind::RomanceRow, 0, 21, 0, -1},
    {23, "Shu Nakajima", LinkKind::Normal, 0, 0, 0, 0},
    {24, "Teddie", LinkKind::Story, 0, 0, 0, 13},               // 4/14
    {25, "Yumi Ozawa", LinkKind::Normal, 26, 0, 27, 0},
    {26, "Yumi Ozawa", LinkKind::RomanceRow, 0, 25, 0, -1},
    {27, "Ayane Matsunaga", LinkKind::Normal, 28, 0, 25, 0},
    {28, "Ayane Matsunaga", LinkKind::RomanceRow, 0, 27, 0, -1},
    {29, "Eri Minami", LinkKind::Normal, 0, 0, 0, 0},
    {30, "Seekers of Truth", LinkKind::Story, 0, 0, 0, -1},
    {31, "Tohru Adachi", LinkKind::Story, 0, 0, 0, -1},
    {32, "Tohru Adachi", LinkKind::Story, 0, 0, 0, -1},
    {33, "Marie", LinkKind::Normal, 34, 0, 0, 17},              // 4/18
    {34, "Marie", LinkKind::RomanceRow, 0, 33, 0, -1},
};
inline constexpr int kLinkCount = int(sizeof kLinks / sizeof kLinks[0]);

inline const LinkInfo* Link(uint16_t id) { return id < kLinkCount ? &kLinks[id] : nullptr; }

// The row shown for a link: romance rows are folded into their base link.
inline uint16_t BaseRow(uint16_t id) {
    const LinkInfo* l = Link(id);
    return (l && l->kind == LinkKind::RomanceRow) ? l->baseRow : id;
}

}  // namespace Data
