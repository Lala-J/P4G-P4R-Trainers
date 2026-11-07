#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "Data/Links.h"
#include "Engine/GameState.h"
#include "Engine/Names.h"
#include "Features/Spoilers.h"
#include "UI/Overlay.h"
#include "UI/Pages/Pages.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

using namespace Theme;

namespace {

struct Row {
    const Data::LinkInfo* info;
    const Game::Link* link;  // null = not started
    uint16_t liveId;         // id stored in the array (romance row when dating)
    uint8_t arcana;
    bool dating, lockedOut, ready, maxed, visible;
    uint16_t threshold;
};

int g_filter = 0;  // 0 all, 1 started, 2 ready, 3 not started

std::vector<Row> BuildRows(const Game::Snapshot& s) {
    std::vector<Row> rows;
    for (const auto& li : Data::kLinks) {
        if (li.kind == Data::LinkKind::RomanceRow || li.kind == Data::LinkKind::Unused) continue;
        Row r{};
        r.info = &li;
        r.link = s.FindLink(li.id);
        r.liveId = li.id;
        if (!r.link && li.romanceRow) {
            r.link = s.FindLink(li.romanceRow);
            if (r.link) {
                r.liveId = li.romanceRow;
                r.dating = true;
            }
        }
        if (r.link && r.link->rank == 0) r.link = nullptr;
        if (li.exclusive) {
            const Game::Link* other = s.FindLink(li.exclusive);
            if (!other && Data::Link(li.exclusive)->romanceRow)
                other = s.FindLink(Data::Link(li.exclusive)->romanceRow);
            r.lockedOut = !r.link && other && other->rank > 0;
        }
        r.arcana = Game::LinkArcana(li.id);
        if (r.link) {
            r.maxed = r.link->rank >= 10;
            r.threshold = Game::LinkThreshold(r.liveId, r.link->rank);
            r.ready = !r.maxed && li.kind == Data::LinkKind::Normal && r.link->points >= r.threshold;
        }
        bool natural = r.link || (li.revealDay >= 0 && s.day >= li.revealDay);
        r.visible = Spoilers::Visible(Spoilers::Kind::Link, li.id, natural);
        rows.push_back(r);
    }
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.arcana < b.arcana; });
    return rows;
}

void DrawCard(const Row& r, float w) {
    float sc = Overlay::Scale();
    ImU32 accent = r.visible ? W::ArcanaColor(r.arcana) : C::Grey;
    char id[16];
    snprintf(id, sizeof id, "link%u", r.info->id);
    W::BeginCard(id, ImVec2(w, 128 * sc), accent);

    if (!r.visible) {
        W::Dim("Social link");
        Spoilers::HiddenPlaceholder(Spoilers::Kind::Link, r.info->id, "not met yet");
        W::EndCard();
        return;
    }

    std::string arcana = Names::Arcana(r.arcana);
    W::Chip(arcana.empty() ? "?" : arcana.c_str(), accent);
    ImGui::SameLine();
    if (r.maxed) W::Chip("MAX", C::Yellow);
    else if (r.ready) W::Chip("RANK UP READY", C::Green);
    else if (r.lockedOut) W::Chip("LOCKED OUT", C::Red, C::White);
    else if (!r.link) W::Chip("NOT STARTED", C::PanelHi, C::Grey);
    if (r.dating) {
        ImGui::SameLine();
        W::Chip("DATING", IM_COL32(240, 90, 150, 255));
    }

    W::Title(r.info->person);
    int rank = r.link ? r.link->rank : 0;
    W::Pips(rank, 10, accent, IM_COL32(255, 255, 255, 60));
    ImGui::SameLine();
    ImGui::Text("Rank %d", rank);

    if (r.info->kind == Data::LinkKind::Story) {
        W::Dim("Advances with the story");
    } else if (r.link && !r.maxed) {
        char buf[96];
        if (r.ready)
            snprintf(buf, sizeof buf, "%u pts — ranks up at the next hangout", r.link->points);
        else
            snprintf(buf, sizeof buf, "%u / %u pts to rank up", r.link->points, r.threshold);
        float frac = r.threshold ? float(r.link->points) / r.threshold : 1.0f;
        W::Bar(frac, ImVec2(w - 30 * sc, 0), r.ready ? C::Green : accent, buf);
    } else if (r.lockedOut) {
        W::Dim("You picked the other club / link");
    }
    W::EndCard();
}

}  // namespace

namespace Pages {

void SocialLinks() {
    if (!RequireSave()) return;
    const auto& s = Game::Current();
    auto rows = BuildRows(s);

    int started = 0, ready = 0, maxed = 0;
    for (const auto& r : rows) {
        started += r.link != nullptr;
        ready += r.ready;
        maxed += r.maxed;
    }
    ImGui::Text("%d started  ·  %d ready to rank up  ·  %d maxed", started, ready, maxed);

    const char* filters[] = {"All", "Started", "Ready", "Not started"};
    for (int i = 0; i < 4; ++i) {
        if (i) ImGui::SameLine();
        bool on = g_filter == i;
        if (on) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(C::Yellow));
        if (on) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(C::Black));
        if (ImGui::Button(filters[i])) g_filter = i;
        if (on) ImGui::PopStyleColor(2);
    }
    ImGui::Spacing();

    float sc = Overlay::Scale();
    int cols = W::Columns(300 * sc);
    float gap = ImGui::GetStyle().ItemSpacing.x;
    float w = (ImGui::GetContentRegionAvail().x - gap * (cols - 1)) / cols;
    int n = 0;
    for (const auto& r : rows) {
        bool show = g_filter == 0 || (g_filter == 1 && r.link) || (g_filter == 2 && r.ready) ||
                    (g_filter == 3 && !r.link);
        if (!show) continue;
        if (n % cols) ImGui::SameLine();
        DrawCard(r, w);
        ++n;
    }
    if (!n) W::Dim("Nothing matches this filter.");
    ImGui::Spacing();
    W::Dim("Points needed are read from the game's own link table. \"Ready\" means the next "
           "hangout with that person raises the rank.");
}

}  // namespace Pages
