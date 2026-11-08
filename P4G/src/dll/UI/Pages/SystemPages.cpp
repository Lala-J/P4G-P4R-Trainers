#include <imgui.h>
#include <cstdio>
#include <string>

#include "Core/Trainer.h"
#include "Data/Cases.h"
#include "Data/Links.h"
#include "Data/Stats.h"
#include "Engine/Calendar.h"
#include "Engine/GameState.h"
#include "Engine/HostCheck.h"
#include "Features/CaseFile.h"
#include "Features/Spoilers.h"
#include "Hooks/Hook.h"
#include "Hooks/InputHooks.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"
#include "Util/Paths.h"
#include "Util/Settings.h"

using namespace Theme;

namespace Pages {

bool RequireSave() {
    const auto& s = Game::Current();
    if (!HostCheck::Get().supported) {
        ImGui::TextWrapped("This game build isn't supported, so nothing is read from it.");
        return false;
    }
    if (!s.valid) {
        ImGui::TextWrapped("Load a save (or start a new game) and this page fills in.");
        return false;
    }
    return true;
}

void Dashboard() {
    const auto& s = Game::Current();
    float sc = Overlay::Scale();
    if (HostCheck::Get().supported && s.valid) {
        auto d = Calendar::FromIndex(s.day);
        SectionHeader("TODAY");
        ImGui::PushFont(g_fonts.title);
        ImGui::Text("%d/%d (%s)  %s", d.month, d.day, Calendar::WeekdayShort(d.weekday),
                    Calendar::TimeName(s.time));
        ImGui::PopFont();
        ImGui::Text("¥%u   ·   Yu Lv %u   ·   %s", s.yen, s.U(Game::Yu).level, Game::ModeName(s.mode));

        auto st = CaseFile::Current(s);
        if (st.state == CaseFile::State::Active || st.state == CaseFile::State::LastDay) {
            const auto& c = Data::kCases[st.index];
            ImU32 col = st.daysLeft <= 3 ? C::Red : C::Yellow;
            W::Chip(st.state == CaseFile::State::LastDay ? "LAST DAY" : "CASE", col);
            ImGui::SameLine();
            if (st.state == CaseFile::State::LastDay)
                ImGui::Text("%s — today is the deadline", c.dungeon);
            else
                ImGui::Text("%s — %d day%s left", c.dungeon, st.daysLeft, st.daysLeft == 1 ? "" : "s");
        } else {
            W::Chip("CASE", C::Green);
            ImGui::SameLine();
            ImGui::TextUnformatted("No deadline pending right now.");
        }

        SectionHeader("LINKS READY TO RANK UP");
        int ready = 0;
        for (const auto& l : s.links) {
            if (!l.id || !l.rank || l.rank >= 10) continue;
            uint16_t t = Game::LinkThreshold(l.id, l.rank);
            if (l.points < t) continue;
            const auto* info = Data::Link(Data::BaseRow(l.id));
            if (!info || info->kind != Data::LinkKind::Normal) continue;
            ImGui::BulletText("%s (rank %u → %u at the next hangout)", info->person, l.rank, l.rank + 1);
            ++ready;
        }
        if (!ready) W::Dim("None right now. The Social Links page shows how close each one is.");

        SectionHeader("STATS");
        const auto& yu = s.U(Game::Yu);
        for (int i = 0; i < 5; ++i) {
            if (i) ImGui::SameLine(0, 24 * sc);
            int r = Data::StatRank(i, yu.social[i]);
            ImGui::Text("%s: %s", Data::kStats[i].name, Data::kStats[i].ranks[r - 1]);
        }
    } else {
        SectionHeader("WELCOME");
        ImGui::TextWrapped("Load a save and this page shows today's date, the case deadline, links that "
                           "are ready to rank up and your stats.");
    }

    SectionHeader("TRAINER");
    const auto& hc = HostCheck::Get();
    if (hc.supported)
        ImGui::Text("P4G.exe v1.0.5 recognised  ·  %d/%d address checks passed  ·  %d hooks",
                    hc.probesPassed, hc.probesTotal, Hooks::Count());
    else
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Red), "Unsupported game build: %s",
                           hc.reason.c_str());
    std::wstring bk = Trainer::BackupDir();
    if (!bk.empty()) {
        size_t cut = bk.find_last_of(L'\\');
        W::Dim("Your saves were backed up this session (backups\\%s).",
               Paths::Narrow(bk.substr(cut + 1)).c_str());
    }
    W::Dim("%s or %s opens and closes this. LB / RB or Page Up / Down switch pages.", Input::ToggleKeyName(),
           Input::PadComboName());
}

