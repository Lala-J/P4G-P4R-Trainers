#include <imgui.h>
#include <cstdio>
#include <string>

#include "Engine/Addresses.h"
#include "Engine/GameState.h"
#include "Engine/Names.h"
#include "Features/Levels.h"
#include "Features/Spoilers.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace {

const char* CharName(int id) {
    static const char* n[] = {"", "Yu", "Yosuke", "Chie", "Yukiko", "Rise", "Kanji", "Naoto", "Teddie"};
    return (id >= 1 && id <= 8) ? n[id] : "?";
}
int JoinFlag(int c) {
    static const int f[] = {-1, -1, 48, 49, 50, 52, 51, 54, 53};
    return (c >= 0 && c <= 8) ? f[c] : -1;
}

int g_addExp = 1000;

void PersonaRow(const char* id, const char* owner, const Game::Persona& p, Levels::Target t) {
    float sc = Overlay::Scale();
    ImGui::PushID(id);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    if (owner) {
        W::Title(owner);
        ImGui::SameLine();
    }
    ImGui::TextUnformatted(Names::PersonaOr(p.id).c_str());
    ImGui::TableNextColumn();
    ImGui::Text("Lv %u", p.level);
    ImGui::TableNextColumn();
    uint32_t cur = Levels::ExpForPersonaLevel(&p, p.level), next = Levels::ExpForPersonaLevel(&p, p.level + 1);
    if (p.level < 99 && next > cur) {
        char buf[64];
        snprintf(buf, sizeof buf, "%u to next", next > p.exp ? next - p.exp : 0);
        float frac = float(p.exp - (p.exp > cur ? cur : p.exp)) / float(next - cur);
        W::Bar(frac, ImVec2(200 * sc, 0), C::Blue, buf);
    } else {
        W::Dim("max");
    }
    ImGui::TableNextColumn();
    bool maxed = p.level >= 99;
    ImGui::BeginDisabled(maxed);
    if (W::RiskButton("+1 level", Risk::Safe,
                      "Grants exactly the EXP for the next level and runs the game's own level-up: stats grow "
                      "and new skills are learned as after a battle."))
        Levels::LevelUp(t, 1);
    ImGui::SameLine();
    if (W::RiskButton("+5", Risk::Safe, "Five levels, one at a time, through the game's level-up."))
        Levels::LevelUp(t, 5);
    ImGui::SameLine();
    char lbl[32];
    snprintf(lbl, sizeof lbl, "+%d EXP", g_addExp);
    if (W::RiskButton(lbl, Risk::Safe, "Adds EXP the way a battle does; levels gained apply normally."))
        Levels::AddExp(t, uint32_t(g_addExp));
    ImGui::EndDisabled();
    ImGui::PopID();
}

}  // namespace

namespace Pages {

void LevelsExp() {
    if (!RequireSave()) return;
    if (!Levels::Available()) {
        ImGui::TextWrapped("The level-up functions didn't match this game build, so this page is off.");
        return;
    }
    const auto& s = Game::Current();
    float sc = Overlay::Scale();

    SectionHeader("HOW LEVELS WORK");
    W::Dim("Yu has his own level (HP / SP). Every Persona levels separately: Yu's stock Personas each have "
           "a level, and a party member's level IS their Persona's level.");

    SectionHeader("YU");
    const auto& yu = s.U(Game::Yu);
    static int target = 0;
    static int lastLv = -1;
    if (lastLv != yu.level) target = lastLv = yu.level;
    ImGui::Text("Level %u  ·  %u EXP", yu.level, yu.exp);
    ImGui::SetNextItemWidth(160 * sc);
    ImGui::SliderInt("##yulv", &target, 1, 99);
    ImGui::SameLine();
    Risk r = target < yu.level ? Risk::Dangerous : Risk::Caution;
    const char* why = target < yu.level
                          ? "Lowering Yu's level sets his EXP back on the game's curve. HP/SP maximums drop; "
                            "nothing else reverts."
                          : "Sets Yu's level and EXP on the game's own curve. His level also caps which Personas "
                            "he can fuse, so a higher level unlocks stronger fusions early.";
    if (W::RiskButton("Set Yu's level", r, why)) Levels::SetYuLevel(target);

    SectionHeader("EXP");
    ImGui::SetNextItemWidth(160 * sc);
    ImGui::InputInt("EXP to add with \"+EXP\"", &g_addExp, 100, 1000);
    if (g_addExp < 1) g_addExp = 1;
    if (g_addExp > 999999) g_addExp = 999999;

    if (ImGui::BeginTable("lv", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Persona", ImGuiTableColumnFlags_WidthStretch, 2.4f);
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthStretch, 1.6f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 2.6f);
        ImGui::TableHeadersRow();
        for (int i = 0; i < Game::kStock; ++i) {
            if (!s.stock[i].valid) continue;
            char id[16];
            snprintf(id, sizeof id, "s%d", i);
            PersonaRow(id, nullptr, s.stock[i], {false, i});
        }
        for (int c = Game::Yosuke; c <= Game::Teddie; ++c) {
            const auto& u = s.U(c);
            if (!u.persona.id || !s.Flag(JoinFlag(c))) continue;
            char id[16];
            snprintf(id, sizeof id, "m%d", c);
            PersonaRow(id, CharName(c), u.persona, {true, c});
        }
        ImGui::EndTable();
    }

    SectionHeader("BATTLE EXP");
    auto& o = Levels::Opts();
    bool ch = false;
    ImGui::SetNextItemWidth(220 * sc);
    ch |= ImGui::SliderInt("EXP multiplier", &o.expMultiplier, 1, 10, "x%d");
    ch |= ImGui::Checkbox("Benched party members gain EXP too", &o.benchShare);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("After each battle, members who weren't in the active party get the same EXP as "
                          "the ones who fought, levelling up the same way.");
    if (ch) Levels::SaveOptions();
}

}  // namespace Pages
