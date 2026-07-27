#pragma once

#include "imgui.h"

namespace rgbpicker::gui {

inline ImFont* g_bodyFont{nullptr};
inline ImFont* g_titleFont{nullptr};

constexpr ImVec4 kGround{0.051f, 0.051f, 0.051f, 1.0f};
constexpr ImVec4 kPanel{0.102f, 0.102f, 0.102f, 1.0f};
constexpr ImVec4 kSectionBand{0.078f, 0.078f, 0.078f, 1.0f};
constexpr ImVec4 kBandHover{0.118f, 0.118f, 0.118f, 1.0f};
constexpr ImVec4 kRowHover{0.129f, 0.129f, 0.129f, 1.0f};
constexpr ImVec4 kSelection{0.153f, 0.153f, 0.153f, 1.0f};
constexpr ImVec4 kField{0.125f, 0.125f, 0.125f, 1.0f};
constexpr ImVec4 kFieldHover{0.165f, 0.165f, 0.165f, 1.0f};
constexpr ImVec4 kFieldBorder{0.220f, 0.220f, 0.220f, 1.0f};
constexpr ImVec4 kText{0.910f, 0.910f, 0.910f, 1.0f};
constexpr ImVec4 kTextSoft{0.784f, 0.784f, 0.784f, 1.0f};
constexpr ImVec4 kTextMuted{0.478f, 0.478f, 0.478f, 1.0f};
constexpr ImVec4 kAccent{0.910f, 0.910f, 0.910f, 1.0f};
constexpr ImVec4 kAccentHover{1.0f, 1.0f, 1.0f, 1.0f};
constexpr ImVec4 kError{1.0f, 0.596f, 0.0f, 1.0f};
constexpr ImVec4 kDestructive{0.937f, 0.325f, 0.314f, 1.0f};
constexpr ImVec4 kDangerFill{0.741f, 0.180f, 0.157f, 1.0f};
constexpr ImVec4 kDangerHover{0.827f, 0.220f, 0.192f, 1.0f};
constexpr ImVec4 kOnline{0.298f, 0.765f, 0.451f, 1.0f};
constexpr ImVec4 kReady{0.180f, 0.627f, 0.263f, 1.0f};
constexpr ImVec4 kReadyHover{0.247f, 0.725f, 0.314f, 1.0f};
constexpr ImVec4 kHairline{0.165f, 0.165f, 0.165f, 1.0f};

constexpr float propertiesWidth{400.0f};
constexpr float rowControlWidth{200.0f};
constexpr float rowGutter{18.0f};
constexpr float bandHeight{40.0f};

void applyTheme(float scale);

void loadFonts(ImGuiIO& io, float dpiScale);

}
