#include "UI/Shell.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <cstdio>

#include "Core/Trainer.h"
#include "Engine/HostCheck.h"
#include "Hooks/Hook.h"
#include "Hooks/InputHooks.h"
#include "UI/Overlay.h"
#include "UI/Theme.h"
#include "Util/Log.h"
#include "Util/Paths.h"

using namespace Theme;

namespace {

void PagePlanned(const char* milestone, const char* const* lines);

// ------------------------------------------------------------------ pages
void DrawDashboard() {
    const auto& hc = HostCheck::Get();
    SectionHeader("STATUS");
    if (hc.supported) {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Green), "P4G.exe v1.0.5 recognised");
        ImGui::Text("Address probes: %d / %d passed", hc.probesPassed, hc.probesTotal);
    } else {
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Red), "Unsupported game build");
        ImGui::TextWrapped("%s. The trainer stays dormant so nothing can be corrupted.",
                           hc.reason.c_str());
    }
    ImGui::Text("Hooks installed: %d", Hooks::Count());
    std::wstring bk = Trainer::BackupDir();
    if (!bk.empty()) {
        size_t s = bk.find_last_of(L'\\');
        ImGui::Text("Saves backed up this session: backups\\%s",
                    Paths::Narrow(bk.substr(s + 1)).c_str());
    } else {
        ImGui::TextDisabled("Save backup: already done earlier this session, or Steam not found");
    }

    SectionHeader("CONTROLS");
    ImGui::BulletText("Open / close: Insert, or Back + R3 on a controller");
    ImGui::BulletText("Switch page: LB / RB, or Page Up / Page Down");
    ImGui::BulletText("Close: Esc or Start as well");
    ImGui::BulletText("While this is open the game receives no input.");

    SectionHeader("ROADMAP");
    static const char* rows[][2] = {
        {"M0", "Overlay, input, host check, save backup  (this build)"},
        {"M1", "Read-only pages: date & case countdown, links, stats, items, Personas"},
        {"M2", "Safe edits: battle HP/SP, inventory, money, stats, undo journal"},
        {"M3", "Organic social link engine and romance"},
        {"M4", "Personas & fusion, levels & EXP, NG+ unlocks, Advanced"},
        {"M4b", "Enemy Intel and the Today planner"},
        {"M4c", "Battle overrides, romance guide, dialogue auto-advance, field QoL"},
    };
    if (ImGui::BeginTable("roadmap", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
        for (auto& r : rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Yellow), "%s", r[0]);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(r[1]);
        }
        ImGui::EndTable();
    }
}

void DrawSettings() {
    SectionHeader("DISPLAY");
    float s = Overlay::UserScale();
    if (ImGui::SliderFloat("UI scale", &s, 0.6f, 2.0f, "%.2fx")) Overlay::SetUserScale(s);
    ImGui::TextDisabled("Applied on the next frame. The base size follows the game's resolution.");

    SectionHeader("HOTKEYS");
    ImGui::Text("Toggle key: Insert   ·   Controller: Back + R3");
    ImGui::TextDisabled("Rebinding arrives with the settings file in M1.");
}

void DrawAbout() {
    SectionHeader("P4G TRAINER");
    ImGui::Text("Build %s", Trainer::BuildId());
    ImGui::TextWrapped(
        "An in-game trainer for Persona 4 Golden (Steam, 64-bit). It changes the game through the "
        "game's own functions wherever possible and warns before anything that could affect the "
        "story. MIT licensed.");
    SectionHeader("FILES");
    ImGui::TextWrapped("Log: %s", Paths::Narrow(Trainer::LogPath()).c_str());
    ImGui::TextWrapped("Backups and settings: %s", Paths::Narrow(Paths::DataDir()).c_str());
    SectionHeader("CREDITS");
    ImGui::BulletText("Dear ImGui (Omar Cornut, MIT), MinHook (Tsuda Kageyu, BSD-2)");
    ImGui::BulletText("Prior research by the Persona modding community: AnimatedSwine37, AquaRose7,");
    ImGui::BulletText("  zarroboogs, tge, ShrineFox, rirurin, seyleonhart, qSleeP and others.");
    std::string failed = HostCheck::FailedProbes();
    if (!failed.empty()) {
        SectionHeader("FAILED PROBES");
        ImGui::TextUnformatted(failed.c_str());
    }
}

#define PLANNED(fn, ms, ...)                          \
    void fn() {                                       \
        static const char* const lines[] = {__VA_ARGS__, nullptr}; \
        PagePlanned(ms, lines);                       \
    }

PLANNED(DrawToday, "M4b", "Everything you can do in the current time slot, and where.",
        "Priority: last chances > romance > links / hangouts > clubs / jobs > the rest.")
PLANNED(DrawBattle, "M2 / M4c", "Infinite HP & SP, no ailments or downs, enemies always miss,",
        "always crit, forced 1 More, weakness override, unlimited Persona swaps.")
PLANNED(DrawInventory, "M2", "Every item in the game, add / remove / set counts,",
        "and items that are never used up in battle or the camp menu.")
