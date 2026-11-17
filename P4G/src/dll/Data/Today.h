#pragma once
// The Today page's priority tiers. What's on offer comes from the game itself: who's around from
// Features/WhoIsAround, jobs and activities from Features/Activities.

#include <cstdint>

namespace Data {

enum class Tier : uint8_t {
    LastChance = 0,  // missable soon (deadlines, cutoffs)
    Romance = 1,     // dating / romance-route hangouts
    Links = 2,       // social links, hangouts, bike trips
    ClubsJobs = 3,
    Other = 4,
};

}  // namespace Data
