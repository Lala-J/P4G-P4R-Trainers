#pragma once
// Who can be found where in the current time slot, for the Today page.
//
//   The game's scheduler sets "who is around today" flags each morning, but a flag on its own
//   isn't enough: some people have two sets of day flags (one in use at a time), evening
//   hangouts have block flags, and several links have extra conditions. These are the rules the
//   Find-a-Friend mod (AnimatedSwine37; also in ShrineFox's P4G Mod Menu) uses to decide who to
//   offer, and the places it takes you to find them, rewritten here. docs/data/around.md lists
//   every flag involved.

#include <cstdint>
#include <vector>

#include "Engine/GameState.h"

namespace Around {

struct Person {
    const char* who;
    uint16_t link;      // base commu id (for rank, spoilers, romance)
    bool bike;          // a scooter trip rather than a hangout
    bool nightOut;      // one of Golden's evening hangouts (points and an item, no rank-up);
                        // not set for links whose own events happen in the evening (Nanako...)
    const char* where;
    int8_t ready = -1;  // can rank up now: -1 = by the link's points, 0/1 = decided here (the fox)
    const char* note = nullptr;
};

// Everyone available in the current slot (daytime: time 0..4, evening: 5).
std::vector<Person> Now(const Game::Snapshot& s);

// The flags these rules read, for the flag browser: "<who>: <what>" (link 0 = nobody in particular).
struct FlagUse {
    int16_t flag;
    uint16_t link;
    const char* who;
    const char* what;
};
const std::vector<FlagUse>& Flags();

}  // namespace Around
