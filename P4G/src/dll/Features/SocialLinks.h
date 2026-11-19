#pragma once
// Social link changes, done the way the game does them:
//   Ready     — the link's points reach the game's own threshold, so the next real hangout plays
//               the actual rank-up scene (every choice and side effect intact). Preferred.
//   Rank up   — the game's INCREASE_SL, then the rank's mined unconditional effects (skill
//               unlocks). Choice-dependent effects (romance) are never replayed.
//   Start     — the game's own "commu open" task (the arcana card), as the story would.
// Every action is assessed first; the UI shows the reasons and the risk.

#include <cstdint>
#include <string>
#include <vector>

#include "Engine/GameState.h"
#include "UI/Theme.h"

namespace Links {

enum class Action { Ready, RankUp, Start };

struct Assessment {
    bool allowed = true;            // false = the action makes no sense (button disabled)
    Theme::Risk risk = Theme::Risk::Caution;
    std::vector<std::string> reasons;
    std::string Text() const;
};

Assessment Assess(const Game::Snapshot& s, uint16_t baseId, Action a);

void MakeReady(uint16_t baseId);
void RankUpNow(uint16_t baseId);
void Start(uint16_t baseId);
void SetPoints(uint16_t baseId, uint16_t points);

// Whether a link has become a romance, from the game's own lover flag. At the romance rank the
// link's entry moves to its romance-row id whichever answer you give (each rank event's
// "<name>_friend" procedure), so the row says nothing about it; only the lover procedure sets
// the flag. Ai's flag also marks the temporary relationship from her rank 6 answer, which her
// rank 9 scene ends (clearing it); it means a real one only from rank 10.
enum class Bond { None, Temporary, Dating };
Bond RomanceBond(const Game::Snapshot& s, uint16_t baseId);

}  // namespace Links
