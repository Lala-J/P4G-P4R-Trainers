#pragma once
// The kidnapping case timeline (docs/research/mechanics.md, section 2).
// Day indices: Calendar::ToIndex(month, day); 4/1 = 0.

#include <cstdint>

namespace Data {

struct CaseInfo {
    const char* victim;
    const char* dungeon;
    int16_t accessDay;     // first day the dungeon can be entered
    int16_t lastDay;       // game over that evening if not cleared (or the only day)
    bool gameOver;         // false for the optional / single-day dungeons
    int16_t rescueFlag;    // flag set when cleared; -1 = not yet identified
    bool flagVerified;     // the flag was confirmed against the game's scripts
};

inline constexpr CaseInfo kCases[] = {
    // Rescue flags 35..40 (docs/data/case_flags.md): set by the day scheduler, as the rescue cutscene
    // plays, when the boss flag (17..22) is on. Yukiko's was also watched live.
    {"Yukiko Amagi", "Yukiko's Castle", 17, 28, true, 35, true},   // 4/18 .. 4/29
    {"Kanji Tatsumi", "Steamy Bathhouse", 48, 64, true, 36, true},  // 5/19 .. 6/4
    {"Rise Kujikawa", "Marukyu Striptease", 85, 99, true, 37, true},  // 6/25 .. 7/9
    {"Mitsuo Kubo", "Void Quest", 119, 133, true, 38, true},        // 7/29 .. 8/12
    {"Naoto Shirogane", "Secret Laboratory", 168, 187, true, 39, true},  // 9/16 .. 10/5
    {"Nanako Dojima", "Heaven", 219, 233, true, 40, true},          // 11/6 .. 11/20
    {"Tohru Adachi", "Magatsu Inaba", 251, 267, true, -1, false},    // 12/8 .. 12/24
    {"Marie", "Hollow Forest", 318, 318, false, -1, false},          // 2/13 only
    {"Izanami", "Yomotsu Hirasaka", 354, 354, false, -1, false},     // 3/20 only
};
inline constexpr int kCaseCount = int(sizeof kCases / sizeof kCases[0]);

}  // namespace Data
