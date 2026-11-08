#pragma once
// Battle rule overrides. Every one is a hook on the game's own resolver for that rule, so the
// game still plays its normal animation and text ("Miss", "Critical", "Block", ...). Turning a
// switch off restores vanilla behaviour on the very next action.
//
//   Hit resolver   0x1400D4650 (attacker, target, skill, flags) -> 1 hit, 4 miss, 0x200 dodged,
//                  0x100 / 0x400 null-type. Flag bit 2 means a sure hit (scripted attacks).
//   Crit resolver  0x1400D4E40 (attacker, target, skill, flags) -> 1 hit, 2 crit, 4 weak, 8 resist.
//   Affinity       0x1400D35B0 (unit, element) -> multiplier % | 0x08000000 weak | 0x10000000 resist
//                  | 0x01000000 null | 0x02000000 repel | 0x04000000 absorb.
//   AI_RUN_AWAY    0x14006F7B0, identical to AI_WAIT 0x14006F820 except for the queued action.
//   CanChangeToState 0x140099180 (turn, state): state 5 = Persona.

#include <cstdint>

namespace BattleFx {

struct Options {
    bool noDowns = false;        // party can't be knocked down (no weak / crit hits on them)
    bool noAilments = false;     // party immune to ailments
    bool noInstakill = false;    // party immune to Light/Dark instant kills
    bool noDebuffs = false;      // party stat penalties are cleared as soon as they land
    bool enemiesMiss = false;    // enemy attacks on the party always miss (sure-hit attacks excepted)
    bool partyHits = false;      // party attacks never miss or get dodged
    bool partyCrits = false;     // party attacks that can crit, always do
    bool forcedOneMore = false;  // every party hit on a standing enemy knocks it down -> 1 More
    bool noFlee = false;         // shadows can't run away
    bool weakness = false;       // party attacks hit the target's weakness
    bool weaknessForce = false;  // ...even when it has none
    bool unlimitedSwap = false;  // Yu can change Persona more than once a turn
    bool revealAffinities = false;  // every enemy affinity shows as analysed (Enemy Intel)
};

Options& Opts();
void SaveOptions();
bool InstallHooks();
void Tick();  // every frame on the game thread (debuff / ailment cleanup)

// The game's own "is this affinity known?" answer, ignoring the reveal switch.
uint64_t KnownUnhooked(uint16_t enemyId, uint16_t element);

}  // namespace BattleFx