PLANNED(DrawPersonas, "M4", "Stock and compendium editing, give / remove,",
        "fusion preview and Velvet Room rule overrides.")
PLANNED(DrawLinks, "M1 / M3", "Every social link: rank, points, availability, prerequisites,",
        "and organic rank-ups that replay the link's own story side effects.")
PLANNED(DrawRomance, "M4c", "Pick a partner by portrait and follow a step-by-step route,",
        "with every step hidden until you choose to reveal it.")
PLANNED(DrawCase, "M1", "The current case, its deadline and the usable days left.")
PLANNED(DrawYu, "M1 / M2", "Courage, Knowledge, Diligence, Understanding, Expression,",
        "money and equipment.")
PLANNED(DrawLevels, "M4", "Levels and EXP for Yu, every party member and every Persona,",
        "using the game's own level-up routines.")
PLANNED(DrawIntel, "M4b", "Reveal every enemy's affinities in the game's own Analysis screen,",
        "plus an in-battle HUD and a spoiler-masked shadow list.")
PLANNED(DrawNgPlus, "M4", "New Game+ carry-overs granted individually, with impact warnings.")
PLANNED(DrawField, "M4c", "Shops from anywhere, floor select, party changes in dungeons,",
        "save anywhere, auto-advance for dialogue.")
PLANNED(DrawAdvanced, "M4", "Flag and counter browser with labels, raw edits, and the undo journal.")

struct PageDef {
    const char* name;
    void (*draw)();
    bool ready;
};

const PageDef kPages[] = {
    {"Dashboard", DrawDashboard, true},   {"Today", DrawToday, false},
    {"Battle", DrawBattle, false},        {"Inventory", DrawInventory, false},
    {"Personas", DrawPersonas, false},    {"Social Links", DrawLinks, false},
    {"Romance", DrawRomance, false},      {"Case File", DrawCase, false},
    {"Protagonist", DrawYu, false},       {"Levels & EXP", DrawLevels, false},
    {"Enemy Intel", DrawIntel, false},    {"NG+ & Unlocks", DrawNgPlus, false},
    {"Field & Dungeon", DrawField, false}, {"Advanced", DrawAdvanced, false},
    {"Settings", DrawSettings, true},     {"About", DrawAbout, true},
};
constexpr int kPageCount = int(sizeof kPages / sizeof kPages[0]);
int g_page = 0;

void PagePlanned(const char* milestone, const char* const* lines) {
    SectionHeader("COMING UP");
    ImGui::PushFont(g_fonts.bold);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Yellow), "Arrives in milestone %s",
                       milestone);
    ImGui::PopFont();
    for (const char* const* l = lines; *l; ++l) ImGui::TextWrapped("%s", *l);
}

// ------------------------------------------------------------------ chrome
void DrawHeader(ImVec2 pos, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float s = Overlay::Scale();
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), C::Black);
    Stripes(dl, pos, ImVec2(pos.x + size.x, pos.y + size.y), 14 * s, IM_COL32(255, 225, 0, 18));

    ImGui::PushFont(g_fonts.title);
    const char* title = "P4G TRAINER";
    ImVec2 ts = ImGui::CalcTextSize(title);
    float slant = size.y * 0.45f;
    SlantRect(dl, ImVec2(pos.x - slant, pos.y), ImVec2(pos.x + ts.x + 40 * s + slant, pos.y + size.y),
              slant, C::Yellow);
    dl->AddText(ImVec2(pos.x + 20 * s, pos.y + (size.y - ts.y) * 0.5f), C::Black, title);
    ImGui::PopFont();

    // Right side: game date and mode (read from the game from M1 on).
    ImGui::PushFont(g_fonts.bold);
    const char* right = "Game state readout arrives in M1";
    ImVec2 rs = ImGui::CalcTextSize(right);
    dl->AddText(ImVec2(pos.x + size.x - rs.x - 20 * s, pos.y + (size.y - rs.y) * 0.5f), C::Grey, right);
    ImGui::PopFont();
    dl->AddLine(ImVec2(pos.x, pos.y + size.y), ImVec2(pos.x + size.x, pos.y + size.y), C::Orange,
                3 * s);
}

bool g_focusRail = false;  // set when the modal opens or the page changes by shortcut

