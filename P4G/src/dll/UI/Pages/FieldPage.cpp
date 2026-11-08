#include <imgui.h>

#include "Features/FieldQol.h"
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
    ImGui::BeginDisabled(!dungeon);
    if (W::RiskButton("Next floor", Risk::Caution,
                      "Moves up one floor, as the stairs would. Floors you skip aren't explored or looted."))
        FieldQol::GotoRelativeFloor(1);
    ImGui::SameLine();
    if (W::RiskButton("Previous floor", Risk::Caution, "Moves down one floor."))
        FieldQol::GotoRelativeFloor(-1);
    ImGui::EndDisabled();
    ImGui::EndDisabled();

    SectionHeader("NOT INCLUDED ON PURPOSE");
    W::Dim("Changing the date, repeating days, unlocking the whole town map and calling arbitrary events "
           "would break the game's calendar and story scripts, so they aren't offered.");
}

}  // namespace Pages
