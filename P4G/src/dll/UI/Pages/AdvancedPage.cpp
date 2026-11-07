#include <imgui.h>

#include "Features/Journal.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace Pages {

void Advanced() {
    SectionHeader("CHANGE JOURNAL");
    const auto& e = Journal::Entries();
    if (e.empty()) {
        W::Dim("Nothing changed yet this session. Every change the trainer makes is listed here and "
               "can be undone.");
    } else {
        if (ImGui::Button("Undo everything this session")) Journal::UndoAll();
        if (ImGui::BeginTable("journal", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("When", ImGuiTableColumnFlags_WidthStretch, 1.4f);
            ImGui::TableSetupColumn("Change", ImGuiTableColumnFlags_WidthStretch, 4.0f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.8f);
            ImGui::TableHeadersRow();
            for (auto it = e.rbegin(); it != e.rend(); ++it) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                W::Dim("%s", it->when.c_str());
                ImGui::TableNextColumn();
                if (it->undone) ImGui::TextDisabled("%s (undone)", it->what.c_str());
                else ImGui::TextUnformatted(it->what.c_str());
                ImGui::TableNextColumn();
                if (!it->undone) {
                    ImGui::PushID(it->id);
                    if (ImGui::SmallButton("Undo")) Journal::Undo(it->id);
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
    }
    SectionHeader("FLAGS & COUNTERS");
    W::Dim("The labelled flag and counter browser arrives in milestone M4.");
}

}  // namespace Pages
