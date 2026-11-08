#include "UI/Shell.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <cstdio>

#include "Data/Cases.h"
#include "Engine/Calendar.h"
#include "Engine/GameState.h"
#include "Features/CaseFile.h"
#include "Features/EnemyIntel.h"
#include "Hooks/InputHooks.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"
#include "Util/Log.h"

using namespace Theme;

namespace {

struct PageDef {
    const char* name;
    void (*draw)();
    bool ready;
};

const PageDef kPages[] = {
    {"Dashboard", Pages::Dashboard, true},     {"Today", Pages::Today, true},
    {"Battle", Pages::BattlePage, true},             {"Inventory", Pages::Inventory, true},
    {"Personas", Pages::Personas, true},       {"Social Links", Pages::SocialLinks, true},
    {"Romance", Pages::Romance, true},           {"Case File", Pages::CaseFile, true},
    {"Protagonist", Pages::Protagonist, true}, {"Levels & EXP", Pages::LevelsExp, true},
    {"Enemy Intel", Pages::EnemyIntel, true},         {"NG+ & Unlocks", Pages::NgPlus, true},
    {"Field & Dungeon", Pages::FieldDungeon, true},     {"Advanced", Pages::Advanced, true},
    {"Settings", Pages::Settings, true},       {"About", Pages::About, true},
};
constexpr int kPageCount = int(sizeof kPages / sizeof kPages[0]);
int g_page = 0;
bool g_focusRail = false;  // set when the modal opens or the page changes by shortcut

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

    // Right side: the in-game date, like the game's own calendar corner, plus the case chip.
    const auto& gs = Game::Current();
    float x = pos.x + size.x - 20 * s;
    if (gs.valid) {
        auto d = Calendar::FromIndex(gs.day);
        char date[32], when[48];
        snprintf(date, sizeof date, "%02d/%02d %s", d.month, d.day, Calendar::WeekdayShort(d.weekday));
        snprintf(when, sizeof when, "%s", Calendar::TimeName(gs.time));
        ImGui::PushFont(g_fonts.title);
        ImVec2 ds = ImGui::CalcTextSize(date);
        ImGui::PopFont();
        ImGui::PushFont(g_fonts.bold);
        ImVec2 ws = ImGui::CalcTextSize(when);
        ImGui::PopFont();
        float blockW = (ds.x > ws.x ? ds.x : ws.x);
        float top = pos.y + (size.y - ds.y - ws.y * 0.9f) * 0.5f;
        ImGui::PushFont(g_fonts.title);
        dl->AddText(ImVec2(x - ds.x, top), C::White, date);
        ImGui::PopFont();
        ImGui::PushFont(g_fonts.bold);
        dl->AddText(ImVec2(x - ws.x, top + ds.y * 0.9f), C::Yellow, when);
        ImGui::PopFont();
        x -= blockW + 24 * s;

        auto st = CaseFile::Current(gs);
        if (st.state == CaseFile::State::Active || st.state == CaseFile::State::LastDay) {
            char chip[64];
            if (st.state == CaseFile::State::LastDay) snprintf(chip, sizeof chip, "CASE: LAST DAY");
            else snprintf(chip, sizeof chip, "CASE: %d DAY%s LEFT", st.daysLeft, st.daysLeft == 1 ? "" : "S");
            ImGui::PushFont(g_fonts.bold);
            ImVec2 cs = ImGui::CalcTextSize(chip);
            ImGui::PopFont();
            ImU32 col = st.daysLeft <= 3 ? C::Red : C::Orange;
            float cy = pos.y + (size.y - cs.y) * 0.5f;
            SlantRect(dl, ImVec2(x - cs.x - 24 * s, cy - 4 * s), ImVec2(x, cy + cs.y + 4 * s), 8 * s, col);
            ImGui::PushFont(g_fonts.bold);
            dl->AddText(ImVec2(x - cs.x - 12 * s, cy), C::Black, chip);
            ImGui::PopFont();
        }
    } else {
        ImGui::PushFont(g_fonts.bold);
        const char* right = "No save loaded";
        ImVec2 rs = ImGui::CalcTextSize(right);
        dl->AddText(ImVec2(x - rs.x, pos.y + (size.y - rs.y) * 0.5f), C::Grey, right);
        ImGui::PopFont();
    }
    dl->AddLine(ImVec2(pos.x, pos.y + size.y), ImVec2(pos.x + size.x, pos.y + size.y), C::Orange, 3 * s);
}

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
        if (ImGui::IsItemFocused() && i != focusTarget && ImGui::GetIO().NavVisible) g_page = i;
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
                       "LB/RB  page      A  select      B  back      Start / Esc / %s / %s  close",
                       Input::ToggleKeyName(), Input::PadComboName());
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

void DrawNavDebug() {
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(8, 8));
    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::Begin("##navdebug", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::Text("NavActive %d  NavVisible %d  WantKb %d", io.NavActive, io.NavVisible, io.WantCaptureKeyboard);
    ImGui::Text("NavWindow %s  NavId %08X", g.NavWindow ? g.NavWindow->Name : "-", g.NavId);
    const auto& s = Game::Current();
    ImGui::Text("field %d/%d  battle %d  camp %d  mode %s", s.fieldMajor, s.fieldMinor, s.inBattle,
                s.campMenu, Game::ModeName(s.mode));
    ImGui::End();
}

bool s_justOpened = false;

}  // namespace

namespace Shell {

bool WantsHud() { return Intel::ShowHud() && Game::Current().inBattle; }

void NotifyOpened() { s_justOpened = true; }

void Draw(bool open) {
    Game::Capture();
    if (!open) {
        if (Intel::ShowHud()) Pages::IntelHud();
        return;
    }
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
        ImGui::PushID(g_page);
        kPages[g_page].draw();
        ImGui::PopID();
        ImGui::EndChild();
        ImGui::EndChild();
        DrawFooter();
        ImGui::EndGroup();
        HandleShortcuts();
    } else {
        ImGui::PopStyleVar();
    }
    ImGui::End();

    if (Input::g_debug) DrawNavDebug();
}

}  // namespace Shell
