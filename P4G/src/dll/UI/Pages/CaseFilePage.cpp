#include <imgui.h>

#include "Data/Cases.h"
#include "Engine/Calendar.h"
#include "Engine/GameState.h"
#include "Features/CaseFile.h"
#include "Features/Spoilers.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace Pages {

void CaseFile() {
    if (!RequireSave()) return;
    const auto& s = Game::Current();
    auto st = ::CaseFile::Current(s);
    float sc = Overlay::Scale();

    SectionHeader("CURRENT CASE");
    if (st.state == ::CaseFile::State::None) {
        ImGui::TextWrapped("No case is pending right now.");
    } else {
        const auto& c = Data::kCases[st.index];
        bool known = st.state != ::CaseFile::State::Upcoming;
        if (!Spoilers::Visible(Spoilers::Kind::Case, st.index, known)) {
            ImGui::TextWrapped("The next case hasn't started yet.");
            Spoilers::HiddenPlaceholder(Spoilers::Kind::Case, st.index, "who and where");
        } else {
            W::BeginCard("case", ImVec2(0, 150 * sc), st.daysLeft <= 3 ? C::Red : C::Yellow);
            auto last = Calendar::FromIndex(c.lastDay);
            ImGui::PushFont(g_fonts.title);
            ImGui::Text("%s", c.victim);
            ImGui::PopFont();
            ImGui::Text("Dungeon: %s", c.dungeon);
            if (st.state == ::CaseFile::State::Upcoming) {
                auto from = Calendar::FromIndex(c.accessDay);
                ImGui::Text("Opens %d/%d. Last day %d/%d (%s).", from.month, from.day, last.month,
                            last.day, Calendar::WeekdayShort(last.weekday));
            } else {
                ImGui::PushFont(g_fonts.bold);
                ImU32 col = st.daysLeft <= 3 ? C::Red : C::Yellow;
                if (st.state == ::CaseFile::State::LastDay)
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(col), "TODAY IS THE LAST DAY");
                else
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(col), "%d day%s left", st.daysLeft,
                                       st.daysLeft == 1 ? "" : "s");
                ImGui::PopFont();
                ImGui::SameLine();
                ImGui::Text("— last day %d/%d (%s)%s", last.month, last.day,
                            Calendar::WeekdayShort(last.weekday),
                            c.gameOver ? ", game over that night if the dungeon isn't cleared" : "");
                W::Dim(st.todayUsable ? "You can still go into the TV today."
                                      : "The TV world is closed for today (daytime and after school only).");
            }
            if (!st.rescueKnown)
                W::Dim("This case's \"cleared\" flag isn't identified yet, so if you've already cleared "
                       "the dungeon, the countdown above doesn't apply.");
            else if (!st.verified)
                W::Dim("Cleared status comes from a flag that is still being verified.");
            W::EndCard();
        }
    }

    SectionHeader("TIMELINE");
    if (ImGui::BeginTable("cases", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Case", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Dungeon", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Last day", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 1.4f);
        ImGui::TableHeadersRow();
        for (int i = 0; i < Data::kCaseCount; ++i) {
            const auto& c = Data::kCases[i];
            bool solved = ::CaseFile::Solved(s, i);
            bool current = i == st.index && st.state != ::CaseFile::State::Upcoming;
            bool visible = Spoilers::Visible(Spoilers::Kind::Case, i, solved || current);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID(i);
            if (visible) ImGui::TextUnformatted(c.victim);
            else Spoilers::HiddenPlaceholder(Spoilers::Kind::Case, i);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(visible ? c.dungeon : "");
            ImGui::TableNextColumn();
            if (visible) {
                auto d = Calendar::FromIndex(c.lastDay);
                ImGui::Text("%d/%d", d.month, d.day);
            }
            ImGui::TableNextColumn();
            if (solved) W::Chip("SOLVED", C::Green);
            else if (current) W::Chip(st.daysLeft <= 3 ? "URGENT" : "ACTIVE", st.daysLeft <= 3 ? C::Red : C::Yellow);
            else W::Dim("not yet");
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    W::Dim("Dates from community research. A case whose deadline has passed is solved — otherwise "
           "the game would have ended.");
}

}  // namespace Pages
