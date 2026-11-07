#pragma once
// Small drawing helpers shared by the pages.

#include <imgui.h>

#include "UI/Theme.h"

namespace W {

// A bordered card. Always pair with EndCard().
bool BeginCard(const char* id, ImVec2 size, ImU32 accent);
void EndCard();

void Pips(int filled, int total, ImU32 on, ImU32 off, float size = 0);
void Bar(float frac, ImVec2 size, ImU32 fill, const char* overlay = nullptr);
void Chip(const char* text, ImU32 bg, ImU32 fg = IM_COL32(12, 12, 12, 255));
void Dim(const char* fmt, ...);
void Title(const char* text);  // bold line

ImU32 ArcanaColor(int arcana);

// A button gated by risk. Safe / Caution act on click (Caution explains itself on hover);
// Story-sensitive / Dangerous open a confirmation that says exactly what will happen.
// Returns true when the action should run.
bool RiskButton(const char* label, Theme::Risk risk, const char* why);

// Number of card columns that fit in the current content width.
int Columns(float minCardWidth);

}  // namespace W
