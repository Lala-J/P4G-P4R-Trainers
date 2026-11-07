#include <imgui.h>
#include <cstdio>
#include <string>

#include "Data/Stats.h"
#include "Engine/GameState.h"
#include "Engine/Names.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace Pages {

void Protagonist() {
    if (!RequireSave()) return;
    const auto& s = Game::Current();
    const auto& yu = s.U(Game::Yu);
    float sc = Overlay::Scale();

    SectionHeader("YU NARUKAMI");
    ImGui::Text("Level %u", yu.level);
    uint32_t cur = Game::ExpForLevel(yu.level), next = Game::ExpForLevel(yu.level + 1);
    if (next > cur) {
        char buf[96];
        snprintf(buf, sizeof buf, "%u EXP  ·  %u to level %u", yu.exp, next - yu.exp, yu.level + 1);
        W::Bar(float(yu.exp - cur) / float(next - cur), ImVec2(420 * sc, 0), C::Blue, buf);
    } else {
        ImGui::Text("%u EXP", yu.exp);
    }
    ImGui::Text("HP %u   SP %u", yu.hp, yu.sp);
    ImGui::Text("Money: ¥%u", s.yen);

    SectionHeader("SOCIAL STATS");
    if (ImGui::BeginTable("stats", 4, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Stat", ImGuiTableColumnFlags_WidthStretch, 1.1f);
        ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthStretch, 1.6f);
        ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("Points", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        for (int i = 0; i < 5; ++i) {
            const auto& d = Data::kStats[i];
            uint16_t v = yu.social[i];
            int rank = Data::StatRank(i, v);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            W::Title(d.name);
            ImGui::TableNextColumn();
            W::Pips(rank, 5, C::Yellow, IM_COL32(255, 255, 255, 60));
            ImGui::SameLine();
            ImGui::Text("%s", d.ranks[rank - 1]);
            ImGui::TableNextColumn();
            if (rank < 5) {
                uint16_t lo = rank == 1 ? 0 : d.threshold[rank - 2], hi = d.threshold[rank - 1];
                char buf[96];
                snprintf(buf, sizeof buf, "%u more to %s%s", hi - v, d.ranks[rank],
                         d.contested && rank == 4 ? " (community-reported)" : "");
                W::Bar(float(v - lo) / float(hi - lo), ImVec2(-1, 0), C::Yellow, buf);
            } else {
                W::Chip("MAX", C::Yellow);
            }
            ImGui::TableNextColumn();
            ImGui::Text("%u", v);
        }
        ImGui::EndTable();
    }
    W::Dim("Rank thresholds are fan-derived (the game shows them only as music notes).");

    SectionHeader("EQUIPMENT");
    static const char* slots[] = {"Weapon", "Armor", "Accessory", "Outfit"};
    for (int i = 0; i < 4; ++i) {
        uint16_t id = yu.equip[i];
        std::string n = id ? Names::ItemOr(id) : std::string("—");
        ImGui::Text("%-10s %s", slots[i], n.c_str());
    }
}

}  // namespace Pages
