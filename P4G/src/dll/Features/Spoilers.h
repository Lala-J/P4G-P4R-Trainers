#pragma once
// Spoiler policy. By default anything past the player's progress shows as "???" and can be
// revealed one item at a time; reveals are remembered (settings.ini, [reveal]).

#include <cstdint>

namespace Spoilers {

enum class Kind : uint8_t { Link, Case, Romance, Step, Persona, Enemy };

enum class Policy : int { HideFuture = 0, ShowAll = 1 };
Policy GetPolicy();
void SetPolicy(Policy p);

bool Revealed(Kind k, int id);
void Reveal(Kind k, int id);

// True when the item may be shown: naturally visible, revealed, or spoilers are off.
bool Visible(Kind k, int id, bool naturallyVisible);

// Draws a "??? [Reveal]" placeholder. Returns true if the item was just revealed.
bool HiddenPlaceholder(Kind k, int id, const char* what = nullptr);

}  // namespace Spoilers
