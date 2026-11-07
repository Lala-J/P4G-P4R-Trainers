#pragma once
// Social stat ranks. Thresholds are the points needed to reach ranks 2..5 (rank 1 at 0).
// Source: AquaRose7 (live-verified) and community guides; Diligence's last step is contested.

#include <cstdint>

namespace Data {

struct StatInfo {
    const char* name;
    const char* ranks[5];
    uint16_t threshold[4];
    bool contested;
};

inline constexpr StatInfo kStats[5] = {
    {"Courage", {"Average", "Reassuring", "Brave", "Daring", "Heroic"}, {16, 40, 80, 140}, false},
    {"Knowledge", {"Aware", "Informed", "Expert", "Professor", "Sage"}, {30, 80, 150, 240}, false},
    {"Diligence", {"Callow", "Persistent", "Strong", "Thorough", "Rock Solid"}, {16, 40, 80, 130}, true},
    {"Understanding", {"Basic", "Kindly", "Generous", "Motherly", "Saintly"}, {16, 40, 80, 140}, false},
    {"Expression", {"Rough", "Eloquent", "Persuasive", "Touching", "Enthralling"}, {13, 33, 53, 85}, false},
};

inline int StatRank(int stat, uint16_t points) {  // 1..5
    int r = 1;
    for (uint16_t t : kStats[stat].threshold)
        if (points >= t) ++r;
    return r;
}

}  // namespace Data
