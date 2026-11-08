#include "UI/Theme.h"

#include <windows.h>

#include "Util/Log.h"
#include "Util/Paths.h"

namespace Theme {

Fonts g_fonts;

ImU32 RiskColor(Risk r) {
    switch (r) {
        case Risk::Safe: return C::Green;
        case Risk::Caution: return C::Yellow;
        case Risk::Story: return C::Orange;
        case Risk::Dangerous: return C::Red;
    }
    return C::Grey;
}

const char* RiskName(Risk r) {
    switch (r) {
        case Risk::Safe: return "SAFE";
        case Risk::Caution: return "CAUTION";
        case Risk::Story: return "STORY-SENSITIVE";
        case Risk::Dangerous: return "DANGEROUS";
    }
    return "";
}

static ImFont* TryFont(const std::string& dir, const char* file, float px,
                       const ImWchar* ranges = nullptr) {
    std::string path = dir + "\\" + file;
    if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) return nullptr;
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path.c_str(), px, &cfg, ranges);
}

// Segoe UI has few symbols (no check mark, few dingbats); Segoe UI Symbol fills them in.
static void MergeSymbols(const std::string& dir, float px) {
    static const ImWchar sym[] = {0x2190, 0x21FF, 0x2600, 0x27BF, 0};
    std::string path = dir + "\\seguisym.ttf";
    if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) return;
    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    ImGui::GetIO().Fonts->AddFontFromFileTTF(path.c_str(), px, &cfg, sym);
}

void LoadFonts(float scale) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    std::string dir = Paths::Narrow(Paths::FontsDir());

    static const ImWchar ranges[] = {
        0x0020, 0x00FF,  // Latin-1
        0x2010, 0x2027,  // dashes, bullets, ellipsis
        0x2190, 0x21FF,  // arrows
        0x2600, 0x26FF,  // misc symbols (hearts, stars)
        0x2700, 0x27BF,  // dingbats (check marks)
        0,
    };
    auto load = [&](const char* file, float px) {
        ImFont* f = TryFont(dir, file, px, ranges);
        if (f) MergeSymbols(dir, px);
        return f;
    };
    g_fonts.body = load("segoeui.ttf", 19.0f * scale);
    g_fonts.bold = load("seguisb.ttf", 19.0f * scale);
    g_fonts.title = load("seguibl.ttf", 30.0f * scale);
    g_fonts.note = load("segoeui.ttf", 15.0f * scale);
    if (!g_fonts.body) {
        LOGW("Segoe UI not found in the Fonts folder; using ImGui's built-in font");
        ImFontConfig cfg;
        cfg.SizePixels = 15.0f * scale;
        g_fonts.body = io.Fonts->AddFontDefault(&cfg);
    }
    if (!g_fonts.bold) g_fonts.bold = g_fonts.body;
    if (!g_fonts.title) g_fonts.title = g_fonts.bold;
    if (!g_fonts.note) g_fonts.note = g_fonts.body;
    io.FontDefault = g_fonts.body;
}

