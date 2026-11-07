#pragma once
// Survival options. HP and SP only ever change through the game's AddHP / AddSP helpers;
// the trainer filters negative changes for party members there, so a hit that would knock
// someone out never lands, rather than being healed back afterwards.

#include "Engine/GameTypes.h"

namespace Battle {

enum class HpMode : int { Off = 0, Lock = 1, Survive = 2 };  // Survive: never below 1 HP
enum class Scope : int { Party = 0, YuOnly = 1 };

struct Options {
    HpMode hp = HpMode::Off;
    bool sp = false;
    Scope scope = Scope::Party;
};

Options& Opts();
void LoadOptions();
void SaveOptions();

bool InstallHooks();  // AddHP / AddSP filters

// True for the protagonist's / party members' unit records, including heap units the game
// creates for scripted fights (found through the battle manager's party list).
bool IsPartyUnit(const Game::Unit* u);

// Restores HP and SP to the game's own maximums for everyone in scope (game thread).
void RefillNow();

}  // namespace Battle
