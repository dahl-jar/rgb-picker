#include "gui/ui/widgets.h"

#include "gui/ui/theme.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace rgbpicker::gui {

void helpMarker(const char* text)
{
    constexpr float radius{8.0f};
    const ImVec2 origin{ImGui::GetCursorScreenPos()};
    const float rowHeight{ImGui::GetFrameHeight()};
    ImGui::Dummy(ImVec2{radius * 2.0f, rowHeight});
    const bool hovered{ImGui::IsItemHovered()};

    const ImVec2 centre{origin.x + radius, origin.y + rowHeight * 0.5f};
    const ImU32 ink{ImGui::ColorConvertFloat4ToU32(hovered ? kText : kTextMuted)};
    ImDrawList* draw{ImGui::GetWindowDrawList()};
    draw->AddCircle(centre, radius, ink, 0, 1.3f);
    const ImVec2 glyph{ImGui::CalcTextSize("?")};
    draw->AddText(ImVec2{centre.x - glyph.x * 0.5f, centre.y - glyph.y * 0.5f}, ink, "?");

    if (!hovered) {
        return;
    }
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 22.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void dottedRule()
{
    ImGui::Dummy(ImVec2{0.0f, 3.0f});
    const ImVec2 start{ImGui::GetCursorScreenPos()};
    const float width{ImGui::GetContentRegionAvail().x};
    ImDrawList* draw{ImGui::GetWindowDrawList()};
    const ImU32 color{ImGui::ColorConvertFloat4ToU32(kHairline)};
    for (float offset{rowGutter}; offset < width - rowGutter; offset += 6.0f) {
        draw->AddLine(ImVec2{start.x + offset, start.y},
                      ImVec2{start.x + std::min(offset + 3.0f, width - rowGutter), start.y}, color);
    }
    ImGui::Dummy(ImVec2{0.0f, 8.0f});
}

std::string fitText(const char* text, float maxWidth)
{
    if (ImGui::CalcTextSize(text).x <= maxWidth) {
        return text;
    }
    std::string clipped{text};
    while (!clipped.empty() && ImGui::CalcTextSize((clipped + "...").c_str()).x > maxWidth) {
        clipped.pop_back();
    }
    return clipped + "...";
}

void segmentedToggle(const char* id, bool& value)
{
    constexpr float halfWidth{58.0f};
    ImGui::PushID(id);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{2.0f, 0.0f});
    ImGui::PushStyleColor(ImGuiCol_Button, value ? kAccent : kField);
    ImGui::PushStyleColor(ImGuiCol_Text, value ? kGround : kTextMuted);
    if (ImGui::Button("ON", ImVec2{halfWidth, 0.0f})) {
        value = true;
    }
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, value ? kField : kSelection);
    ImGui::PushStyleColor(ImGuiCol_Text, value ? kTextMuted : kText);
    if (ImGui::Button("OFF", ImVec2{halfWidth, 0.0f})) {
        value = false;
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
    ImGui::PopID();
}

bool sectionBand(const char* title, bool& open)
{
    const float width{ImGui::GetContentRegionAvail().x};
    const ImVec2 origin{ImGui::GetCursorScreenPos()};
    ImGui::InvisibleButton(title, ImVec2{width, bandHeight});
    if (ImGui::IsItemClicked()) {
        open = !open;
    }

    ImDrawList* draw{ImGui::GetWindowDrawList()};
    const ImVec2 corner{origin.x + width, origin.y + bandHeight};
    draw->AddRectFilled(origin, corner,
                        ImGui::ColorConvertFloat4ToU32(ImGui::IsItemHovered() ? kBandHover
                                                                             : kSectionBand));
    const ImU32 rule{ImGui::ColorConvertFloat4ToU32(kHairline)};
    draw->AddLine(origin, ImVec2{corner.x, origin.y}, rule);
    draw->AddLine(ImVec2{origin.x, corner.y}, corner, rule);

    const float textY{origin.y + (bandHeight - ImGui::GetTextLineHeight()) * 0.5f};
    const ImU32 titleColor{ImGui::ColorConvertFloat4ToU32(kTextMuted)};
    draw->AddText(ImVec2{origin.x + rowGutter, textY}, titleColor, title);
    const char* const glyph{open ? "-" : "+"};
    draw->AddText(ImVec2{origin.x + width - rowGutter - ImGui::CalcTextSize(glyph).x, textY},
                  titleColor, glyph);

    ImGui::Dummy(ImVec2{0.0f, open ? 10.0f : 2.0f});
    return open;
}

