#include <imgui.h>
#include <cstdio>
#include <string>

#include "Data/Romance.h"
#include "Engine/Calendar.h"
#include "Engine/GameState.h"
#include "Features/Spoilers.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Portraits.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"
#include "Util/Settings.h"

using namespace Theme;

namespace {

int g_sel = -1;

struct Live {
    const Game::Link* link = nullptr;
    int rank = 0;
    bool dating = false;
    bool started = false;
};

Live Read(const Game::Snapshot& s, const Data::RomanceRoute& r) {
    Live l;
    for (const auto& e : s.links)
        if (e.id && (e.id == r.link || e.id == r.loverRow)) {
            l.link = &e;
            l.rank = e.rank;
            l.started = e.rank > 0;
            l.dating = e.id == r.loverRow || (r.loverFlag >= 0 && s.Flag(r.loverFlag));
        }
    return l;
}

void Card(int i, const Data::RomanceRoute& r, const Live& l, float w, bool met) {
    float sc = Overlay::Scale();
    float h = 300 * sc;
    ImGui::PushID(i);
    ImVec2 p = ImGui::GetCursorScreenPos();
    bool sel = g_sel == i;
    if (ImGui::InvisibleButton("card", ImVec2(w, h), ImGuiButtonFlags_EnableNav)) g_sel = i;
    bool hov = ImGui::IsItemHovered() || ImGui::IsItemFocused();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 frame = l.dating ? IM_COL32(240, 90, 150, 255) : (sel ? C::Yellow : (hov ? C::YellowDim : C::PanelHi));
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), IM_COL32(30, 28, 26, 255));

    float imgH = h - 46 * sc;
    ImU32 tint = met ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 255);  // silhouette if unmet
    if (!Portraits::DrawCover(dl, r.bustup, 1, r.bustupOutfit, ImVec2(p.x + 2, p.y + 2),
                              ImVec2(p.x + w - 2, p.y + imgH), tint)) {
        const char* t = met ? r.name : "?";
        ImGui::PushFont(g_fonts.title);
        ImVec2 ts = ImGui::CalcTextSize(t);
        dl->AddText(ImVec2(p.x + (w - ts.x) * 0.5f, p.y + imgH * 0.45f), C::Grey, t);
        ImGui::PopFont();
    }
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), frame, 0, 0, sel ? 3.0f : 1.5f);
    // name band
    dl->AddRectFilled(ImVec2(p.x, p.y + imgH), ImVec2(p.x + w, p.y + h), IM_COL32(12, 12, 12, 235));
    ImGui::PushFont(g_fonts.bold);
    const char* label = met ? r.name : "???";
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(p.x + 10 * sc, p.y + imgH + 6 * sc), met ? C::White : C::Grey, label);
    ImGui::PopFont();
    if (met) {
        char sub[48];
        if (l.dating) snprintf(sub, sizeof sub, "♥ Dating · rank %d", l.rank);
        else if (l.started) snprintf(sub, sizeof sub, "Rank %d", l.rank);
        else snprintf(sub, sizeof sub, "Not started");
        ImGui::PushFont(g_fonts.note);
        dl->AddText(ImVec2(p.x + 10 * sc + ts.x + 10 * sc, p.y + imgH + 9 * sc),
                    l.dating ? IM_COL32(240, 90, 150, 255) : C::Grey, sub);
        ImGui::PopFont();
    }
    ImGui::PopID();
}

void Steps(const Game::Snapshot& s, int idx, const Data::RomanceRoute& r, const Live& l) {
    for (int k = 0; k < r.stepCount; ++k) {
        const auto& st = r.steps[k];
        bool done = st.rank == 0 ? l.started : (st.rank <= 10 ? l.rank >= st.rank : false);
        if (st.rank == r.loverRank) done = l.dating;
        bool current = !done && (k == 0 || [&] {
            const auto& prev = r.steps[k - 1];
            return prev.rank == 0 ? l.started : l.rank >= prev.rank;
        }());
        int key = idx * 64 + k;
        ImGui::PushID(key);
        W::Chip(done ? "DONE" : (current ? "NEXT" : "LATER"), done ? C::Green : (current ? C::Yellow : C::PanelHi),
                done || current ? C::Black : C::Grey);
        ImGui::SameLine();
        if (st.rank && st.rank <= 10) {
            W::Dim("Rank %d", st.rank);
            ImGui::SameLine();
        }
        // Each step is hidden until revealed, except ones already done.
        if (!Spoilers::Visible(Spoilers::Kind::Step, key, done)) {
            Spoilers::HiddenPlaceholder(Spoilers::Kind::Step, key, "this step");
        } else {
            ImGui::PushTextWrapPos(0);
            ImGui::TextUnformatted(st.text);
            if (st.avoid) {
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Orange), "Avoid: %s", st.avoid);
            }
            ImGui::PopTextWrapPos();
        }
        if (st.byMonth && !done) {
            int last = Calendar::ToIndex(st.byMonth, st.byDay);
            if (last >= 0 && s.day <= last) W::Dim("%d day(s) left for this (until %d/%d).", last - s.day, st.byMonth, st.byDay);
            else if (last >= 0) ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Red), "Past %d/%d.", st.byMonth, st.byDay);
        }
        ImGui::Spacing();
        ImGui::PopID();
    }
}

