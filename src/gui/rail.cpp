#include "gui/rail.h"

#include "gui/app_state.h"
#include "gui/theme.h"
#include "gui/widgets.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace rgbpicker::gui {

namespace {

void drawDeleteConfirm(int& pendingDelete)
{
    constexpr float cardWidth{330.0f};
    constexpr float buttonWidth{96.0f};
    const ImVec2 centre{ImGui::GetMainViewport()->GetCenter()};
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{cardWidth, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{20.0f, 18.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, kSectionBand);
    ImGui::PushStyleColor(ImGuiCol_Border, kFieldBorder);

    if (ImGui::BeginPopupModal("Delete profile", nullptr,
                               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoMove)) {
        const bool valid{pendingDelete >= 0 &&
                         pendingDelete < static_cast<int>(g_profiles.size())};
        ImGui::PushFont(g_titleFont);
        ImGui::PushStyleColor(ImGuiCol_Text, kText);
        ImGui::TextUnformatted("Delete profile");
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::Dummy(ImVec2{0.0f, 10.0f});
        ImGui::PushStyleColor(ImGuiCol_Text, kTextMuted);
        ImGui::TextWrapped("\"%s\" is removed for good. The devices keep whatever is lit now.",
                           valid ? g_profiles[static_cast<std::size_t>(pendingDelete)].name.c_str()
                                 : "");
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2{0.0f, 16.0f});
        if (ImGui::Button("Cancel", ImVec2{buttonWidth, 32.0f})) {
            pendingDelete = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - buttonWidth);
        ImGui::PushStyleColor(ImGuiCol_Button, kDangerFill);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kDangerHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, kDangerFill);
        ImGui::PushStyleColor(ImGuiCol_Text, kText);
        if (ImGui::Button("Delete", ImVec2{buttonWidth, 32.0f})) {
            if (valid) {
                const std::string name{g_profiles[static_cast<std::size_t>(pendingDelete)].name};
                rgbpicker::removeProfile(g_profiles, name);
                saveProfiles();
                if (g_settings.activeProfile == name) {
                    setActiveProfile({});
                }
            }
            pendingDelete = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);
        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

constexpr float rowMenuWidth{30.0f};

struct RowBounds {
    ImVec2 pos{};
    ImVec2 end{};
    float height{};

    float centreY() const { return pos.y + height * 0.5f; }
};

struct RowLook {
    float height{};
    bool selected{false};
    bool menuActive{false};
};

struct RowOutcome {
    bool applyProfile{false};
    bool toggleMenu{false};
    bool deleteRequested{false};
    bool claimedClick{false};
};

void drawRowDots(const RowBounds& row, bool visible, bool menuActive, RowOutcome& outcome)
{
    ImDrawList* draw{ImGui::GetWindowDrawList()};
    const float menuX{row.end.x - rowMenuWidth};

    ImGui::SetCursorScreenPos(ImVec2{menuX, row.pos.y});
    if (ImGui::InvisibleButton("##menu", ImVec2{rowMenuWidth, row.height})) {
        outcome.toggleMenu = true;
    }
    const bool hovered{ImGui::IsItemHovered()};
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        outcome.claimedClick = true;
    }
    if (!visible && !hovered && !menuActive) {
        return;
    }
    const ImU32 ink{ImGui::ColorConvertFloat4ToU32(hovered || menuActive ? kText : kTextMuted)};
    for (int dot{-1}; dot <= 1; ++dot) {
        draw->AddCircleFilled(
            ImVec2{menuX + rowMenuWidth * 0.5f, row.centreY() + static_cast<float>(dot) * 5.0f},
            1.7f, ink);
    }
}

void drawRowTrash(const RowBounds& row, RowOutcome& outcome)
{
    ImDrawList* draw{ImGui::GetWindowDrawList()};
    const float trashX{row.end.x - rowMenuWidth * 2.0f};

    ImGui::SetCursorScreenPos(ImVec2{trashX, row.pos.y});
    outcome.deleteRequested = ImGui::InvisibleButton("##trash", ImVec2{rowMenuWidth, row.height});
    const bool hovered{ImGui::IsItemHovered()};
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        outcome.claimedClick = true;
    }
    if (hovered) {
        draw->AddRectFilled(ImVec2{trashX + 2.0f, row.pos.y + 4.0f},
                            ImVec2{trashX + rowMenuWidth - 2.0f, row.end.y - 4.0f},
                            ImGui::ColorConvertFloat4ToU32(kFieldHover), 2.0f);
    }
    trashIcon(draw, ImVec2{trashX + rowMenuWidth * 0.5f, row.centreY()},
              ImGui::ColorConvertFloat4ToU32(hovered ? kDangerHover : kDestructive));
}

