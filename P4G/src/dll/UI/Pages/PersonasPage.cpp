#include <imgui.h>
#include <cstdio>
#include <string>

#include "Engine/GameState.h"
#include "Engine/Names.h"
#include "Features/Edits.h"
#include "Features/Spoilers.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace {

// Party-member join flags (48..54): Yosuke, Chie, Yukiko, Kanji, Rise, Teddie, Naoto.
int JoinFlag(int charId) {
    switch (charId) {
        case Game::Yosuke: return 48;
        case Game::Chie: return 49;
        case Game::Yukiko: return 50;
        case Game::Kanji: return 51;
        case Game::Rise: return 52;
        case Game::Teddie: return 53;
        case Game::Naoto: return 54;
    }
    return -1;
}

const char* CharName(int id) {
    static const char* n[] = {"", "Yu", "Yosuke", "Chie", "Yukiko", "Rise", "Kanji", "Naoto", "Teddie"};
    return (id >= 1 && id <= 8) ? n[id] : "?";
}

void DrawPersona(const char* id, const Game::Persona& p, float w, bool equipped, const char* owner) {
    float sc = Overlay::Scale();
    uint8_t arc = Game::PersonaArcana(p.id);
    ImU32 accent = W::ArcanaColor(arc);
    W::BeginCard(id, ImVec2(w, 230 * sc), equipped ? C::Yellow : accent);
    std::string arcana = Names::Arcana(arc);
    W::Chip(arcana.empty() ? "?" : arcana.c_str(), accent);
    if (equipped) {
        ImGui::SameLine();
        W::Chip("EQUIPPED", C::Yellow);
    }
    if (owner) {
        ImGui::SameLine();
        W::Dim("%s", owner);
    }
    ImGui::PushFont(g_fonts.bold);
    ImGui::Text("%s", Names::PersonaOr(p.id).c_str());
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::Text("Lv %u", p.level);

    static const char* stat[] = {"St", "Ma", "En", "Ag", "Lu"};
    float barW = (w - 60 * sc) * 0.5f;
    for (int i = 0; i < 5; ++i) {
        int v = p.stats[i] + p.bonus[i] + p.other[i];
        if (v > 99) v = 99;
        char buf[16];
        snprintf(buf, sizeof buf, "%s %d", stat[i], v);
        if (i % 2) ImGui::SameLine();
        W::Bar(v / 99.0f, ImVec2(barW, 0), accent, buf);
    }
    ImGui::Spacing();
    int shown = 0;
    for (uint16_t sk : p.skills) {
        if (!sk) continue;
        if (shown % 2) ImGui::SameLine(w * 0.5f);
        ImGui::Text("%s", Names::SkillOr(sk).c_str());
        ++shown;
    }
    if (!shown) W::Dim("No skills");
    W::EndCard();
}

int g_edit = -1;          // stock slot being edited
int g_stats[5] = {};
int g_statsFor = -1;
int g_skillSlot = -1;     // skill slot whose picker is open
char g_skillFilter[32] = "";

bool Contains(const std::string& hay, const char* needle) {
    if (!*needle) return true;
    std::string a = hay, b = needle;
    for (auto& c : a) c = (char)tolower((unsigned char)c);
    for (auto& c : b) c = (char)tolower((unsigned char)c);
    return a.find(b) != std::string::npos;
}

