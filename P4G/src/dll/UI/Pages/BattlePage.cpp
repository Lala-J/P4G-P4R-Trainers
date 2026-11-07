#include <imgui.h>
#include <cstdio>

#include "Engine/Addresses.h"
#include "Engine/GameState.h"
#include "Engine/GameThread.h"
#include "Engine/HostCheck.h"
#include "Engine/Names.h"
#include "Engine/Natives.h"
#include "Features/Battle.h"
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
    W::BeginCard(id, ImVec2(w, 104 * sc), c == 1 ? C::Yellow : C::Blue);
    W::Title(CharName(c));
    ImGui::SameLine();
    W::Dim("Lv %u", c == 1 ? u.level : u.persona.level);
    char buf[48];
    snprintf(buf, sizeof buf, "HP %u / %u", u.hp, mh);
    W::Bar(mh ? float(u.hp) / mh : 0, ImVec2(w - 30 * sc, 0), C::Green, buf);
    snprintf(buf, sizeof buf, "SP %u / %u", u.sp, ms);
    W::Bar(ms ? float(u.sp) / ms : 0, ImVec2(w - 30 * sc, 0), IM_COL32(220, 90, 200, 255), buf);
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

    SectionHeader("MORE");
    W::Dim("No ailments or knock-downs, enemies always miss, always crit, forced 1 More, the weakness "
           "override and unlimited Persona swaps arrive in milestone M4c.");
}

}  // namespace Pages
