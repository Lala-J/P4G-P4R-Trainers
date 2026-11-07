#include "Features/Spoilers.h"

#include <imgui.h>
#include <cstdio>
#include <set>

#include "UI/Theme.h"
#include "Util/Settings.h"

namespace {

std::set<int> g_revealed;
bool g_loaded = false;
int g_policy = 0;

int Key(Spoilers::Kind k, int id) { return (int(k) << 20) | (id & 0xFFFFF); }

void EnsureLoaded() {
    if (g_loaded) return;
    g_loaded = true;
    g_policy = Settings::GetInt("spoilers", "policy", 0);
    int n = Settings::GetInt("reveal", "count", 0);
    for (int i = 0; i < n; ++i) {
        char key[24];
        snprintf(key, sizeof key, "k%d", i);
        int v = Settings::GetInt("reveal", key, -1);
        if (v >= 0) g_revealed.insert(v);
    }
}

void Save() {
    int i = 0;
    for (int v : g_revealed) {
        char key[24];
        snprintf(key, sizeof key, "k%d", i++);
        Settings::SetInt("reveal", key, v);
    }
    Settings::SetInt("reveal", "count", i);
}

}  // namespace

namespace Spoilers {

Policy GetPolicy() {
    EnsureLoaded();
    return Policy(g_policy);
}

void SetPolicy(Policy p) {
    EnsureLoaded();
    g_policy = int(p);
    Settings::SetInt("spoilers", "policy", g_policy);
}

bool Revealed(Kind k, int id) {
    EnsureLoaded();
    return g_revealed.count(Key(k, id)) != 0;
}

void Reveal(Kind k, int id) {
    EnsureLoaded();
    if (g_revealed.insert(Key(k, id)).second) Save();
}

bool Visible(Kind k, int id, bool naturallyVisible) {
    return naturallyVisible || GetPolicy() == Policy::ShowAll || Revealed(k, id);
}

bool HiddenPlaceholder(Kind k, int id, const char* what) {
    ImGui::PushID(Key(k, id));
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::C::Grey), "???");
    if (what) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", what);
    }
    ImGui::SameLine();
    bool clicked = ImGui::SmallButton("Reveal");
    if (clicked) Reveal(k, id);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Hidden because it's past your current point in the story.\n"
                          "Reveal shows just this one item, and remembers it.");
    ImGui::PopID();
    return clicked;
}

}  // namespace Spoilers