void rowLabel(const char* label, const char* help)
{
    constexpr float markerWidth{26.0f};
    const float controlX{ImGui::GetWindowContentRegionMax().x - rowControlWidth - rowGutter};
    const float labelWidth{controlX - rowGutter * 2.0f - (help == nullptr ? 0.0f : markerWidth)};

    ImGui::Indent(rowGutter);
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, kTextSoft);
    ImGui::TextUnformatted(fitText(label, labelWidth).c_str());
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered() && ImGui::CalcTextSize(label).x > labelWidth) {
        ImGui::SetTooltip("%s", label);
    }
    ImGui::Unindent(rowGutter);
    if (help != nullptr) {
        ImGui::SameLine(0.0f, 8.0f);
        helpMarker(help);
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX(controlX);
}

void trashIcon(ImDrawList* draw, ImVec2 centre, ImU32 ink)
{
    constexpr float halfWidth{5.0f};
    constexpr float bodyTop{-3.0f};
    constexpr float bodyBottom{6.0f};
    draw->AddLine(ImVec2{centre.x - halfWidth - 1.0f, centre.y - 5.0f},
                  ImVec2{centre.x + halfWidth + 1.0f, centre.y - 5.0f}, ink, 1.5f);
    draw->AddLine(ImVec2{centre.x - 2.0f, centre.y - 7.5f},
                  ImVec2{centre.x + 2.0f, centre.y - 7.5f}, ink, 1.5f);
    draw->AddRect(ImVec2{centre.x - halfWidth, centre.y + bodyTop},
                  ImVec2{centre.x + halfWidth, centre.y + bodyBottom}, ink, 1.0f, 0, 1.5f);
    draw->AddLine(ImVec2{centre.x - 1.8f, centre.y - 1.0f},
                  ImVec2{centre.x - 1.8f, centre.y + 4.0f}, ink, 1.2f);
    draw->AddLine(ImVec2{centre.x + 1.8f, centre.y - 1.0f},
                  ImVec2{centre.x + 1.8f, centre.y + 4.0f}, ink, 1.2f);
}

bool settingsButton(const char* id, float size)
{
    const ImVec2 origin{ImGui::GetCursorScreenPos()};
    ImGui::InvisibleButton(id, ImVec2{size, size});
    const bool hovered{ImGui::IsItemHovered()};
    const bool clicked{ImGui::IsItemClicked()};

    ImDrawList* draw{ImGui::GetWindowDrawList()};
    const ImVec2 centre{origin.x + size * 0.5f, origin.y + size * 0.5f};
    const ImU32 ink{ImGui::ColorConvertFloat4ToU32(hovered ? kText : kTextMuted)};

    constexpr int teeth{7};
    constexpr float twoPi{6.2831853f};
    const float step{twoPi / teeth};
    const float tip{size * 0.40f};
    const float root{size * 0.29f};
    const float toothHalf{step * 0.20f};
    const float flank{step * 0.09f};

    std::array<ImVec2, teeth * 4> outline{};
    const auto at{[&](float angle, float radius) {
        return ImVec2{centre.x + std::cos(angle) * radius, centre.y + std::sin(angle) * radius};
    }};
    for (int tooth{0}; tooth < teeth; ++tooth) {
        const float base{static_cast<float>(tooth) * step};
        const std::size_t slot{static_cast<std::size_t>(tooth) * 4};
        outline[slot] = at(base - toothHalf, tip);
        outline[slot + 1] = at(base + toothHalf, tip);
        outline[slot + 2] = at(base + toothHalf + flank, root);
        outline[slot + 3] = at(base + step - toothHalf - flank, root);
    }
    draw->AddPolyline(outline.data(), static_cast<int>(outline.size()), ink, ImDrawFlags_Closed,
                      1.6f);
    draw->AddCircle(centre, size * 0.14f, ink, 16, 1.6f);

    if (hovered) {
        ImGui::SetTooltip("Settings");
    }
    return clicked;
}

void drawRailHeader(const char* title)
{
    const ImVec2 pos{ImGui::GetCursorScreenPos()};
    const float width{ImGui::GetContentRegionAvail().x};
    const ImVec2 end{pos.x + width, pos.y + bandHeight};
    ImDrawList* draw{ImGui::GetWindowDrawList()};
    draw->AddRectFilled(pos, end, ImGui::ColorConvertFloat4ToU32(kSectionBand));
    const ImU32 rule{ImGui::ColorConvertFloat4ToU32(kHairline)};
    draw->AddLine(pos, ImVec2{end.x, pos.y}, rule);
    draw->AddLine(ImVec2{pos.x, end.y}, end, rule);
    draw->AddText(
        ImVec2{pos.x + rowGutter, pos.y + (bandHeight - ImGui::GetTextLineHeight()) * 0.5f},
        ImGui::ColorConvertFloat4ToU32(kTextMuted), title);
    ImGui::Dummy(ImVec2{0.0f, bandHeight});
}
}