void DrawRail(float width) {
    float s = Overlay::Scale();
    ImGui::BeginChild("rail", ImVec2(width, 0), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float h = ImGui::GetFrameHeight() + 8 * s;
    int focusTarget = g_focusRail ? g_page : -1;
    g_focusRail = false;
    for (int i = 0; i < kPageCount; ++i) {
        ImGui::PushID(i);
        ImVec2 p = ImGui::GetCursorScreenPos();
        bool sel = (i == g_page);
        if (i == focusTarget) ImGui::SetKeyboardFocusHere();
        if (ImGui::InvisibleButton("tab", ImVec2(width - 6 * s, h), ImGuiButtonFlags_EnableNav)) g_page = i;
        // Moving the keyboard / pad focus along the rail switches pages, like the game's menus.
        if (ImGui::IsItemFocused() && i != focusTarget && ImGui::GetIO().NavActive) g_page = i;
        bool hov = ImGui::IsItemHovered() || ImGui::IsItemFocused();
        ImVec2 mx(p.x + width - 6 * s, p.y + h);
        if (sel) SlantRect(dl, p, mx, h * 0.3f, C::Yellow);
        else if (hov) SlantRect(dl, p, mx, h * 0.3f, IM_COL32(255, 225, 0, 60));
        ImGui::PushFont(sel ? g_fonts.bold : g_fonts.body);
        ImU32 tc = sel ? C::Black : (kPages[i].ready ? C::White : C::Grey);
        ImVec2 ts = ImGui::CalcTextSize(kPages[i].name);
        dl->AddText(ImVec2(p.x + 18 * s, p.y + (h - ts.y) * 0.5f), tc, kPages[i].name);
        ImGui::PopFont();
        ImGui::PopID();
    }
    ImGui::EndChild();
}

void DrawFooter() {
    ImGui::PushFont(g_fonts.note);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Grey),
                       "LB/RB  page      A  select      B  back      Start / Esc / Insert / Back+R3  close");
    ImGui::PopFont();
}

void HandleShortcuts() {
    if (ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)) return;
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false) || ImGui::IsKeyPressed(ImGuiKey_PageUp, false)) {
        g_page = (g_page + kPageCount - 1) % kPageCount;
        g_focusRail = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false) || ImGui::IsKeyPressed(ImGuiKey_PageDown, false)) {
        g_page = (g_page + 1) % kPageCount;
        g_focusRail = true;
    }
    if (!ImGui::GetIO().WantTextInput &&
        (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false)))
        Overlay::SetOpen(false);
}

}  // namespace

namespace Shell {

bool WantsHud() { return false; }

static bool s_justOpened = false;

void NotifyOpened() { s_justOpened = true; }

void Draw(bool open) {
    if (!open) return;
    if (s_justOpened) {
        s_justOpened = false;
        ImGui::SetNextWindowFocus();
        g_focusRail = true;
    }
    ImGuiIO& io = ImGui::GetIO();
    float s = Overlay::Scale();
    ImVec2 ds = io.DisplaySize;

    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    bg->AddRectFilled(ImVec2(0, 0), ds, C::Dim);

    ImVec2 size(ds.x * 0.86f, ds.y * 0.86f);
    ImGui::SetNextWindowPos(ImVec2((ds.x - size.x) * 0.5f, (ds.y - size.y) * 0.5f));
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGuiWindowFlags wf = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("P4G Trainer", nullptr, wf)) {
        ImGui::PopStyleVar();
        ImVec2 wp = ImGui::GetWindowPos();
        float headerH = 64 * s;
        DrawHeader(wp, ImVec2(size.x, headerH));
        ImGui::SetCursorScreenPos(ImVec2(wp.x + 12 * s, wp.y + headerH + 14 * s));

        float railW = 230 * s;
        float footerH = ImGui::GetTextLineHeightWithSpacing() + 12 * s;
        ImGui::BeginGroup();
        ImGui::BeginChild("body", ImVec2(size.x - 24 * s, size.y - headerH - 28 * s - footerH),
                          ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
        DrawRail(railW);
        ImGui::SameLine(0, 10 * s);
        ImGui::BeginChild("page", ImVec2(0, 0), ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_AlwaysUseWindowPadding);
        ImGui::PushFont(g_fonts.title);
        ImGui::TextUnformatted(kPages[g_page].name);
        ImGui::PopFont();
        kPages[g_page].draw();
        ImGui::EndChild();
        ImGui::EndChild();
        DrawFooter();
        ImGui::EndGroup();
        HandleShortcuts();
    } else {
        ImGui::PopStyleVar();
    }
    ImGui::End();

    if (Input::g_debug) {
        ImGuiContext& g = *ImGui::GetCurrentContext();
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, false) || ImGui::IsKeyPressed(ImGuiKey_Tab, false))
            LOGI("imgui: key pressed; NavWindow=%s NavId=%08X NavMoveDir=%d NavLayer=%d KeyMods=%X "
                 "InitReq=%d InitResult=%08X ActiveId=%08X",
                 g.NavWindow ? g.NavWindow->Name : "-", g.NavId, (int)g.NavMoveDir, (int)g.NavLayer,
                 (unsigned)io.KeyMods, g.NavInitRequest, g.NavInitResult.ID, g.ActiveId);
        ImGui::SetNextWindowPos(ImVec2(8, 8));
        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::Begin("##navdebug", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        ImGui::Text("NavActive %d  NavVisible %d  WantKb %d", io.NavActive, io.NavVisible,
                    io.WantCaptureKeyboard);
        ImGui::Text("NavWindow %s  NavId %08X", g.NavWindow ? g.NavWindow->Name : "-", g.NavId);
        ImGui::Text("Down held %d  frame %d", ImGui::IsKeyDown(ImGuiKey_DownArrow), ImGui::GetFrameCount());
        ImGui::End();
    }
}

}  // namespace Shell
