#include <imgui.h>
#include <cctype>
#include <cstdio>
#include <string>

#include "Data/Items.h"
#include "Engine/GameState.h"
#include "Engine/Names.h"
#include "Features/Edits.h"
#include "Features/Inventory.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace {

char g_search[64] = "";
bool g_showAll = false;
bool g_allowStory = false;
int g_batch = 9;

// Key items (0x4xx) and the Golden range (0x8xx, which mixes in story items) drive events and
// quests; editing them can skip or break those, so they're locked unless explicitly allowed.
bool IsStoryItem(int id) { return (id >= 0x400 && id <= 0x4FF) || (id >= 0x800 && id <= 0x8FF); }
// Quest hand-in items inside the consumable range (Amicitia quest list).
bool IsQuestItem(int id) { return id >= 0x3D2 && id <= 0x3EB; }

bool Matches(const std::string& name, const char* q) {
    if (!*q) return true;
    std::string a = name, b = q;
    for (auto& c : a) c = (char)std::tolower((unsigned char)c);
    for (auto& c : b) c = (char)std::tolower((unsigned char)c);
    return a.find(b) != std::string::npos;
}

void DrawCategory(const Game::Snapshot& s, const Data::ItemCategory& cat) {
    if (!ImGui::BeginTable("items", 4,
                           ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp,
                           ImVec2(0, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 1.5f)))
        return;
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 4.0f);
    ImGui::TableSetupColumn("Owned", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthStretch, 0.8f);
    ImGui::TableSetupColumn("Set count", ImGuiTableColumnFlags_WidthStretch, 2.0f);
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
        ImGui::TableNextColumn();
        ImGui::PushID(id);
        bool locked = IsStoryItem(id) && !g_allowStory;
        ImGui::BeginDisabled(locked);
        int v = n;
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputInt("##n", &v, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (v < 0) v = 0;
            if (v > 99) v = 99;
            Edits::SetItemCount((uint16_t)id, (uint8_t)v);
        }
        ImGui::EndDisabled();
        if (locked && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Story item. Turn on \"Allow editing story items\" above to change it.");
        else if (IsQuestItem(id) && ImGui::IsItemHovered())
            ImGui::SetTooltip("Quest item: having it can complete the matching quest early.");
        ImGui::PopID();
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
    ImGui::Checkbox("Allow editing story items", &g_allowStory);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        RiskBadge(Risk::Story);
        ImGui::TextUnformatted("Key items open events, dungeons and quests. Adding or removing one\n"
                               "can skip or break those. Leave this off unless you know you need it.");
        ImGui::EndTooltip();
    }

    ImGui::BeginDisabled(!InventoryFx::Available());
    if (ImGui::Checkbox("Items never run out", &InventoryFx::NeverRunOut())) InventoryFx::SaveOptions();
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (InventoryFx::Available())
        W::Dim("Using an item doesn't use it up. Selling, giving and quests still work normally.");
    else
        W::Dim("Being verified against each way of using items; available in an upcoming build.");

    ImGui::SetNextItemWidth(110 * sc);
    ImGui::InputInt("##batch", &g_batch, 1, 10);
    if (g_batch < 1) g_batch = 1;
    if (g_batch > 99) g_batch = 99;
    ImGui::SameLine();
    if (W::RiskButton("Set every consumable I own to this", Risk::Safe,
                      "Only consumables you already have; quest items are skipped.")) {
        for (int id = 0x300; id <= 0x3FF; ++id)
            if (s.items[id] && !IsQuestItem(id) && !Names::Item((uint16_t)id).empty())
                Edits::SetItemCount((uint16_t)id, (uint8_t)g_batch);
    }
    W::Dim("Counts change through the game's own item routine (max 99). Press Enter to apply a typed value.");

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
