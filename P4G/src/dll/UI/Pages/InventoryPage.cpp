#include <imgui.h>
#include <cctype>
#include <cstdio>
#include <string>

#include "Data/Items.h"
#include "Engine/GameState.h"
#include "Engine/Names.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace {

char g_search[64] = "";
bool g_showAll = false;

bool Matches(const std::string& name, const char* q) {
    if (!*q) return true;
    std::string a = name, b = q;
    for (auto& c : a) c = (char)std::tolower((unsigned char)c);
    for (auto& c : b) c = (char)std::tolower((unsigned char)c);
    return a.find(b) != std::string::npos;
}

void DrawCategory(const Game::Snapshot& s, const Data::ItemCategory& cat) {
    if (!ImGui::BeginTable("items", 3,
                           ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp,
                           ImVec2(0, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 1.5f)))
        return;
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 4.0f);
    ImGui::TableSetupColumn("Owned", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthStretch, 0.8f);
    ImGui::TableHeadersRow();
    int shown = 0;
    for (int id = cat.first; id <= cat.last; ++id) {
        uint8_t n = s.items[id];
        if (!n && !g_showAll) continue;
        std::string name = Names::Item((uint16_t)id);
        if (name.empty() && !n) continue;  // unused slot
        if (name.empty()) name = Names::ItemOr((uint16_t)id);
        if (!Matches(name, g_search)) continue;
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (n) ImGui::TextUnformatted(name.c_str());
        else ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Grey), "%s", name.c_str());
        ImGui::TableNextColumn();
        if (n) ImGui::Text("%u", n);
        else W::Dim("—");
        ImGui::TableNextColumn();
        W::Dim("%03X", id);
        ++shown;
    }
    ImGui::EndTable();
    if (!shown) W::Dim(g_showAll ? "No items match." : "You don't have any items here.");
}

}  // namespace

namespace Pages {

void Inventory() {
    if (!RequireSave()) return;
    const auto& s = Game::Current();
    float sc = Overlay::Scale();

    ImGui::SetNextItemWidth(260 * sc);
    ImGui::InputTextWithHint("##search", "Search items", g_search, sizeof g_search);
    ImGui::SameLine();
    ImGui::Checkbox("Show items you don't have", &g_showAll);
    ImGui::SameLine();
    W::Dim("Editing arrives in M2.");

    if (ImGui::BeginTabBar("cats", ImGuiTabBarFlags_FittingPolicyScroll)) {
        for (const auto& cat : Data::kItemCategories) {
            int owned = 0;
            for (int id = cat.first; id <= cat.last; ++id) owned += s.items[id] != 0;
            char label[64];
            snprintf(label, sizeof label, "%s (%d)###%s", cat.name, owned, cat.name);
            if (ImGui::BeginTabItem(label)) {
                DrawCategory(s, cat);
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
}

}  // namespace Pages
