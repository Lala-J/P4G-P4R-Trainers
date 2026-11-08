#include <imgui.h>
#include <cstdio>

#include "Data/Stats.h"
#include "Engine/GameState.h"
#include "Engine/Names.h"
#include "Features/Edits.h"
#include "Features/PersonaStock.h"
#include "Features/Spoilers.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace {

constexpr uint16_t kOkami = 189;  // Izanagi-no-Okami

// Points that reach the top rank of each stat. Diligence's last step is reported as 130 or 140;
// 140 reaches it either way.
constexpr uint16_t kMaxPoints[5] = {140, 240, 140, 140, 85};

void Card(const char* title, const char* carried) {
    ImGui::Spacing();
    W::Title(title);
    ImGui::SameLine();
    W::Chip(carried, C::PanelHi, C::Grey);
}

}  // namespace

namespace Pages {

void NgPlus() {
    if (!RequireSave()) return;
    const auto& s = Game::Current();
    W::Dim("Starting New Game+ carries some things over from a finished playthrough. Each of them can be "
           "granted here on its own, without the rest. None of this changes the story's own flags.");

    SectionHeader("CARRIED OVER BY NEW GAME+");

    Card("Social stats", "CARRIED OVER");
    const auto& yu = s.U(Game::Yu);
    bool allMax = true;
    for (int i = 0; i < 5; ++i) {
        int r = Data::StatRank(i, yu.social[i]);
        if (i) ImGui::SameLine(0, 18 * Overlay::Scale());
        ImGui::Text("%s: %s", Data::kStats[i].name, Data::kStats[i].ranks[r - 1]);
        allMax &= r >= 5;
    }
    ImGui::BeginDisabled(allMax);
    if (W::RiskButton("Max all five", Risk::Caution,
                      "Raises Courage, Knowledge, Diligence, Understanding and Expression to their top rank. "
                      "Stat-gated choices, links and jobs open up immediately."))
        for (int i = 0; i < 5; ++i)
            if (yu.social[i] < kMaxPoints[i]) Edits::SetSocialStat(i, kMaxPoints[i]);
    ImGui::EndDisabled();

    Card("Money", "CARRIED OVER");
    ImGui::Text("¥%u", s.yen);
    ImGui::SameLine();
    if (W::RiskButton("Set to ¥9,999,999", Risk::Caution, "The most the game can hold."))
        Edits::SetYen(Edits::kMaxYen);

    Card("Persona compendium", "CARRIED OVER");
    if (!PersonaStock::Available()) {
        W::Dim("Unavailable on this game build (a host check failed).");
    } else {
        int n = PersonaStock::CompendiumGaps();
        if (n == 0) {
            W::Dim("Every Persona is registered.");
        } else {
            W::Dim("%d Personas aren't registered yet (arcana you haven't met are left out).", n);
            if (W::RiskButton("Register every Persona (base forms)", Risk::Caution,
                              "Adds each missing Persona to the compendium as it first appears (base level, "
                              "stats and skills), using the game's own register. Summoning still costs the "
                              "usual fee. Registered entries you already have are left as they are."))
                PersonaStock::FillCompendium();
        }
    }

    Card("Items and equipment", "CARRIED OVER");
    W::Dim("Weapons, armour, accessories and consumables carry over (key items don't). Set any of them on "
           "the Inventory page.");

    SectionHeader("NEW GAME+ ONLY");
    bool known = PersonaStock::SlotOf(kOkami) >= 0 || PersonaStock::Registered(kOkami);
    if (!Spoilers::Visible(Spoilers::Kind::Persona, kOkami, known)) {
        Spoilers::HiddenPlaceholder(Spoilers::Kind::Persona, kOkami, "a Persona only New Game+ unlocks");
    } else {
        std::string nm = Names::PersonaOr(kOkami);
        W::Title(nm.c_str());
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Orange),
                           "Normally fused only in New Game+, after every social link is maxed. Getting it now "
                           "skips both requirements; nothing else in the story changes.");
        ImGui::PopTextWrapPos();
        const char* why = PersonaStock::WhyNotGive(kOkami);
        ImGui::BeginDisabled(why != nullptr);
        char label[96];
        snprintf(label, sizeof label, "Get %s (base form)", nm.c_str());
        if (W::RiskButton(label, Risk::Dangerous,
                          "Adds it to Yu's stock exactly as a fusion would create it, at its base level."))
            PersonaStock::Give(kOkami);
        ImGui::EndDisabled();
        if (why) W::Dim("%s", why);
    }

    SectionHeader("NOT CARRIED OVER");
    W::Dim("Social links, Yu's level, the Persona stock, key items and the calendar always start fresh. "
           "The trainer doesn't mark a playthrough as New Game+: the story checks that in many places.");
}

}  // namespace Pages
