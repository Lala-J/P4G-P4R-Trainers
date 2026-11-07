#pragma once
// Names come from the game's own tables, so they match the player's language and nothing
// copyrighted ships with the trainer. Placeholder entries ("000", "1A0", "blank") report empty.

#include <cstdint>
#include <string>

namespace Names {

std::string Persona(uint16_t id);
std::string Skill(uint16_t id);
std::string Enemy(uint16_t id);
std::string Arcana(uint8_t id);
std::string Item(uint16_t id);

// Name or a "#id" fallback when the table has no real name for it.
std::string PersonaOr(uint16_t id);
std::string SkillOr(uint16_t id);
std::string ItemOr(uint16_t id);

}  // namespace Names
