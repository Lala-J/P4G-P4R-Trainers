#include <imgui.h>
#include <cstdio>

#include "Engine/Addresses.h"
#include "Engine/GameState.h"
#include "Engine/GameThread.h"
#include "Engine/HostCheck.h"
#include "Engine/Names.h"
#include "Engine/Natives.h"
#include "Features/Battle.h"
#include "Features/BattleOverrides.h"
#include "Features/Edits.h"
#include "Features/Spoilers.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace {

const char* CharName(int id) {
    static const char* n[] = {"", "Yu", "Yosuke", "Chie", "Yukiko", "Rise", "Kanji", "Naoto", "Teddie"};
    return (id >= 1 && id <= 8) ? n[id] : "?";
}

int JoinFlag(int c) {
    static const int f[] = {-1, -1, 48, 49, 50, 52, 51, 54, 53};
    return (c >= 0 && c <= 8) ? f[c] : -1;
}

void MemberCard(int c, float w) {
    const auto& s = Game::Current();
    float sc = Overlay::Scale();
    auto* live = reinterpret_cast<const Game::Unit*>(Addr::SaveBlock + Game::Off::Units) + (c - 1);
    const Game::Unit& u = s.U(c);
    uint16_t mh = Natives::GetMaxHP(live), ms = Natives::GetMaxSP(live);
    char id[16];
    snprintf(id, sizeof id, "unit%d", c);
    W::BeginCard(id, ImVec2(w, 140 * sc), c == 1 ? C::Yellow : C::Blue);
    W::Title(CharName(c));
    ImGui::SameLine();
    W::Dim("Lv %u", c == 1 ? u.level : u.persona.level);
    char buf[48];
    snprintf(buf, sizeof buf, "HP %u / %u", u.hp, mh);
    W::Bar(mh ? float(u.hp) / mh : 0, ImVec2(w - 30 * sc, 0), C::Green, buf);
    snprintf(buf, sizeof buf, "SP %u / %u", u.sp, ms);
    W::Bar(ms ? float(u.sp) / ms : 0, ImVec2(w - 30 * sc, 0), IM_COL32(220, 90, 200, 255), buf);
    static int hpEdit[9] = {}, spEdit[9] = {};
    static int lastHp[9] = {-1, -1, -1, -1, -1, -1, -1, -1, -1}, lastSp[9] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};
    if (lastHp[c] != u.hp) hpEdit[c] = lastHp[c] = u.hp;
    if (lastSp[c] != u.sp) spEdit[c] = lastSp[c] = u.sp;
    ImGui::PushID(c);
    ImGui::SetNextItemWidth(70 * sc);
    ImGui::DragInt("##hp", &hpEdit[c], 1, 0, mh ? mh : 999, "HP %d");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70 * sc);
    ImGui::DragInt("##sp", &spEdit[c], 1, 0, ms ? ms : 999, "SP %d");
    ImGui::SameLine();
    if (W::RiskButton("Set", Risk::Caution, "Sets HP / SP through the game's own helpers (0 to max)."))
        Edits::SetUnitHpSp(c, hpEdit[c], spEdit[c]);
    ImGui::PopID();
    W::EndCard();
}

}  // namespace

