#pragma once
// Names for the game's event flags and counters, gathered from what the trainer already knows
// (party joins, romance, case clears, the daily availability flags, quests, social link rank
// events). Labels that would reveal story are marked so the browser can mask them.

#include <cstdint>
#include <string>

namespace FlagLabels {

struct Label {
    std::string text;
    bool spoiler = false;
};

const Label* Flag(int n);        // null if unknown
const char* Counter(int n);      // null if unknown
int LabelledFlagCount();

}  // namespace FlagLabels
