#include <imgui.h>
#include <cstdio>
#include <string>

#include "Data/Stats.h"
#include "Engine/GameState.h"
#include "Engine/Names.h"
#include "Features/Edits.h"
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

    static int yen = -1;
    static int64_t lastYen = -1;
    if (lastYen != (int64_t)s.yen) yen = (int)s.yen, lastYen = s.yen;  // follow the game's value
    ImGui::SetNextItemWidth(220 * sc);
    ImGui::InputInt("##yen", &yen, 1000, 10000);
    if (yen < 0) yen = 0;
    if (yen > (int)Edits::kMaxYen) yen = (int)Edits::kMaxYen;
    ImGui::SameLine();
    if (W::RiskButton("Set money", Risk::Safe, "Changes how much money you have (max ¥9,999,999)."))
        Edits::SetYen((uint32_t)yen);
    ImGui::SameLine();
    W::Dim("now ¥%u", s.yen);

    SectionHeader("SOCIAL STATS");
    if (ImGui::BeginTable("stats", 5, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Stat", ImGuiTableColumnFlags_WidthStretch, 1.1f);
        ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthStretch, 1.6f);
        ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("Points", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableSetupColumn("Edit", ImGuiTableColumnFlags_WidthStretch, 2.2f);
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
            ImGui::TableNextColumn();
            ImGui::PushID(i);
            static int edit[5] = {-1, -1, -1, -1, -1};
            static int last[5] = {-1, -1, -1, -1, -1};
            if (last[i] != v) edit[i] = last[i] = v;  // follow the game's value until edited
            ImGui::SetNextItemWidth(110 * sc);
            ImGui::InputInt("##v", &edit[i], 1, 10);
            if (edit[i] < 0) edit[i] = 0;
            if (edit[i] > 999) edit[i] = 999;
            ImGui::SameLine();
            if (W::RiskButton("Set", Risk::Caution,
                              "Sets the stat directly. The in-game \"stat up\" message won't play; "
                              "rank-gated options (links, jobs, dialogue) unlock as normal."))
                Edits::SetSocialStat(i, (uint16_t)edit[i]);
            ImGui::SameLine();
            if (rank < 5 && W::RiskButton("Next rank", Risk::Caution,
                                          "Raises the stat to exactly the next rank's threshold."))
                Edits::SetSocialStat(i, d.threshold[rank - 1]);
            ImGui::PopID();
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