bool Met(const Game::Snapshot& s, const Data::RomanceRoute& r, const Live& l) {
    if (l.started) return true;
    if (r.metFlag != 0xFFFF && s.Flag(r.metFlag)) return true;
    if (r.metMonth) {
        int d = Calendar::ToIndex(r.metMonth, r.metDay);
        if (d >= 0 && s.day >= d) return true;
    }
    return false;
}

}  // namespace

namespace Pages {

void Romance() {
    if (!RequireSave()) return;
    const auto& s = Game::Current();
    float sc = Overlay::Scale();
    if (g_sel < 0) g_sel = Settings::GetInt("romance", "target", -1);

    Live lives[Data::kRouteCount];
    int dating = 0;
    for (int i = 0; i < Data::kRouteCount; ++i) {
        lives[i] = Read(s, Data::kRoutes[i]);
        dating += lives[i].dating;
    }

    SectionHeader("CHOOSE A PARTNER");
    if (dating)
        W::Dim("♥ marks a relationship that's official — the game has no way to break up, so it stays "
               "locked. Dating more than one person is possible, with consequences on 2/14.");
    else
        W::Dim("Pick who you'd like to date. Characters you haven't met are shadows until you meet them.");

    float w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 7) / 8;
    if (w < 120 * sc) w = 120 * sc;
    int cols = int((ImGui::GetContentRegionAvail().x + ImGui::GetStyle().ItemSpacing.x) /
                   (w + ImGui::GetStyle().ItemSpacing.x));
    if (cols < 1) cols = 1;
    for (int i = 0; i < Data::kRouteCount; ++i) {
        if (i % cols) ImGui::SameLine();
        bool met = Spoilers::Visible(Spoilers::Kind::Romance, i, Met(s, Data::kRoutes[i], lives[i]));
        int before = g_sel;
        Card(i, Data::kRoutes[i], lives[i], w, met);
        if (g_sel != before) Settings::SetInt("romance", "target", g_sel);
    }

    if (g_sel < 0 || g_sel >= Data::kRouteCount) return;
    const auto& r = Data::kRoutes[g_sel];
    const auto& l = lives[g_sel];
    bool met = Spoilers::Visible(Spoilers::Kind::Romance, g_sel, Met(s, r, l));
    SectionHeader("ROUTE");
    if (!met) {
        ImGui::TextWrapped("You haven't met this person yet.");
        Spoilers::HiddenPlaceholder(Spoilers::Kind::Romance, g_sel, "who it is and their route");
        return;
    }
    ImGui::PushFont(g_fonts.title);
    ImGui::Text("%s", r.name);
    ImGui::PopFont();
    if (l.dating) {
        ImGui::SameLine();
        W::Chip("♥ OFFICIAL", IM_COL32(240, 90, 150, 255));
    }
    if (dating && !l.dating)
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Orange),
                           "You're already dating someone. Starting a second relationship is allowed, but on "
                           "Valentine's Day you can only choose one.");
    ImGui::Spacing();
    Steps(s, g_sel, r, l);

    if (l.dating) {
        SectionHeader("AFTER IT'S OFFICIAL");
        for (int k = 0; k < int(sizeof(Data::kAfterSteps) / sizeof(Data::kAfterSteps[0])); ++k) {
            int key = 900 + k;
            ImGui::PushID(key);
            if (!Spoilers::Visible(Spoilers::Kind::Step, key, false)) Spoilers::HiddenPlaceholder(Spoilers::Kind::Step, key);
            else ImGui::BulletText("%s", Data::kAfterSteps[k]);
            ImGui::PopID();
        }
    }
    ImGui::Spacing();
    W::Dim("To make a rank's scene play sooner, use Social Links → Manage → \"Ready at next hangout\".");
}

}  // namespace Pages
