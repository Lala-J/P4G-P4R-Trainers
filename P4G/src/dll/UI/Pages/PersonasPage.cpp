#include <imgui.h>
#include <cstdio>
#include <string>

#include "Engine/GameState.h"
#include "Engine/Names.h"
#include "Features/Edits.h"
#include "Features/PersonaStock.h"
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


// --- Stock management and getting Personas --------------------------------------------------

bool SameForm(const Game::Persona& a, const Game::Persona& b) {
    if (a.level != b.level || a.exp != b.exp) return false;
    for (int i = 0; i < 8; ++i)
        if (a.skills[i] != b.skills[i]) return false;
    for (int i = 0; i < 5; ++i)
        if (a.stats[i] != b.stats[i] || a.bonus[i] != b.bonus[i]) return false;
    return true;
}

void DrawStockManager(const Game::Snapshot& s) {
    SectionHeader("STOCK & COMPENDIUM");
    W::Dim("%d of %d slots used. Saving to the compendium works like the Velvet Room's Register.",
           PersonaStock::Count(), PersonaStock::Capacity());
    if (!ImGui::BeginTable("stock", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) return;
    ImGui::TableSetupColumn("Persona", 0, 2.0f);
    ImGui::TableSetupColumn("Compendium", 0, 1.6f);
    ImGui::TableSetupColumn("Save", 0, 1.4f);
    ImGui::TableSetupColumn("Remove", 0, 1.2f);
    for (int i = 0; i < Game::kStock; ++i) {
        const auto& p = s.stock[i];
        if (!p.valid) continue;
        ImGui::PushID(i);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%s  Lv %u", Names::PersonaOr(p.id).c_str(), p.level);
        if (i == s.equippedSlot) {
            ImGui::SameLine();
            W::Chip("EQUIPPED", C::Yellow);
        }
        ImGui::TableNextColumn();
        const Game::Persona* c = PersonaStock::CompendiumEntry(p.id);
        bool same = c && SameForm(*c, p);
        if (!c) W::Dim("Not registered");
        else if (same) W::Dim("Up to date");
        else W::Dim("Saved copy: Lv %u", c->level);
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(same);
        if (W::RiskButton("Save to compendium", Risk::Safe,
                          "Registers this Persona as it is now, replacing the saved copy."))
            PersonaStock::Register(i);
        ImGui::EndDisabled();
        ImGui::TableNextColumn();
        const char* why = PersonaStock::WhyNotRemove(i);
        ImGui::BeginDisabled(why != nullptr);
        if (W::RiskButton("Remove", Risk::Caution,
                          "Takes it out of the stock the way fusion uses up an ingredient. Undo puts the "
                          "same Persona back."))
            PersonaStock::Remove(i);
        ImGui::EndDisabled();
        if (why && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", why);
        ImGui::PopID();
    }
    ImGui::EndTable();
}

int g_pick = -1;
char g_pickFilter[32] = "";

int ArcanaLink(uint8_t arcana) { return PersonaStock::ArcanaLink(arcana); }
bool ArcanaVisible(const Game::Snapshot&, uint8_t arcana) { return PersonaStock::ArcanaVisible(arcana); }

bool NgVisible(uint16_t id, const PersonaStock::Info& i) {
    if (!ArcanaVisible(Game::Current(), i.arcana)) return false;
    if (!i.ngPlus) return true;
    bool known = PersonaStock::Registered(id) || PersonaStock::SlotOf(id) >= 0;
    return Spoilers::Visible(Spoilers::Kind::Persona, id, known);
}

void DrawPickDetail(const Game::Snapshot& s, uint16_t id) {
    auto i = PersonaStock::Describe(id);
    if (!i.yours) return;
    ImGui::Spacing();
    if (!NgVisible(id, i)) {
        Spoilers::HiddenPlaceholder(Spoilers::Kind::Persona, id, "a New Game+ Persona");
        return;
    }
    ImGui::PushFont(g_fonts.title);
    ImGui::Text("%s", Names::PersonaOr(id).c_str());
    ImGui::PopFont();
    std::string arc = Names::Arcana(i.arcana);
    W::Chip(arc.empty() ? "?" : arc.c_str(), W::ArcanaColor(i.arcana));
    ImGui::SameLine();
    W::Dim("Base level %u", i.baseLevel);
    bool have = PersonaStock::SlotOf(id) >= 0, reg = PersonaStock::Registered(id);
    if (have) {
        ImGui::SameLine();
        W::Chip("IN STOCK", C::Green);
    }
    if (reg) {
        ImGui::SameLine();
        W::Chip("REGISTERED", C::Blue);
    }
    if (i.special) {
        ImGui::SameLine();
        W::Chip("SPECIAL FUSION", C::PanelHi, C::White);
    }

    int yuLevel = s.U(Game::Yu).level;
    bool above = i.baseLevel > yuLevel;
    ImVec4 warn = ImGui::ColorConvertU32ToFloat4(C::Orange);
    ImGui::PushTextWrapPos(0);
    if (i.ngPlus)
        ImGui::TextColored(warn, "Normally only available in New Game+. It works in a first playthrough, but the "
                                 "game never expects you to have it before the ending.");
    if (i.ultimate)
        ImGui::TextColored(warn, "The game normally lets you fuse this only after the %s link reaches its max rank.",
                           arc.c_str());
    if (above)
        ImGui::TextColored(warn, "Its base level (%u) is above Yu's (%d). The game never hands out a Persona above "
                                 "your level; it works normally once you have it.",
                           i.baseLevel, yuLevel);
    ImGui::PopTextWrapPos();

    Theme::Risk risk = i.ngPlus ? Risk::Dangerous : ((above || i.ultimate) ? Risk::Caution : Risk::Safe);
    const char* why = PersonaStock::WhyNotGive(id);
    ImGui::BeginDisabled(why != nullptr);
    if (W::RiskButton("Get it (base form)", risk,
                      "Creates it exactly as a new Persona starts out (base level, stats and skills) and adds it "
                      "to the stock. A first-time Persona is registered in the compendium, as the game does."))
        PersonaStock::Give(id);
    ImGui::EndDisabled();
    if (reg) {
        int fee = PersonaStock::Price(id);
        const char* whyPay = PersonaStock::WhyNotSummon(id, true);
        const char* whyFree = PersonaStock::WhyNotSummon(id, false);
        char label[64];
        snprintf(label, sizeof label, "Summon saved copy (¥%d)", fee < 0 ? 0 : fee);
        ImGui::SameLine();
        ImGui::BeginDisabled(whyPay != nullptr);
        if (W::RiskButton(label, Risk::Safe, "The Velvet Room's compendium summon: same fee, same saved copy."))
            PersonaStock::Summon(id, true);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(whyFree != nullptr);
        if (W::RiskButton("Summon free", Risk::Caution, "The compendium summon without the fee."))
            PersonaStock::Summon(id, false);
        ImGui::EndDisabled();
        if (!why && whyPay && !whyFree) W::Dim("%s", whyPay);
    }
    if (why) W::Dim("%s", why);
}

void DrawGive(const Game::Snapshot& s) {
    float sc = Overlay::Scale();
    SectionHeader("GET A PERSONA");
    if (!PersonaStock::Available()) {
        W::Dim("Unavailable on this game build (a host check failed).");
        return;
    }
    W::Dim("Every Persona Yu can hold, by arcana, with its base level. Party members' Personas aren't listed.");
    ImGui::SetNextItemWidth(240 * sc);
    ImGui::InputTextWithHint("##pf", "Filter by name", g_pickFilter, sizeof g_pickFilter);
    if (g_pick > 0) DrawPickDetail(s, uint16_t(g_pick));
    ImGui::BeginChild("give", ImVec2(0, 300 * sc), ImGuiChildFlags_Borders);
    int cols = W::Columns(190 * sc);
    for (int arc = 0; arc < 32; ++arc) {
        int n = 0;
        for (uint16_t id = 1; id < 256; ++id) {
            auto i = PersonaStock::Describe(id);
            if (!i.yours || i.arcana != arc) continue;
            bool vis = NgVisible(id, i);
            std::string nm = vis ? Names::PersonaOr(id) : std::string("???");
            if (!Contains(nm, g_pickFilter)) continue;
            if (n == 0) {
                ImGui::Spacing();
                if (!ArcanaVisible(s, uint8_t(arc))) {
                    // The whole arcana is a spoiler: one placeholder instead of its Personas.
                    W::Chip("???", C::PanelHi, C::Grey);
                    ImGui::SameLine();
                    Spoilers::HiddenPlaceholder(Spoilers::Kind::Link, ArcanaLink(uint8_t(arc)),
                                                "an arcana you haven't met yet");
                    break;
                }
                // The World arcana holds only the New Game+ Persona; its name is an ending spoiler.
                std::string an = (arc == 24 && !vis) ? std::string("???") : Names::Arcana(uint8_t(arc));
                W::Chip(an.empty() ? "?" : an.c_str(), W::ArcanaColor(uint8_t(arc)));
            }
            if (n++ % cols) ImGui::SameLine();
            ImGui::PushID(id);
            char label[64];
            bool have = PersonaStock::SlotOf(id) >= 0;
            snprintf(label, sizeof label, "%s%s  %u", have ? "\xE2\x9C\x93 " : "", nm.c_str(), i.baseLevel);
            if (have) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(C::Green));
            if (ImGui::Selectable(label, g_pick == id, 0, ImVec2(176 * sc, 0))) g_pick = id;
            if (have) ImGui::PopStyleColor();
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
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
    if (PersonaStock::Available()) DrawStockManager(s);
    DrawGive(s);

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