void Settings() {
    SectionHeader("DISPLAY");
    float s = Overlay::UserScale();
    if (ImGui::SliderFloat("UI scale", &s, 0.6f, 2.0f, "%.2fx")) Overlay::SetUserScale(s);
    W::Dim("The base size follows the game's resolution; this multiplies it.");

    SectionHeader("SPOILERS");
    int p = int(Spoilers::GetPolicy());
    if (ImGui::RadioButton("Hide anything past my progress (recommended)", p == 0)) Spoilers::SetPolicy(Spoilers::Policy::HideFuture);
    if (ImGui::RadioButton("Show everything", p == 1)) Spoilers::SetPolicy(Spoilers::Policy::ShowAll);
    W::Dim("Hidden items show as ??? with a Reveal button that reveals just that one item.");

    SectionHeader("START WITH THE GAME");
    {
        wchar_t v[4] = {};
        bool viaGameFolder = GetEnvironmentVariableW(L"P4GTRAINER_AUTOLOAD", v, 4) > 0;
        bool on = ::Settings::GetInt("autoload", "enabled", 1) != 0;
        if (viaGameFolder)
            W::Dim("This session the trainer was started by dinput8.dll in the game folder.");
        else
            W::Dim("To have the trainer start by itself every time the game does, copy dinput8.dll and "
                   "P4GTrainer.dll into the game folder (Steam: Manage → Browse local files).");
        if (ImGui::Checkbox("Load automatically with the game", &on)) ::Settings::SetInt("autoload", "enabled", on);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Only matters when the two files are in the game folder. Off: the game starts "
                              "without the trainer (P4GTrainer.exe can still load it). Takes effect at the next "
                              "game start.");
    }

    SectionHeader("OPEN / CLOSE THE TRAINER");
    float sc = Overlay::Scale();
    W::Title("Keyboard");
    W::Dim("Keys the game itself uses (from its own key settings) can't be picked.");
    int nk = 0;
    const Input::KeyOption* keys = Input::ToggleKeyOptions(&nk);
    for (int i = 0; i < nk; ++i) {
        if (i % 8) ImGui::SameLine();
        bool used = Input::GameUsesKey(keys[i].dik);
        bool cur = Input::ToggleKey() == keys[i].vk;
        ImGui::PushID(i);
        ImGui::BeginDisabled(used && !cur);
        if (ImGui::Selectable(keys[i].name, cur, 0, ImVec2(110 * sc, 0)) && !used) Input::SetToggleKey(keys[i].vk);
        ImGui::EndDisabled();
        if (used && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("The game uses this key.");
        ImGui::PopID();
    }
    ImGui::Spacing();
    W::Title("Controller");
    int nc = 0;
    const Input::PadCombo* combos = Input::PadCombos(&nc);
    for (int i = 0; i < nc; ++i) {
        if (i) ImGui::SameLine();
        ImGui::PushID(100 + i);
        if (ImGui::Selectable(combos[i].name, Input::PadComboIndex() == i, 0, ImVec2(150 * sc, 0)))
            Input::SetPadCombo(i);
        ImGui::PopID();
    }
    W::Dim("Both buttons together. Saved for next time.");
}

void About() {
    SectionHeader("P4G TRAINER");
    ImGui::Text("Version %s  (build %s)", Trainer::Version(), Trainer::BuildId());
    ImGui::TextWrapped(
        "An in-game trainer for Persona 4 Golden (Steam, 64-bit). It changes the game through the "
        "game's own functions wherever possible and warns before anything that could affect the "
        "story. MIT licensed.");
    SectionHeader("FILES");
    ImGui::TextWrapped("Log: %s", Paths::Narrow(Trainer::LogPath()).c_str());
    ImGui::TextWrapped("Backups and settings: %s", Paths::Narrow(Paths::DataDir()).c_str());
    SectionHeader("CREDITS");
    ImGui::BulletText("Dear ImGui (Omar Cornut, MIT), MinHook (Tsuda Kageyu, BSD-2)");
    ImGui::BulletText("Research by the Persona modding community: AnimatedSwine37, AquaRose7,");
    ImGui::BulletText("  zarroboogs, tge, ShrineFox, rirurin, seyleonhart, qSleeP and others.");
    std::string failed = HostCheck::FailedProbes();
    if (!failed.empty()) {
        SectionHeader("FAILED ADDRESS CHECKS");
        ImGui::TextUnformatted(failed.c_str());
    }
}

}  // namespace Pages