void DrawEditor(const Game::Snapshot& s) {
    float sc = Overlay::Scale();
    SectionHeader("EDIT A PERSONA");
    // Pick a Persona: one button each (no popups, so it works the same with a pad).
    int shown = 0;
    for (int i = 0; i < Game::kStock; ++i) {
        if (!s.stock[i].valid) continue;
        if (shown++) ImGui::SameLine();
        ImGui::PushID(i);
        bool sel = g_edit == i;
        if (sel) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(C::Yellow));
        if (sel) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(C::Black));
        if (ImGui::Button(Names::PersonaOr(s.stock[i].id).c_str())) {
            g_edit = i;
            g_skillSlot = -1;
        }
        if (sel) ImGui::PopStyleColor(2);
        ImGui::PopID();
    }
    if (g_edit < 0 || !s.stock[g_edit].valid) {
        W::Dim("Pick one of Yu's Personas to change its stats or skills.");
        return;
    }
    const auto& p = s.stock[g_edit];
    if (g_statsFor != g_edit) {
        for (int i = 0; i < 5; ++i) g_stats[i] = p.stats[i];
        g_statsFor = g_edit;
    }
    static const char* stat[] = {"St", "Ma", "En", "Ag", "Lu"};
    for (int i = 0; i < 5; ++i) {
        ImGui::SetNextItemWidth(220 * sc);
        ImGui::SliderInt(stat[i], &g_stats[i], 1, 99);
        if (i % 2 == 0 && i < 4) ImGui::SameLine(300 * sc);
    }
    if (ImGui::Button("Max all to 99")) for (int& v : g_stats) v = 99;
    ImGui::SameLine();
    if (W::RiskButton("Apply stats", Risk::Caution,
                      "Sets the Persona's base stats (the displayed stat also adds incense and level-up "
                      "growth, capped at 99). Its level doesn't change.")) {
        for (int i = 0; i < 5; ++i)
            if (g_stats[i] != p.stats[i]) Edits::SetPersonaStat(g_edit, i, (uint8_t)g_stats[i]);
    }

    ImGui::Spacing();
    W::Title("Skills");
    for (int k = 0; k < 8; ++k) {
        ImGui::PushID(k);
        uint16_t sk = p.skills[k];
        char label[64];
        snprintf(label, sizeof label, "%d. %s", k + 1, sk ? Names::SkillOr(sk).c_str() : "(empty)");
        bool open = g_skillSlot == k;
        if (open) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(C::YellowDim));
        if (ImGui::Button(label, ImVec2(260 * sc, 0))) g_skillSlot = open ? -1 : k;
        if (open) ImGui::PopStyleColor();
        ImGui::PopID();
        if (k % 2 == 0) ImGui::SameLine(300 * sc);
    }
    if (g_skillSlot >= 0) {
        int k = g_skillSlot;
        ImGui::Spacing();
        W::Dim("Choose a skill for slot %d:", k + 1);
        ImGui::SetNextItemWidth(240 * sc);
        ImGui::InputTextWithHint("##skf", "Filter", g_skillFilter, sizeof g_skillFilter);
        ImGui::SameLine();
        if (ImGui::Button("Clear slot")) {
            Edits::SetPersonaSkill(g_edit, k, 0);
            g_skillSlot = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) g_skillSlot = -1;
        ImGui::BeginChild("skills", ImVec2(0, 220 * sc), ImGuiChildFlags_Borders);
        int cols = W::Columns(220 * sc), n = 0;
        for (int id = 1; id < 624 && g_skillSlot >= 0; ++id) {
            std::string nm = Names::Skill((uint16_t)id);
            if (nm.empty() || !Contains(nm, g_skillFilter)) continue;
            if (n++ % cols) ImGui::SameLine();
            ImGui::PushID(id);
            if (ImGui::Selectable(nm.c_str(), p.skills[k] == id, 0, ImVec2(200 * sc, 0))) {
                Edits::SetPersonaSkill(g_edit, k, (uint16_t)id);
                g_skillSlot = -1;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    W::Dim("Skill changes take effect immediately. Some skills are enemy-only or unique to one "
           "Persona and may behave oddly on another.");
}

}  // namespace

namespace Pages {

void Personas() {
    if (!RequireSave()) return;
    const auto& s = Game::Current();
    float sc = Overlay::Scale();

    int count = 0;
    for (const auto& p : s.stock) count += p.valid != 0;
    SectionHeader("YU'S PERSONAS");
    ImGui::Text("%d in stock", count);
    int cols = W::Columns(330 * sc);
    float gap = ImGui::GetStyle().ItemSpacing.x;
    float w = (ImGui::GetContentRegionAvail().x - gap * (cols - 1)) / cols;
    int n = 0;
    for (int i = 0; i < Game::kStock; ++i) {
        const auto& p = s.stock[i];
        if (!p.valid) continue;
        if (n % cols) ImGui::SameLine();
        char id[16];
        snprintf(id, sizeof id, "stock%d", i);
        DrawPersona(id, p, w, i == s.equippedSlot, nullptr);
        ++n;
    }

    DrawEditor(s);

    SectionHeader("PARTY MEMBERS");
    W::Dim("A party member's level is their Persona's level.");
    n = 0;
    for (int c = Game::Yosuke; c <= Game::Teddie; ++c) {
        const auto& u = s.U(c);
        bool joined = s.Flag(JoinFlag(c));
        if (!u.persona.id) continue;
        if (n % cols) ImGui::SameLine();
        char id[16];
        snprintf(id, sizeof id, "member%d", c);
        if (Spoilers::Visible(Spoilers::Kind::Persona, 1000 + c, joined)) {
            DrawPersona(id, u.persona, w, false, CharName(c));
        } else {
            W::BeginCard(id, ImVec2(w, 230 * sc), C::Grey);
            W::Dim("Party member");
            Spoilers::HiddenPlaceholder(Spoilers::Kind::Persona, 1000 + c, "hasn't joined yet");
            W::EndCard();
        }
        ++n;
    }
}

}  // namespace Pages
