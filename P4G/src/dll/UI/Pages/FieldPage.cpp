#include <imgui.h>
#include <cstdio>
#include <cstring>

#include "Features/AutoAdvance.h"
#include "Features/FieldGuard.h"
#include "Features/FieldQol.h"
#include "Hooks/InputHooks.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace Pages {

void FieldDungeon() {
    if (!RequireSave()) return;
    const char* why = nullptr;
    bool can = FieldQol::CanRunNow(&why);
    if (!can) {
        W::Chip("NOT NOW", C::Orange);
        ImGui::SameLine();
        ImGui::TextUnformatted(why);
    }
    ImGui::BeginDisabled(!can);

    SectionHeader("SHOPS & THE VELVET ROOM");
    W::Dim("In town: opens the facility with the same steps the town's own scripts use, then brings you back "
           "to this area's main entrance. No time passes. Close this menu to use it.");
    bool dungeon = FieldQol::InDungeon();
    if (dungeon) W::Dim("Not available inside the TV world — use the Velvet Room door at the entrance.");
    ImGui::BeginDisabled(dungeon);
    using F = FieldQol::Facility;
    const F shops[] = {F::Velvet, F::Daidara, F::Shiroku, F::Crocofur, F::Books, F::Produce};
    int n = 0;
    for (F f : shops) {
        if (n++ % 3) ImGui::SameLine();
        ImGui::PushID(int(f));
        if (W::RiskButton(FieldQol::FacilityName(f), Risk::Caution,
                          "Opens it here instead of walking there. Shops sell their normal stock for today."))
            FieldQol::OpenFacility(f);
        ImGui::PopID();
    }

    ImGui::EndDisabled();

    SectionHeader("SAVE");
    if (W::RiskButton("Save anywhere", Risk::Caution, "Opens the save screen the way a save point does."))
        FieldQol::SaveAnywhere();

    SectionHeader("DUNGEON");
    W::Dim("While exploring a dungeon:");
    if (FieldGuard::Rescues() || FieldGuard::Fired())
        W::Dim("Crash guard: the party was moved back onto the floor %d time(s) this session (some staircases "
               "let you walk through their back end, which crashes the unmodified game).",
               FieldGuard::Rescues());
    const char* whyUp = FieldQol::WhyNotChangeFloor(1);
    const char* whyDown = FieldQol::WhyNotChangeFloor(-1);
    ImGui::BeginDisabled(whyUp != nullptr);
    if (W::RiskButton("Next floor", Risk::Caution,
                      "Goes up one floor with the same steps as the stairs (fade out, then the floor change). "
                      "Floors you skip aren't explored or looted."))
        FieldQol::GotoRelativeFloor(1);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(whyDown != nullptr);
    if (W::RiskButton("Previous floor", Risk::Caution, "Goes down one floor, as the stairs down would."))
        FieldQol::GotoRelativeFloor(-1);
    ImGui::EndDisabled();
    ImGui::SameLine();
    const char* whyEsc = FieldQol::WhyNotEscape();
    ImGui::BeginDisabled(whyEsc != nullptr);
    if (W::RiskButton("Return to the entrance", Risk::Caution,
                      "Leaves the dungeon the way a Goho-M does (same effect, no item used). The entrance then "
                      "offers to continue from the floor you left, as after a Goho-M."))
        FieldQol::ReturnToEntrance();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!dungeon);
    if (W::RiskButton("Fox: restore SP", Risk::Caution,
                      "What the Fox does on a dungeon floor (a white flash, everyone's SP back to full), without "
                      "the fee."))
        FieldQol::FoxRecovery();
    ImGui::EndDisabled();
    if (dungeon && whyDown && !whyUp) W::Dim("%s", whyDown);
    if (dungeon && FieldQol::CurrentFloorIdCached() >= 0) W::Dim("Floor id %d.", FieldQol::CurrentFloorIdCached());

    SectionHeader("PARTY");
    W::Dim("Who comes along (Yu plus up to three). Uses the game's own party functions, the ones entering "
           "the TV world uses. Only members who have joined are listed.");
    static const char* who[] = {"", "Yu", "Yosuke", "Chie", "Yukiko", "Rise", "Kanji", "Naoto", "Teddie"};
    int listed = 0;
    for (int c = 2; c <= 8; ++c) {
        if (!FieldQol::PartyAvailable(c)) continue;
        if (listed++) ImGui::SameLine();
        bool in = FieldQol::InParty(c);
        const char* why = FieldQol::WhyNotParty(c, !in);
        ImGui::PushID(200 + c);
        ImGui::BeginDisabled(why != nullptr);
        bool v = in;
        if (ImGui::Checkbox(who[c], &v)) FieldQol::SetInParty(c, v);
        ImGui::EndDisabled();
        if (why && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", why);
        ImGui::PopID();
    }
    if (!listed) W::Dim("No one has joined yet.");
    ImGui::EndDisabled();

    SectionHeader("MINIMAP");
    if (!FieldQol::RevealMapAvailable()) {
        W::Dim("Unavailable on this game build (a host check failed).");
    } else {
        bool on = FieldQol::RevealMapEnabled();
        if (ImGui::Checkbox("Reveal the whole floor on the minimap", &on)) FieldQol::SetRevealMap(on);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Each floor's map fills in as soon as you arrive, as if you had already walked "
                              "every corridor and room. Nothing else changes: chests and stairs are where "
                              "they would be anyway, and turning it off leaves the next floor unexplored.");
        int total = 0, shown = FieldQol::RevealedCells(&total);
        if (shown >= 0 && total > 0) W::Dim("This floor: %d of %d areas on the map.", shown, total);
        else if (dungeon) W::Dim("This floor doesn't use the generated map (fixed floors are drawn in full anyway).");
        else W::Dim("Applies inside the TV world's generated floors.");
    }

    SectionHeader("DIALOGUE AUTO-ADVANCE");
    {
        auto& o = AutoAdvance::Opts();
        bool ch = ImGui::Checkbox("Advance dialogue automatically", &o.enabled);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Only the conversation box: never choices, menus, tutorials or pop-ups. It "
                              "presses Confirm for you once the page is fully shown and the voice has finished.");
        float sc = Overlay::Scale();
        ImGui::SetNextItemWidth(260 * sc);
        ch |= ImGui::SliderInt("After a voiced line", &o.voicedDelayMs, 0, 6000, "%d ms");
        ImGui::SetNextItemWidth(260 * sc);
        ch |= ImGui::SliderInt("Unvoiced line", &o.unvoicedDelayMs, 300, 8000, "%d ms");
        ImGui::SetNextItemWidth(260 * sc);
        ch |= ImGui::SliderInt("System text in the box", &o.systemDelayMs, 300, 8000, "%d ms");
        if (ch) AutoAdvance::SaveOptions();
        ImGui::SetNextItemWidth(260 * sc);
        if (ImGui::BeginCombo("On/off key", AutoAdvance::KeyName(o.keyDik))) {
            int nk = 0;
            const AutoAdvance::KeyChoice* keys = AutoAdvance::KeyChoices(&nk);
            for (int i = 0; i < nk; ++i) {
                int dik = keys[i].dik;
                bool taken = dik && (Input::GameUsesKeyInDialogue(dik) || dik == Input::ToggleKeyDik());
                bool cur = o.keyDik == dik;
                ImGui::PushID(i);
                ImGui::BeginDisabled(taken && !cur);
                if (ImGui::Selectable(keys[i].name, cur) && !taken) AutoAdvance::SetKey(dik);
                ImGui::EndDisabled();
                if (taken && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip(dik == Input::ToggleKeyDik() ? "This key opens and closes the trainer."
                                                                   : "The game uses this key in conversations.");
                if (cur) ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Turns auto-advance on or off while a conversation is on screen. The key is "
                              "only taken during conversations, so everywhere else it does what it always "
                              "does (A, for example, still walks left).");
        ImGui::SetNextItemWidth(260 * sc);
        if (ImGui::BeginCombo("On/off button", AutoAdvance::PadName(o.padButton))) {
            int nb = 0;
            const AutoAdvance::KeyChoice* buttons = AutoAdvance::PadChoices(&nb);
            uint32_t combo = Input::PadComboVita();
            for (int i = 0; i < nb; ++i) {
                uint32_t bit = uint32_t(buttons[i].dik);
                bool inCombo = (bit & combo) != 0;
                bool taken = bit && (Input::GameUsesPadInDialogue(bit) || inCombo);
                bool cur = uint32_t(o.padButton) == bit;
                ImGui::PushID(1000 + i);
                ImGui::BeginDisabled(taken && !cur);
                if (ImGui::Selectable(buttons[i].name, cur) && !taken) AutoAdvance::SetPad(int(bit));
                ImGui::EndDisabled();
                if (taken && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip(inCombo ? "Part of the button combo that opens the trainer."
                                              : "The game uses this button in conversations.");
                if (cur) ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("The same on a controller: Xbox names first, then PlayStation. Works with any "
                              "controller the game itself reads, and the button is only taken during "
                              "conversations.");
        if (o.keyDik || o.padButton) {
            char how[96];
            if (o.keyDik && o.padButton)
                snprintf(how, sizeof how, "%s (or %s on a controller)", AutoAdvance::KeyName(o.keyDik),
                         AutoAdvance::PadName(o.padButton));
            else
                snprintf(how, sizeof how, "%s", o.keyDik ? AutoAdvance::KeyName(o.keyDik)
                                                         : AutoAdvance::PadName(o.padButton));
            W::Dim("Press %s during a conversation to turn it on or off. While it's on, \"Autoplay On\" "
                   "shows above the conversation box.", how);
        }
        W::Dim("Now: %s. Holding any key or button pauses it.", AutoAdvance::Status());
        // While the trainer is open autoplay is paused, so also show what it was doing just before:
        // if it ever seems stuck, this is what to report.
        const char* before = AutoAdvance::StatusBeforeTrainer();
        if (o.enabled && *AutoAdvance::StatusDetail() && strcmp(before, "No dialogue") != 0) {
            W::Dim("Before you opened the trainer: %s.", before);
            W::Dim("Details: %s", AutoAdvance::StatusDetail());
        }
    }

    SectionHeader("NOT INCLUDED ON PURPOSE");
    W::Dim("Changing the date, repeating days, unlocking the whole town map and calling arbitrary events "
           "would break the game's calendar and story scripts, so they aren't offered.");
}

}  // namespace Pages
