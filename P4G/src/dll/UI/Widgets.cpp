#include "UI/Widgets.h"

#include <cstdarg>
#include <cfloat>
#include <cstdio>

#include "UI/Overlay.h"
#include "UI/Theme.h"

using namespace Theme;

namespace W {

bool BeginCard(const char* id, ImVec2 size, ImU32 accent) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(C::PanelHi));
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 30)));
    bool open = ImGui::BeginChild(id, size, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
                                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImVec2 p = ImGui::GetWindowPos();
    float h = ImGui::GetWindowHeight();
    ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 4 * Overlay::Scale(), p.y + h), accent);
    return open;
}

void EndCard() {
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
}

void Pips(int filled, int total, ImU32 on, ImU32 off, float size) {
    float s = size > 0 ? size : ImGui::GetTextLineHeight() * 0.55f;
    float gap = s * 0.45f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    float y = p.y + (ImGui::GetTextLineHeight() - s) * 0.5f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int i = 0; i < total; ++i) {
        ImVec2 a(p.x + i * (s + gap), y);
        ImVec2 pts[4] = {ImVec2(a.x + s * 0.5f, a.y), ImVec2(a.x + s, a.y + s * 0.5f),
                         ImVec2(a.x + s * 0.5f, a.y + s), ImVec2(a.x, a.y + s * 0.5f)};
        if (i < filled) dl->AddConvexPolyFilled(pts, 4, on);
        else dl->AddPolyline(pts, 4, off, ImDrawFlags_Closed, 1.0f);
    }
    ImGui::Dummy(ImVec2(total * (s + gap), ImGui::GetTextLineHeight()));
}

void Bar(float frac, ImVec2 size, ImU32 fill, const char* overlay) {
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    if (size.x <= 0) size.x = ImGui::GetContentRegionAvail().x;
    if (size.y <= 0) size.y = ImGui::GetTextLineHeight() * 0.9f;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(0, 0, 0, 120));
    dl->AddRectFilled(p, ImVec2(p.x + size.x * frac, p.y + size.y), fill);
    if (overlay) {
        ImGui::PushFont(g_fonts.note);
        ImVec2 ts = ImGui::CalcTextSize(overlay);
        ImVec2 tp(p.x + 6, p.y + (size.y - ts.y) * 0.5f);
        // Dark outline so the label reads on any fill colour (yellow included).
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy)
                if (dx || dy) dl->AddText(ImVec2(tp.x + dx, tp.y + dy), IM_COL32(0, 0, 0, 230), overlay);
        dl->AddText(tp, C::White, overlay);
        ImGui::PopFont();
    }
    ImGui::Dummy(size);
}

void Chip(const char* text, ImU32 bg, ImU32 fg) {
    ImGui::PushFont(g_fonts.note);
    ImVec2 ts = ImGui::CalcTextSize(text);
    ImVec2 p = ImGui::GetCursorScreenPos();
    float padX = 6 * Overlay::Scale(), padY = 1;
    float y = p.y + (ImGui::GetTextLineHeight() - ts.y - padY * 2) * 0.5f;
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x, y), ImVec2(p.x + ts.x + padX * 2, y + ts.y + padY * 2), bg);
    ImGui::GetWindowDrawList()->AddText(ImVec2(p.x + padX, y + padY), fg, text);
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(ts.x + padX * 2, ImGui::GetTextLineHeight()));
}

void Dim(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    ImGui::PushFont(g_fonts.note);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(C::Grey), "%s", buf);
    ImGui::PopFont();
}

void Title(const char* text) {
    ImGui::PushFont(g_fonts.bold);
    ImGui::TextUnformatted(text);
    ImGui::PopFont();
}

ImU32 ArcanaColor(int a) {
    // One hue per arcana, spread around the wheel, kept bright enough to read on dark panels.
    static const ImU32 c[] = {
        IM_COL32(150, 150, 150, 255), IM_COL32(240, 240, 240, 255), IM_COL32(230, 70, 60, 255),
        IM_COL32(90, 150, 240, 255),  IM_COL32(240, 140, 200, 255), IM_COL32(230, 160, 40, 255),
        IM_COL32(180, 130, 90, 255),  IM_COL32(240, 90, 150, 255),  IM_COL32(120, 200, 90, 255),
        IM_COL32(250, 220, 90, 255),  IM_COL32(130, 120, 200, 255), IM_COL32(80, 200, 200, 255),
        IM_COL32(230, 110, 40, 255),  IM_COL32(160, 160, 200, 255), IM_COL32(110, 110, 120, 255),
        IM_COL32(120, 200, 230, 255), IM_COL32(170, 50, 60, 255),   IM_COL32(200, 200, 140, 255),
        IM_COL32(110, 160, 250, 255), IM_COL32(200, 160, 250, 255), IM_COL32(250, 190, 60, 255),
        IM_COL32(230, 230, 200, 255), IM_COL32(150, 220, 160, 255), IM_COL32(200, 120, 220, 255),
    };
    return c[(a >= 0 ? a : 0) % (int)(sizeof c / sizeof c[0])];
}

bool RiskButton(const char* label, Theme::Risk risk, const char* why) {
    bool clicked = ImGui::Button(label);
    if (ImGui::IsItemHovered() && why && *why) {
        ImGui::BeginTooltip();
        RiskBadge(risk);
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28);
        ImGui::TextUnformatted(why);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    if (risk == Risk::Safe || risk == Risk::Caution) return clicked;

    char popup[160];
    snprintf(popup, sizeof popup, "Confirm##%s", label);
    if (clicked) ImGui::OpenPopup(popup);
    bool apply = false;
    ImGui::SetNextWindowSizeConstraints(ImVec2(420 * Overlay::Scale(), 0), ImVec2(720 * Overlay::Scale(), FLT_MAX));
    if (ImGui::BeginPopupModal(popup, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        RiskBadge(risk);
        ImGui::PushFont(g_fonts.bold);
        ImGui::TextUnformatted(label);
        ImGui::PopFont();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 34);
        ImGui::TextUnformatted(why ? why : "");
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(RiskColor(risk)));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(C::Black));
        if (ImGui::Button("Apply anyway")) {
            apply = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
        if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    return apply;
}

int Columns(float minCardWidth) {
    float avail = ImGui::GetContentRegionAvail().x;
    int n = (int)(avail / minCardWidth);
    return n < 1 ? 1 : n;
}

}  // namespace W
