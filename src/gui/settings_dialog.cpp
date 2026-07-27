#include "gui/settings_dialog.h"

#include "gui/app_state.h"
#include "gui/theme.h"
#include "gui/widgets.h"

namespace rgbpicker::gui {

namespace {

bool settingRow(const char* label, const char* description, bool& value)
{
    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, kTextSoft);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 8.0f);
    helpMarker(description);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::GetFrameHeight());
    const bool toggled{ImGui::Checkbox("##toggle", &value)};
    ImGui::PopID();
    return toggled;
}

}

void drawSettingsPopup(Worker& worker)
{
    constexpr float dialogWidth{400.0f};
    constexpr float buttonWidth{96.0f};

    const ImVec2 centre{ImGui::GetMainViewport()->GetCenter()};
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{dialogWidth, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{20.0f, 18.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, kSectionBand);
    ImGui::PushStyleColor(ImGuiCol_Border, kFieldBorder);

    if (ImGui::BeginPopupModal("Settings", nullptr,
                               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoMove)) {
        ImGui::PushFont(g_titleFont);
        ImGui::PushStyleColor(ImGuiCol_Text, kText);
        ImGui::TextUnformatted("Settings");
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::Dummy(ImVec2{0.0f, 12.0f});

        if (settingRow("Start with Windows",
                       "Adds RGB Picker to the programs Windows starts at sign-in. It "
                       "connects and restores your lighting without being opened.",
                       g_settings.runAtLogin)) {
            if (!g_loginStartup->setEnabled(g_settings.runAtLogin)) {
                g_settings.runAtLogin = g_loginStartup->isEnabled();
            }
            saveSettings();
        }

        dottedRule();
        if (settingRow("Restore last color",
                       "Puts the color from the last session back on every device once the "
                       "hardware answers.",
                       g_settings.restoreColor)) {
            worker.postRestorePreference(g_settings.restoreColor);
            saveSettings();
        }

        ImGui::Dummy(ImVec2{0.0f, 16.0f});
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - buttonWidth);
        if (ImGui::Button("Close", ImVec2{buttonWidth, 32.0f})) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}
}
