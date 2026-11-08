#include <imgui.h>
#include <cctype>
#include <string>

#include "Engine/GameState.h"
#include "Engine/Names.h"
#include "Features/BattleOverrides.h"
#include "Features/EnemyIntel.h"
#include "Features/Spoilers.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace {

char g_search[48] = "";

ImU32 AffColor(Intel::Aff a) {
    switch (a) {
        case Intel::Aff::Weak: return C::Red;
        case Intel::Aff::Resist: return C::Blue;
        case Intel::Aff::Null: return C::Grey;
        case Intel::Aff::Repel: return C::Orange;
        case Intel::Aff::Drain: return C::Green;
        default: return IM_COL32(0, 0, 0, 0);
    }
}

// One row of element cells: Wk / Rs / Nu / Rp / Dr, coloured.
void AffRow(uint16_t id, int first, int count, float cell) {
    for (int e = first; e < first + count; ++e) {
        if (e > first) ImGui::SameLine(0, 2);
        Intel::Aff a = Intel::Affinity(id, e);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImVec2 sz(cell, ImGui::GetTextLineHeight() + 4);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y),
                          a == Intel::Aff::Normal ? IM_COL32(255, 255, 255, 18) : AffColor(a));
        const char* t = Intel::AffShort(a);
        ImVec2 ts = ImGui::CalcTextSize(t);
        dl->AddText(ImVec2(p.x + (sz.x - ts.x) * 0.5f, p.y + 2),
                    a == Intel::Aff::Normal ? C::Grey : C::Black, t);
        ImGui::Dummy(sz);
    }
}

void HeaderRow(int first, int count, float cell, bool ailments) {
    ImGui::PushFont(g_fonts.note);
    for (int e = first; e < first + count; ++e) {
        if (e > first) ImGui::SameLine(0, 2);
        const char* n = ailments ? Intel::AilmentName(e - 8) : Intel::ElementName(e);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImVec2 ts = ImGui::CalcTextSize(n);
        float x = p.x + (cell - ts.x) * 0.5f;
        ImGui::GetWindowDrawList()->AddText(ImVec2(x < p.x ? p.x : x, p.y), C::Grey, n);
        ImGui::Dummy(ImVec2(cell, ts.y));
    }
    ImGui::PopFont();
}

}  // namespace

namespace Pages {

void EnemyIntel() {
    if (!RequireSave()) return;
    if (!Intel::Available()) {
        ImGui::TextWrapped("The enemy tables aren't loaded yet (they load with the first battle data) or "
                           "didn't match this game build.");
    }
    float sc = Overlay::Scale();
    auto& f = BattleFx::Opts();
    SectionHeader("IN THE GAME");
    if (ImGui::Checkbox("Reveal every enemy's affinities", &f.revealAffinities)) BattleFx::SaveOptions();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("The battle screen and Rise's Analysis show every affinity as if already "
                          "analysed. Nothing is saved; switch it off and the game shows what you've "
                          "actually found again.");
    if (ImGui::Checkbox("Show the intel panel during battles", &Intel::ShowHud())) Intel::SaveOptions();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("A small panel in the corner listing each enemy's weaknesses and resistances, "
                          "visible while this menu is closed.");

    // Current battle
    auto foes = Intel::CurrentEnemies();
    if (!foes.empty()) {
        SectionHeader("THIS BATTLE");
        float cell = 64 * sc;
        ImGui::Dummy(ImVec2(200 * sc, 0));
        ImGui::SameLine();
        HeaderRow(0, 8, cell, false);
        for (const auto& e : foes) {
            ImGui::PushID(e.id);
            std::string n = Names::Enemy(e.id);
            ImGui::Text("%-22s", n.empty() ? "?" : n.c_str());
            ImGui::SameLine(200 * sc + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().WindowPadding.x);
            AffRow(e.id, 0, 8, cell);
            ImGui::PopID();
        }
    }

    SectionHeader("SHADOW LIST");
    ImGui::SetNextItemWidth(260 * sc);
    ImGui::InputTextWithHint("##search", "Search shadows", g_search, sizeof g_search);
    W::Dim("Shadows you haven't encountered are hidden; reveal them one at a time.");
    float cell = 52 * sc;
    if (ImGui::BeginChild("list", ImVec2(0, 0), ImGuiChildFlags_None)) {
        ImGui::Dummy(ImVec2(220 * sc, 0));
        ImGui::SameLine();
        HeaderRow(0, 8, cell, false);
        ImGui::SameLine(0, 14 * sc);
        HeaderRow(8, 8, cell, true);
        for (uint16_t id = 1; id < 368; ++id) {
            std::string n = Names::Enemy(id);
            if (n.empty()) continue;
            bool seen = Intel::Seen(id);
            if (g_search[0]) {
                if (!Spoilers::Visible(Spoilers::Kind::Enemy, id, seen)) continue;
                std::string a = n, b = g_search;
                for (auto& c : a) c = (char)std::tolower((unsigned char)c);
                for (auto& c : b) c = (char)std::tolower((unsigned char)c);
                if (a.find(b) == std::string::npos) continue;
            }
            ImGui::PushID(id);
            if (!Spoilers::Visible(Spoilers::Kind::Enemy, id, seen)) {
                Spoilers::HiddenPlaceholder(Spoilers::Kind::Enemy, id, "not encountered");
            } else {
                ImGui::TextUnformatted(n.c_str());
                ImGui::SameLine(220 * sc + ImGui::GetStyle().ItemSpacing.x);
                AffRow(id, 0, 8, cell);
                ImGui::SameLine(0, 14 * sc);
                AffRow(id, 8, 8, cell);
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

// Small always-on panel during battle (drawn while the trainer menu is closed).
void IntelHud() {
    auto foes = Intel::CurrentEnemies();
    if (foes.empty()) return;
    float sc = Overlay::Scale();
    float cell = 34 * sc;
    ImGui::SetNextWindowPos(ImVec2(12 * sc, 140 * sc), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.78f);
    ImGuiWindowFlags fl = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                          ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing;
    if (ImGui::Begin("##intelhud", nullptr, fl)) {
        ImGui::PushFont(g_fonts.note);
        ImGui::Dummy(ImVec2(130 * sc, 0));
        ImGui::SameLine();
        static const char* el[] = {"Ph", "Fi", "Ic", "El", "Wi", "Al", "Li", "Dk"};
        for (int e = 0; e < 8; ++e) {
            if (e) ImGui::SameLine(0, 2);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImVec2 ts = ImGui::CalcTextSize(el[e]);
            ImGui::GetWindowDrawList()->AddText(ImVec2(p.x + (cell - ts.x) * 0.5f, p.y), C::Grey, el[e]);
            ImGui::Dummy(ImVec2(cell, ts.y));
        }
        for (const auto& e : foes) {
            ImGui::PushID(e.id);
            std::string n = Names::Enemy(e.id);
            if (n.size() > 16) n = n.substr(0, 15) + "…";
            ImGui::Text("%s", n.c_str());
            ImGui::SameLine(130 * sc + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().WindowPadding.x);
            AffRow(e.id, 0, 8, cell);
            ImGui::PopID();
        }
        ImGui::PopFont();
    }
    ImGui::End();
}

}  // namespace Pages
