#pragma once
// P4G look: hard edges, Inaba yellow on near-black, slanted tabs, TV-static accents.

#include <imgui.h>

namespace Theme {

// Palette (ImU32, ABGR) shared by every page.
namespace C {
constexpr ImU32 Yellow = IM_COL32(255, 225, 0, 255);
constexpr ImU32 YellowDim = IM_COL32(190, 165, 0, 255);
constexpr ImU32 Orange = IM_COL32(247, 148, 29, 255);
constexpr ImU32 Black = IM_COL32(12, 12, 12, 255);
constexpr ImU32 Panel = IM_COL32(24, 24, 22, 248);
constexpr ImU32 PanelHi = IM_COL32(40, 39, 34, 255);
constexpr ImU32 White = IM_COL32(245, 245, 240, 255);
constexpr ImU32 Grey = IM_COL32(150, 148, 140, 255);
constexpr ImU32 Red = IM_COL32(230, 60, 50, 255);
constexpr ImU32 Green = IM_COL32(110, 205, 90, 255);
constexpr ImU32 Blue = IM_COL32(80, 160, 240, 255);
constexpr ImU32 Dim = IM_COL32(0, 0, 0, 150);
}  // namespace C

// Risk tiers used by every control that changes the game.
enum class Risk { Safe, Caution, Story, Dangerous };
ImU32 RiskColor(Risk r);
const char* RiskName(Risk r);

struct Fonts {
    ImFont* body = nullptr;
    ImFont* bold = nullptr;
    ImFont* title = nullptr;  // heavy display face for headers
    ImFont* note = nullptr;
};
extern Fonts g_fonts;

void LoadFonts(float scale);
void Apply(float scale);

// Drawing helpers
void SlantRect(ImDrawList* dl, ImVec2 min, ImVec2 max, float slant, ImU32 col);
void Stripes(ImDrawList* dl, ImVec2 min, ImVec2 max, float spacing, ImU32 col);
void SectionHeader(const char* text);
void RiskBadge(Risk r);

}  // namespace Theme
