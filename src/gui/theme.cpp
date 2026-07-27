#include "gui/theme.h"

#include <array>
#include <filesystem>
#include <string>
#include <system_error>

#include <windows.h>

namespace rgbpicker::gui {

namespace {

constexpr const char* uiFontFile{"fonts/Roboto.ttf"};
constexpr float bodyFontSize{18.0f};
constexpr float titleFontSize{23.0f};

std::filesystem::path assetPath(const char* relative)
{
    std::array<wchar_t, MAX_PATH> module{};
    const DWORD length{
        GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()))};
    const std::filesystem::path executable{std::wstring{module.data(), length}};
    return executable.parent_path() / relative;
}

}

void applyTheme(float scale)
{
    ImGuiStyle& style{ImGui::GetStyle()};
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowPadding = ImVec2{0.0f, 0.0f};
    style.FramePadding = ImVec2{14.0f, 9.0f};
    style.ItemSpacing = ImVec2{10.0f, 9.0f};
    style.ItemInnerSpacing = ImVec2{8.0f, 6.0f};
    style.ChildBorderSize = 0.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 1.0f;
    style.SelectableTextAlign = ImVec2{0.0f, 0.5f};
    style.ScaleAllSizes(scale);

    ImVec4* colors{style.Colors};
    colors[ImGuiCol_WindowBg] = kGround;
    colors[ImGuiCol_ChildBg] = ImVec4{0.0f, 0.0f, 0.0f, 0.0f};
    colors[ImGuiCol_PopupBg] = kPanel;
    colors[ImGuiCol_Text] = kText;
    colors[ImGuiCol_TextDisabled] = kTextMuted;
    colors[ImGuiCol_FrameBg] = kField;
    colors[ImGuiCol_FrameBgHovered] = kFieldHover;
    colors[ImGuiCol_FrameBgActive] = kFieldHover;
    colors[ImGuiCol_Button] = kField;
    colors[ImGuiCol_ButtonHovered] = kFieldHover;
    colors[ImGuiCol_ButtonActive] = kSelection;
    colors[ImGuiCol_CheckMark] = kAccent;
    colors[ImGuiCol_SliderGrab] = kAccent;
    colors[ImGuiCol_SliderGrabActive] = kAccent;
    colors[ImGuiCol_Header] = kSelection;
    colors[ImGuiCol_HeaderHovered] = kRowHover;
    colors[ImGuiCol_HeaderActive] = kSelection;
    colors[ImGuiCol_Separator] = kHairline;
    colors[ImGuiCol_Border] = kFieldBorder;
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4{0.0f, 0.0f, 0.0f, 0.55f};
    colors[ImGuiCol_BorderShadow] = ImVec4{0.0f, 0.0f, 0.0f, 0.0f};
    colors[ImGuiCol_TitleBg] = kGround;
    colors[ImGuiCol_TitleBgActive] = kGround;
    colors[ImGuiCol_ScrollbarBg] = ImVec4{0.0f, 0.0f, 0.0f, 0.0f};
    colors[ImGuiCol_ScrollbarGrab] = kField;
    colors[ImGuiCol_ScrollbarGrabHovered] = kFieldHover;
    colors[ImGuiCol_ScrollbarGrabActive] = kFieldHover;
}

void loadFonts(ImGuiIO& io, float dpiScale)
{
    const std::filesystem::path font{assetPath(uiFontFile)};
    std::error_code ignored;
    if (std::filesystem::exists(font, ignored)) {
        const std::string path{font.string()};
        const ImFontConfig config{};
        g_bodyFont = io.Fonts->AddFontFromFileTTF(path.c_str(), bodyFontSize * dpiScale, &config);
        g_titleFont = io.Fonts->AddFontFromFileTTF(path.c_str(), titleFontSize * dpiScale, &config);
    }
    if (g_bodyFont == nullptr) {
        g_bodyFont = io.Fonts->AddFontDefault();
    }
    if (g_titleFont == nullptr) {
        g_titleFont = g_bodyFont;
    }
}
}
