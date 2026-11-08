#pragma once
// Plain-value edits (money, social stats, item counts, Persona stats and skills). Each one is
// queued for the game thread and journaled with its exact undo. Story-level changes (social
// link ranks, Persona creation, fusion) go through the game's own routines elsewhere.

#include <cstdint>

namespace Edits {

constexpr uint32_t kMaxYen = 9999999;

void SetYen(uint32_t yen);
void SetSocialStat(int stat, uint16_t points);
void SetItemCount(uint16_t id, uint8_t count);           // through the game's SetItemCount
void SetPersonaStat(int slot, int stat, uint8_t base);   // stock slot 0..11, stat 0..4
void SetPersonaSkill(int slot, int index, uint16_t skillId);
void SetUnitHpSp(int charId, int hp, int sp);

// Raw event flags and counters, through the game's own BIT_ON / BIT_OFF / SET_CNT.
void SetFlag(int flag, bool on);
void SetCounter(int index, uint32_t value);              // -1 = leave; clamped by the game's own helpers

}  // namespace Edits
