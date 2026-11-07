#include <imgui.h>
#include <cstdio>
#include <string>

#include "Engine/GameState.h"
#include "Engine/Names.h"
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