void Apply(float scale) {
    ImGuiStyle& s = ImGui::GetStyle();
    s = ImGuiStyle();
    s.WindowRounding = 0;
    s.ChildRounding = 0;
    s.FrameRounding = 0;
    s.PopupRounding = 0;
    s.ScrollbarRounding = 0;
    s.GrabRounding = 0;
    s.TabRounding = 0;
    s.WindowBorderSize = 0;
    s.ChildBorderSize = 1;
    s.FrameBorderSize = 0;
    s.WindowPadding = ImVec2(14, 12);
    s.FramePadding = ImVec2(10, 6);
    s.ItemSpacing = ImVec2(10, 8);
    s.ItemInnerSpacing = ImVec2(8, 6);
    s.ScrollbarSize = 12;
    s.GrabMinSize = 12;
    s.ScaleAllSizes(scale);

    ImVec4* c = s.Colors;
    auto v = [](ImU32 u) { return ImGui::ColorConvertU32ToFloat4(u); };
    c[ImGuiCol_Text] = v(C::White);
    c[ImGuiCol_TextDisabled] = v(C::Grey);
    c[ImGuiCol_WindowBg] = v(C::Panel);
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = v(IM_COL32(18, 18, 16, 252));
    c[ImGuiCol_Border] = v(IM_COL32(255, 225, 0, 90));
    c[ImGuiCol_FrameBg] = v(IM_COL32(48, 46, 40, 255));
    c[ImGuiCol_FrameBgHovered] = v(IM_COL32(70, 66, 50, 255));
    c[ImGuiCol_FrameBgActive] = v(IM_COL32(90, 84, 50, 255));
    c[ImGuiCol_TitleBg] = v(C::Black);
    c[ImGuiCol_TitleBgActive] = v(C::Black);
    c[ImGuiCol_ScrollbarBg] = v(IM_COL32(0, 0, 0, 80));
    c[ImGuiCol_ScrollbarGrab] = v(C::YellowDim);
    c[ImGuiCol_ScrollbarGrabHovered] = v(C::Yellow);
    c[ImGuiCol_ScrollbarGrabActive] = v(C::Orange);
    c[ImGuiCol_CheckMark] = v(C::Yellow);
    c[ImGuiCol_SliderGrab] = v(C::Yellow);
    c[ImGuiCol_SliderGrabActive] = v(C::Orange);
    c[ImGuiCol_Button] = v(IM_COL32(56, 53, 44, 255));
    c[ImGuiCol_ButtonHovered] = v(C::YellowDim);
    c[ImGuiCol_ButtonActive] = v(C::Orange);
    c[ImGuiCol_Header] = v(IM_COL32(255, 225, 0, 60));
    c[ImGuiCol_HeaderHovered] = v(IM_COL32(255, 225, 0, 110));
    c[ImGuiCol_HeaderActive] = v(IM_COL32(255, 225, 0, 160));
    c[ImGuiCol_Separator] = v(IM_COL32(255, 225, 0, 70));
    c[ImGuiCol_Tab] = v(IM_COL32(40, 38, 32, 255));
    c[ImGuiCol_TabHovered] = v(C::YellowDim);
    c[ImGuiCol_TabSelected] = v(C::Yellow);
    c[ImGuiCol_TableHeaderBg] = v(IM_COL32(36, 34, 28, 255));
    c[ImGuiCol_TableBorderStrong] = v(IM_COL32(255, 225, 0, 60));
    c[ImGuiCol_TableBorderLight] = v(IM_COL32(255, 255, 255, 25));
    c[ImGuiCol_TableRowBgAlt] = v(IM_COL32(255, 255, 255, 8));
    c[ImGuiCol_NavCursor] = v(C::Orange);
    c[ImGuiCol_ModalWindowDimBg] = v(IM_COL32(0, 0, 0, 160));
    c[ImGuiCol_PlotHistogram] = v(C::Yellow);
}

void SlantRect(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float slant, ImU32 col) {
    ImVec2 p[4] = {ImVec2(mn.x + slant, mn.y), ImVec2(mx.x, mn.y), ImVec2(mx.x - slant, mx.y),
                   ImVec2(mn.x, mx.y)};
    dl->AddConvexPolyFilled(p, 4, col);
}

void Stripes(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float spacing, ImU32 col) {
    dl->PushClipRect(mn, mx, true);
    float h = mx.y - mn.y;
    for (float x = mn.x - h; x < mx.x; x += spacing)
        dl->AddLine(ImVec2(x, mx.y), ImVec2(x + h, mn.y), col, 1.0f);
    dl->PopClipRect();
}

void SectionHeader(const char* text) {
    ImGui::Spacing();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushFont(g_fonts.bold);
    ImVec2 ts = ImGui::CalcTextSize(text);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float pad = ImGui::GetStyle().FramePadding.x;
    float h = ts.y + 6;
    SlantRect(dl, p, ImVec2(p.x + ts.x + pad * 3, p.y + h), h * 0.35f, C::Yellow);
    dl->AddText(ImVec2(p.x + pad * 1.2f, p.y + 3), C::Black, text);
    float lineY = p.y + h - 1;
    dl->AddLine(ImVec2(p.x + ts.x + pad * 3, lineY),
                ImVec2(p.x + ImGui::GetContentRegionAvail().x, lineY), C::Yellow, 1.0f);
    ImGui::Dummy(ImVec2(ts.x + pad * 3, h));
    ImGui::PopFont();
}

void RiskBadge(Risk r) {
    ImGui::PushFont(g_fonts.note);
    const char* t = RiskName(r);
    ImVec2 ts = ImGui::CalcTextSize(t);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + ts.x + 10, p.y + ts.y + 2), RiskColor(r));
    dl->AddText(ImVec2(p.x + 5, p.y + 1), C::Black, t);
    ImGui::Dummy(ImVec2(ts.x + 10, ts.y + 2));
    ImGui::PopFont();
}

}  // namespace Theme