void drawRowMenu(const RowBounds& row, bool visible, bool menuActive, RowOutcome& outcome)
{
    drawRowDots(row, visible, menuActive, outcome);
    if (menuActive) {
        drawRowTrash(row, outcome);
    }
}

RowOutcome drawProfileRow(const rgbpicker::Profile& profile, RowLook look)
{
    RowOutcome outcome;
    const ImVec2 rowPos{ImGui::GetCursorScreenPos()};
    const float rowHeight{look.height};
    const bool isSelected{look.selected};
    const bool menuActive{look.menuActive};
    const RowBounds row{rowPos,
                        ImVec2{rowPos.x + ImGui::GetContentRegionAvail().x, rowPos.y + rowHeight},
                        rowHeight};
    const ImVec2 rowEnd{row.end};
    ImDrawList* draw{ImGui::GetWindowDrawList()};

    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
    ImGui::SetNextItemAllowOverlap();
    outcome.applyProfile = ImGui::Selectable("##profile", isSelected,
                                             ImGuiSelectableFlags_AllowOverlap,
                                             ImVec2{0.0f, rowHeight});
    ImGui::PopStyleColor();
    const ImVec2 afterRow{ImGui::GetCursorScreenPos()};

    const bool hovered{ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(rowPos, rowEnd)};
    if (!isSelected && (hovered || menuActive)) {
        draw->AddRectFilled(rowPos, rowEnd, ImGui::ColorConvertFloat4ToU32(kRowHover));
    }
    if (isSelected) {
        draw->AddRect(rowPos, rowEnd, ImGui::ColorConvertFloat4ToU32(kFieldBorder), 2.0f, 0, 1.0f);
        draw->AddRectFilled(rowPos, ImVec2{rowPos.x + 3.0f, rowEnd.y},
                            ImGui::ColorConvertFloat4ToU32(kAccent));
    }

    draw->PushClipRect(rowPos, ImVec2{rowEnd.x - 28.0f, rowEnd.y}, true);
    draw->AddText(ImVec2{rowPos.x + rowGutter,
                         rowPos.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f},
                  ImGui::ColorConvertFloat4ToU32(isSelected ? kText : kTextSoft),
                  profile.name.c_str());
    draw->PopClipRect();

    drawRowMenu(row, hovered || isSelected, menuActive, outcome);

    ImGui::SetCursorScreenPos(afterRow);
    return outcome;
}

struct SaveTarget {
    std::string_view driftedProfile;
    bool anythingToSave{false};
};

void pushSaveColors(bool ready)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ready ? kReady : kField);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ready ? kReadyHover : kField);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ready ? kReadyHover : kField);
    ImGui::PushStyleColor(ImGuiCol_Text, ready ? kText : kTextMuted);
}

