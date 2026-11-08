#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Engine/GameState.h"
#include "Features/Edits.h"
#include "Features/FlagLabels.h"
#include "Features/Journal.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace {

bool g_counters = false;      // which list
char g_find[48] = "";
bool g_onlyLabelled = true;
bool g_onlyOn = false;
bool g_onlyChanged = false;
bool g_showSpoilers = false;
int g_pageStart = 0;
std::vector<uint8_t> g_baseFlags;      // as they were when the browser was first shown
std::vector<uint32_t> g_baseCounters;
int g_editCounter = -1;
int g_editValue = 0;

bool Matches(int n, const std::string& label) {
    if (!g_find[0]) return true;
    char num[16];
    snprintf(num, sizeof num, "%d", n);
    if (strstr(num, g_find)) return true;
    std::string a = label, b = g_find;
    for (auto& c : a) c = (char)tolower((unsigned char)c);
    for (auto& c : b) c = (char)tolower((unsigned char)c);
    return a.find(b) != std::string::npos;
}

void Baseline(const Game::Snapshot& s) {
    g_baseFlags.assign(s.flags, s.flags + sizeof s.flags);
    g_baseCounters.assign(s.counters, s.counters + Game::kCounters);
}

void DrawFlags(const Game::Snapshot& s) {
    float sc = Overlay::Scale();
    std::vector<int> rows;
    for (int n = 0; n < Game::kFlags; ++n) {
        const auto* l = FlagLabels::Flag(n);
        if (g_onlyLabelled && !l) continue;
        bool on = s.Flag(n);
        if (g_onlyOn && !on) continue;
        bool changed = !g_baseFlags.empty() && (((g_baseFlags[n >> 3] >> (n & 7)) & 1) != on);
        if (g_onlyChanged && !changed) continue;
        std::string text = l ? ((l->spoiler && !g_showSpoilers) ? std::string("(hidden: may spoil the story)") : l->text)
                             : std::string();
        if (!Matches(n, text)) continue;
        rows.push_back(n);
    }
    constexpr int kPer = 60;
    if (g_pageStart >= int(rows.size())) g_pageStart = 0;
    W::Dim("%zu flag(s) match.", rows.size());
    if (rows.size() > size_t(kPer)) {
        ImGui::SameLine();
        if (ImGui::SmallButton("< Prev")) g_pageStart = g_pageStart >= kPer ? g_pageStart - kPer : 0;
        ImGui::SameLine();
        W::Dim("%d-%d", g_pageStart + 1, std::min<int>(g_pageStart + kPer, int(rows.size())));
        ImGui::SameLine();
        if (ImGui::SmallButton("Next >") && g_pageStart + kPer < int(rows.size())) g_pageStart += kPer;
    }
    if (!ImGui::BeginTable("flags", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) return;
    ImGui::TableSetupColumn("Flag", 0, 0.7f);
    ImGui::TableSetupColumn("State", 0, 0.7f);
    ImGui::TableSetupColumn("Meaning", 0, 4.0f);
    ImGui::TableSetupColumn("", 0, 1.0f);
    ImGui::TableHeadersRow();
    for (int k = g_pageStart; k < int(rows.size()) && k < g_pageStart + kPer; ++k) {
        int n = rows[k];
        bool on = s.Flag(n);
        bool changed = !g_baseFlags.empty() && (((g_baseFlags[n >> 3] >> (n & 7)) & 1) != on);
        const auto* l = FlagLabels::Flag(n);
        ImGui::PushID(n);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%d", n);
        ImGui::TableNextColumn();
        W::Chip(on ? "ON" : "off", on ? C::Green : C::PanelHi, on ? C::Black : C::Grey);
        if (changed) {
            ImGui::SameLine();
            W::Chip("CHANGED", C::Orange);
        }
        ImGui::TableNextColumn();
        if (!l) W::Dim("—");
        else if (l->spoiler && !g_showSpoilers) W::Dim("(hidden: may spoil the story)");
        else ImGui::TextUnformatted(l->text.c_str());
        ImGui::TableNextColumn();
        if (W::RiskButton(on ? "Turn off" : "Turn on", Risk::Dangerous,
                          "A raw story flag. Scripts read these to decide what happens next; the wrong one "
                          "can skip or repeat events. Undo is in the journal above."))
            Edits::SetFlag(n, !on);
        ImGui::PopID();
    }
    ImGui::EndTable();
    (void)sc;
}

void DrawCounters(const Game::Snapshot& s) {
    float sc = Overlay::Scale();
    if (!ImGui::BeginTable("counters", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) return;
    ImGui::TableSetupColumn("Counter", 0, 0.7f);
    ImGui::TableSetupColumn("Value", 0, 1.2f);
    ImGui::TableSetupColumn("Meaning", 0, 3.0f);
    ImGui::TableSetupColumn("", 0, 2.0f);
    ImGui::TableHeadersRow();
    for (int n = 0; n < Game::kCounters; ++n) {
        uint32_t v = s.counters[n];
        bool changed = !g_baseCounters.empty() && g_baseCounters[n] != v;
        const char* l = FlagLabels::Counter(n);
        if (g_onlyLabelled && !l && !v) continue;  // labelled or non-zero
        if (g_onlyChanged && !changed) continue;
        if (!Matches(n, l ? l : "")) continue;
        ImGui::PushID(n);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%d", n);
        ImGui::TableNextColumn();
        ImGui::Text("%u", v);
        if (changed) {
            ImGui::SameLine();
            W::Chip("CHANGED", C::Orange);
        }
        ImGui::TableNextColumn();
        if (l) ImGui::TextUnformatted(l);
        else W::Dim("—");
        ImGui::TableNextColumn();
        if (g_editCounter == n) {
            ImGui::SetNextItemWidth(120 * sc);
            ImGui::InputInt("##v", &g_editValue);
            ImGui::SameLine();
            if (W::RiskButton("Set", Risk::Dangerous, "A raw story counter. Undo is in the journal above.")) {
                Edits::SetCounter(n, uint32_t(g_editValue < 0 ? 0 : g_editValue));
                g_editCounter = -1;
            }
        } else if (ImGui::SmallButton("Edit")) {
            g_editCounter = n;
            g_editValue = int(v);
        }
        ImGui::PopID();
    }
    ImGui::EndTable();
}

void DrawBrowser() {
    if (!Game::Current().valid) {
        W::Dim("Load a save to browse its flags and counters.");
        return;
    }
    const auto& s = Game::Current();
    if (g_baseFlags.empty()) Baseline(s);
    float sc = Overlay::Scale();
    W::Dim("The game's own event flags and counters, read live. %d flags have a known meaning; the rest are "
           "listed by number. Changes go through the game's own flag functions and can be undone above.",
           FlagLabels::LabelledFlagCount());
    if (W::RiskButton(g_counters ? "Show flags" : "Show counters", Risk::Safe, "Switch between the two lists."))
        g_counters = !g_counters;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220 * sc);
    ImGui::InputTextWithHint("##find", "Number or words", g_find, sizeof g_find);
    ImGui::SameLine();
    ImGui::Checkbox(g_counters ? "Labelled or non-zero only" : "Labelled only", &g_onlyLabelled);
    if (!g_counters) {
        ImGui::SameLine();
        ImGui::Checkbox("On only", &g_onlyOn);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Changed only", &g_onlyChanged);
    ImGui::SameLine();
    if (ImGui::SmallButton("Mark current as unchanged")) Baseline(s);
    if (!g_counters) {
        ImGui::Checkbox("Show labels that may spoil the story", &g_showSpoilers);
    }
    ImGui::Spacing();
    if (g_counters) DrawCounters(s);
    else DrawFlags(s);
}

}  // namespace

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
    DrawBrowser();
}

}  // namespace Pages
