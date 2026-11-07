#pragma once
// Item id ranges (the inventory is a count per id, 0..2559).

#include <cstdint>

namespace Data {

struct ItemCategory {
    const char* name;
    uint16_t first, last;  // inclusive
};

inline constexpr ItemCategory kItemCategories[] = {
    {"Weapons", 0x000, 0x0FF},     {"Armor", 0x100, 0x1FF},       {"Accessories", 0x200, 0x2FF},
    {"Consumables", 0x300, 0x3FF}, {"Key Items", 0x400, 0x4FF},   {"Materials", 0x500, 0x5FF},
    {"Skill Cards", 0x600, 0x6FF}, {"Outfits", 0x700, 0x7FF},     {"Golden Items", 0x800, 0x8FF},
    {"Golden Weapons", 0x900, 0x9FF},
};
inline constexpr int kItemCategoryCount = int(sizeof kItemCategories / sizeof kItemCategories[0]);

inline int ItemCategoryOf(uint16_t id) { return id >> 8; }

}  // namespace Data
