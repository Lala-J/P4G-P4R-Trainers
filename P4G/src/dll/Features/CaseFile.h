#pragma once
// Where the kidnapping case stands, from the date and the rescue flags.

#include "Engine/GameState.h"

namespace CaseFile {

enum class State {
    None,        // nothing pending (between cases, or after the last one)
    Upcoming,    // the next victim's case hasn't opened yet
    Active,      // dungeon open, deadline ahead
    LastDay,     // today is the deadline
};

struct Status {
    State state = State::None;
    int index = -1;          // into Data::kCases
    int daysLeft = 0;        // calendar days until the deadline (0 = today)
    bool todayUsable = false;  // the TV world can still be entered today (daytime / after school)
    bool rescueKnown = false;  // a rescue flag exists for this case
    bool verified = false;     // ...and it was confirmed against the scripts
};

Status Current(const Game::Snapshot& s);

// Per-case resolution, for the timeline: true = solved, false = not yet.
// Cases before today's date are solved by definition (the game would have ended otherwise).
bool Solved(const Game::Snapshot& s, int caseIndex);

}  // namespace CaseFile