namespace Pages {

void BattlePage() {
    if (!RequireSave()) return;
    if (!HostCheck::GroupOk("battle")) {
        ImGui::TextWrapped("The battle functions didn't match this game build, so these options are off.");
        return;
    }
    auto& o = Battle::Opts();
    bool changed = false;

    SectionHeader("SURVIVAL");
    ImGui::TextUnformatted("HP");
    ImGui::SameLine(120 * Overlay::Scale());
    int hp = int(o.hp);
    changed |= ImGui::RadioButton("Normal##hp", &hp, 0);
    ImGui::SameLine();
    changed |= ImGui::RadioButton("Infinite (no damage)", &hp, 1);
    ImGui::SameLine();
    changed |= ImGui::RadioButton("Never below 1 HP", &hp, 2);
    o.hp = Battle::HpMode(hp);
    W::Dim("Applies to every way HP is lost: attacks, HP-cost skills, poison, traps.");

    changed |= ImGui::Checkbox("Infinite SP (skills cost no SP)", &o.sp);

    ImGui::TextUnformatted("Who");
    ImGui::SameLine(120 * Overlay::Scale());
    int scope = int(o.scope);
    changed |= ImGui::RadioButton("Whole party", &scope, 0);
    ImGui::SameLine();
    changed |= ImGui::RadioButton("Yu only", &scope, 1);
    o.scope = Battle::Scope(scope);
    if (changed) Battle::SaveOptions();

    ImGui::Spacing();
    if (W::RiskButton("Refill HP & SP now", Risk::Safe,
                      "Restores HP and SP to their maximums, the way the game's own full heal does."))
        GameThread::Post([] { Battle::RefillNow(); });

    // Values above the game's own maximum are usually left behind by another cheat tool.
    {
        bool over = false;
        auto* units = reinterpret_cast<const Game::Unit*>(Addr::SaveBlock + Game::Off::Units);
        for (int c = 0; c < Game::kUnits; ++c) {
            uint16_t mh = Natives::GetMaxHP(&units[c]), ms = Natives::GetMaxSP(&units[c]);
            if ((mh && units[c].hp > mh) || (ms && units[c].sp > ms)) over = true;
        }
        if (over) {
            ImGui::Spacing();
            W::Chip("ABOVE MAXIMUM", C::Orange);
            ImGui::SameLine();
            ImGui::TextWrapped("Some HP / SP values are higher than the game allows (often left by another "
                               "cheat tool). The game clamps them the next time they change.");
            if (W::RiskButton("Bring them back to the game's maximum", Risk::Safe,
                              "Runs the game's own HP/SP update with no change, which clamps each value "
                              "to its real maximum."))
                GameThread::Post([] {
                    auto* u = reinterpret_cast<Game::Unit*>(Addr::SaveBlock + Game::Off::Units);
                    for (int c = 0; c < Game::kUnits; ++c) {
                        Natives::AddHP(&u[c], 0);
                        Natives::AddSP(&u[c], 0);
                    }
                });
        }
    }

    SectionHeader("PARTY");
    const auto& s = Game::Current();
    float sc = Overlay::Scale();
    int cols = W::Columns(260 * sc);
    float gap = ImGui::GetStyle().ItemSpacing.x;
    float w = (ImGui::GetContentRegionAvail().x - gap * (cols - 1)) / cols;
    int n = 0;
    for (int c = 1; c <= 8; ++c) {
        if (c > 1 && !Spoilers::Visible(Spoilers::Kind::Persona, 1000 + c, s.Flag(JoinFlag(c)))) continue;
        if (c > 1 && !s.Flag(JoinFlag(c))) continue;
        if (n % cols) ImGui::SameLine();
        MemberCard(c, w);
        ++n;
    }

    auto& f = BattleFx::Opts();
    bool fx = false;
    auto toggle = [&](const char* label, bool* v, const char* help) {
        fx |= ImGui::Checkbox(label, v);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", help);
    };
    SectionHeader("DEFENCE");
    toggle("No knock-downs", &f.noDowns,
           "Enemies can't hit the party's weaknesses or land critical hits on them, so nobody is knocked "
           "down. Weakness damage is treated as normal damage.");
    toggle("No ailments", &f.noAilments,
           "Ailment skills are blocked (\"Block\"), and any ailment that still lands is cured at once.");
    toggle("No instant kills", &f.noInstakill, "Hama and Mudo skills are blocked against the party.");
    toggle("No stat penalties", &f.noDebuffs,
           "Attack, defence, accuracy/evasion and critical penalties on the party are removed as soon as "
           "they land. Your own buffs stay.");
    toggle("Enemies always miss", &f.enemiesMiss,
           "Every enemy attack on the party misses. Attacks the game marks as sure hits (some boss moves "
           "that drive a story scene) still connect.");
    SectionHeader("OFFENCE");
    toggle("Party never misses", &f.partyHits, "The party's attacks are never dodged.");
    toggle("Always critical", &f.partyCrits,
           "Every party attack that can be critical is (physical skills). Magic still can't crit, and "
           "bosses that can't be knocked down still can't.");
    toggle("Forced 1 More", &f.forcedOneMore,
           "Every party hit on a standing enemy knocks it down, which is how the game grants a 1 More. "
           "Hits also count as critical for damage. Enemies that can't be knocked down (most bosses) "
           "still can't; once everyone is down you get the All-Out Attack prompt.");
    toggle("Weakness override", &f.weakness,
           "Party attacks count as hitting the target's weakness: a fire skill on a shadow weak to "
           "electricity lands as an electricity hit. The skill's own power is unchanged.");
    if (f.weakness) {
        ImGui::Indent();
        toggle("Even when it has no weakness", &f.weaknessForce,
               "Treat every hit as a weakness hit (×1.5), even on targets without one.");
        ImGui::Unindent();
    }
    SectionHeader("CONTROL");
    toggle("Unlimited Persona changes", &f.unlimitedSwap,
           "Yu can change Persona as often as he likes during his turn.");
    toggle("Shadows can't run away", &f.noFlee,
           "A shadow that would flee waits instead (Golden Hands, cowardly shadows).");
    if (fx) BattleFx::SaveOptions();
    W::Dim("Every switch hooks the game's own rule for it, so the game still shows its normal "
           "\"Miss\", \"Critical\" and \"Block\" results. Turning one off takes effect on the next action.");
}

}  // namespace Pages
