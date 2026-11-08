#pragma once
// Enemy affinities from the game's own enemy affinity table (16 values per enemy:
// Phys Fire Ice Elec Wind Almighty Light Dark, then 8 ailments), and the current battle's enemies.

#include <cstdint>
#include <vector>

namespace Intel {

enum class Aff : uint8_t { Normal, Weak, Resist, Null, Repel, Drain };
const char* AffShort(Aff a);   // "Wk", "Rs", "Nu", "Rp", "Dr", "-"

constexpr int kElements = 8;
constexpr int kAilments = 8;
const char* ElementName(int e);   // "Phys", "Fire", ...
const char* AilmentName(int a);   // "Panic", "Poison", ...

bool Available();
Aff Affinity(uint16_t enemyId, int element);   // element 0..15
bool Seen(uint16_t enemyId);                    // the game knows at least one of its affinities

struct Foe {
    uint16_t id;
    uint16_t hp, sp;
};
std::vector<Foe> CurrentEnemies();              // empty outside battle

bool& ShowHud();
void SaveOptions();

}  // namespace Intel