std::optional<std::string> drawSaveProfile(float width, const SaveTarget& target)
{
    static std::array<char, 64> nameEntry{};
    constexpr float actionHeight{40.0f};

    ImGui::SetNextItemWidth(width);
    const bool submitted{ImGui::InputTextWithHint("##profile-name", "Profile name",
                                                  nameEntry.data(), nameEntry.size(),
                                                  ImGuiInputTextFlags_EnterReturnsTrue)};
    const std::string typed{nameEntry.data()};
    const bool updating{typed.empty() && !target.driftedProfile.empty()};
    const std::string name{updating ? std::string{target.driftedProfile} : typed};
    const bool canSave{!name.empty() && target.anythingToSave};

    pushSaveColors(canSave);
    const bool pressed{ImGui::Button("Save profile", ImVec2{width, actionHeight})};
    ImGui::PopStyleColor(4);

    const bool asked{pressed || (submitted && !typed.empty())};
    if (!canSave || !asked) {
        return std::nullopt;
    }
    nameEntry.fill('\0');
    return name;
}

struct RailState {
    int pendingDelete{-1};
    int menuRow{-1};
};

struct ListOutcome {
    bool confirmDelete{false};
    bool claimedClick{false};
};

ListOutcome drawProfileList(Worker& worker, float rowHeight, RailState& rail)
{
    ListOutcome outcome;
    for (std::size_t index{0}; index < g_profiles.size(); ++index) {
        const int row{static_cast<int>(index)};
        ImGui::PushID(row);
        const bool isActive{g_profiles[index].name == g_settings.activeProfile &&
                            !g_settings.activeProfile.empty()};
        const RowOutcome result{drawProfileRow(
            g_profiles[index], RowLook{rowHeight, isActive, rail.menuRow == row})};
        ImGui::PopID();

        outcome.claimedClick = outcome.claimedClick || result.claimedClick;
        if (result.applyProfile) {
            setActiveProfile(g_profiles[index].name);
            worker.postProfile(g_profiles[index].devices);
        }
        if (result.toggleMenu) {
            rail.menuRow = rail.menuRow == row ? -1 : row;
        }
        if (result.deleteRequested) {
            rail.pendingDelete = row;
            rail.menuRow = -1;
            outcome.confirmDelete = true;
        }
    }
    if (g_profiles.empty()) {
        ImGui::Indent(rowGutter);
        ImGui::TextDisabled("No profiles saved");
        ImGui::Unindent(rowGutter);
    }
    return outcome;
}

}

void drawProfileRail(Worker& worker, const std::vector<rgbpicker::DeviceColor>& applied,
                     float height)
{
    constexpr float railWidth{270.0f};
    constexpr float rowHeight{40.0f};
    constexpr float footerHeight{124.0f};
    static RailState rail;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, kPanel);
    ImGui::BeginChild("rail", ImVec2{railWidth, height}, ImGuiChildFlags_None);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
    ImGui::BeginChild("rail-inner", ImVec2{0.0f, 0.0f}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollWithMouse);

    drawRailHeader("PROFILES");

    const float listHeight{std::max(rowHeight, ImGui::GetContentRegionAvail().y - footerHeight)};
    ImGui::BeginChild("profile-list", ImVec2{0.0f, listHeight}, ImGuiChildFlags_None);
    const ListOutcome list{drawProfileList(worker, rowHeight, rail)};
    ImGui::EndChild();

    ImGui::PushStyleColor(ImGuiCol_Separator, kHairline);
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2{0.0f, 10.0f});

    ImGui::Indent(rowGutter);
    const rgbpicker::Profile* const active{activeProfile()};
    const bool drifted{active != nullptr && !rgbpicker::sameLook(active->devices, applied)};
    const SaveTarget target{.driftedProfile = drifted ? std::string_view{active->name}
                                                      : std::string_view{},
                            .anythingToSave = !applied.empty()};
    if (const auto name{drawSaveProfile(railWidth - rowGutter * 2.0f, target)}) {
        rgbpicker::storeProfile(g_profiles, rgbpicker::Profile{*name, applied});
        saveProfiles();
        setActiveProfile(*name);
    }
    ImGui::Unindent(rowGutter);

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (!list.claimedClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        rail.menuRow = -1;
    }

    if (list.confirmDelete) {
        ImGui::OpenPopup("Delete profile");
    }
    drawDeleteConfirm(rail.pendingDelete);
}
}
