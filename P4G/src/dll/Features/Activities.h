#pragma once
// Jobs and other things to do in the current time slot, for the Today page.
//
//   Each job runs only on the days the game's scheduler switches its day flag on, and only once
//   you've signed up for it at the shopping district's job board. The scheduler's flags are a
//   fixed date table that already leaves out days the job can't run (the day-care closes when
//   it rains, for one), so these are the same checks the town's own scripts make before they
//   offer a shift. Activities that depend on the weather itself ask the game's GET_WEATHER.
//   docs/data/jobs.md lists every flag and the script each check comes from.

#include <cstdint>
#include <string>
#include <vector>

#include "Data/Today.h"
#include "Engine/GameState.h"

namespace Activities {

struct Item {
    Data::Tier tier;
    int order;          // tie-break inside a tier (lower first)
    std::string what, where, note;
};

// Everything on offer now (daytime: after school or a day off; evening: slot 5).
std::vector<Item> Now(const Game::Snapshot& s);

// The flags these checks read, for the flag browser: "<what>: <meaning>".
struct FlagUse {
    int16_t flag;
    const char* what;
    const char* meaning;
};
const std::vector<FlagUse>& Flags();

}  // namespace Activities
