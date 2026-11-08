#include <imgui.h>
#include <cstdio>
#include <string>

#include "Data/Stats.h"
#include "Engine/GameState.h"
#include "Engine/Names.h"
#include "Features/Edits.h"
#include "Features/Equipment.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace {

int g_eqChar = 1;   // member whose gear is shown
int g_eqSlot = -1;  // slot whose picker is open

bool Joined(const Game::Snapshot& s, int c) {
    if (c == Game::Yu) return true;
    static const int flag[9] = {0, 0, 48, 49, 50, 52, 51, 54, 53};  // by character id
    return s.Flag(flag[c]);
}

std::string StatLine(uint16_t item, int slot) {
    auto st = Equipment::ItemStats(item);
    char b[64];
    if (slot == Equipment::Weapon) snprintf(b, sizeof b, "Atk %d  Hit %d", st.attack, st.hit);
    else if (slot == Equipment::Armour) snprintf(b, sizeof b, "Def %d  Eva %d", st.defence, st.evasion);
    else b[0] = 0;
    return b;
}

void DrawEquipment(const Game::Snapshot& s) {
    float sc = Overlay::Scale();
    SectionHeader("EQUIPMENT");
    if (!Equipment::Available()) {
        W::Dim("Unavailable on this game build (a host check failed).");
        return;
    }
    static const char* who[] = {"", "Yu", "Yosuke", "Chie", "Yukiko", "Rise", "Kanji", "Naoto", "Teddie"};
    int shown = 0;
    for (int c = 1; c <= 8; ++c) {
        if (!Joined(s, c)) continue;  // members who haven't joined stay hidden (spoilers)
        if (shown++) ImGui::SameLine();
        bool sel = g_eqChar == c;
        if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(C::Yellow));
        if (sel) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(C::Black));
        ImGui::PushID(c);
        if (ImGui::Button(who[c])) {
            g_eqChar = c;
            g_eqSlot = -1;
        }
        ImGui::PopID();
        if (sel) ImGui::PopStyleColor(2);
    }
    if (!Joined(s, g_eqChar)) g_eqChar = 1;
    const auto& u = s.U(g_eqChar);
    for (int slot = 0; slot < 4; ++slot) {
        ImGui::PushID(100 + slot);
        uint16_t id = u.equip[slot];
        char label[96];
        snprintf(label, sizeof label, "%-10s %s", Equipment::SlotName(slot), id ? Names::ItemOr(id).c_str() : "—");
        bool open = g_eqSlot == slot;
        if (open) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(C::YellowDim));
        if (ImGui::Button(label, ImVec2(330 * sc, 0))) g_eqSlot = open ? -1 : slot;
        if (open) ImGui::PopStyleColor();
        ImGui::SameLine();
        W::Dim("%s", StatLine(id, slot).c_str());
        ImGui::PopID();
    }
    if (g_eqSlot < 0) {
        W::Dim("Pick a slot to see what this member can wear from the bag. The old piece goes back into the "
               "bag, as in the game's own equip menu.");
        return;
    }
    ImGui::Spacing();
    W::Title(Equipment::SlotName(g_eqSlot));
    ImGui::BeginChild("eqpick", ImVec2(0, 220 * sc), ImGuiChildFlags_Borders);
    int n = 0;
    for (uint16_t item = 1; item < Game::kItems; ++item) {
        if (!Equipment::ItemFitsSlot(item, g_eqSlot) || !Equipment::CanWield(item, g_eqChar)) continue;
        if (!s.items[item]) continue;
        std::string nm = Names::Item(item);
        if (nm.empty()) continue;
        ++n;
        ImGui::PushID(item);
        const char* why = Equipment::WhyNot(g_eqChar, g_eqSlot, item);
        char row[128];
        snprintf(row, sizeof row, "%s%s", nm.c_str(), u.equip[g_eqSlot] == item ? "  (equipped)" : "");
        ImGui::BeginDisabled(why != nullptr);
        if (ImGui::Selectable(row, false, 0, ImVec2(330 * sc, 0))) Equipment::Equip(g_eqChar, g_eqSlot, item);
        ImGui::EndDisabled();
        ImGui::SameLine(360 * sc);
        W::Dim("%s  ×%u", StatLine(item, g_eqSlot).c_str(), s.items[item]);
        ImGui::PopID();
    }
    if (!n) W::Dim("Nothing in the bag that %s can use here.", who[g_eqChar]);
    ImGui::EndChild();
}

}  // namespace

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

    DrawEquipment(s);
}

}  // namespace Pages
