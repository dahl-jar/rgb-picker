#include "gui/workspace.h"

#include "gui/theme.h"
#include "gui/widgets.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <string>

namespace rgbpicker::gui {

namespace {

struct ColorTools {
    Worker& worker;
    const PickerTarget& target;
    const std::vector<Device>& devices;
    float (&rgb)[3];
    float panelWidth{};
};

void drawDeviceSelector(const std::vector<Device>& devices, const Device* device,
                        PickerTarget& target)
{
    ImGui::PushStyleColor(ImGuiCol_Text, kTextMuted);
    ImGui::TextUnformatted("DEVICE");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2{0.0f, 6.0f});
    ImGui::SetNextItemWidth(320.0f);

    if (!ImGui::BeginCombo("##device", device != nullptr ? device->name.c_str() : "All devices")) {
        return;
    }
    if (ImGui::Selectable("All devices", device == nullptr)) {
        target = PickerTarget{allDevicesId, std::nullopt};
    }
    for (const Device& candidate : devices) {
        if (ImGui::Selectable(candidate.name.c_str(),
                              device != nullptr && device->id == candidate.id)) {
            target = PickerTarget{candidate.id, std::nullopt};
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", std::string{rgbpicker::deviceTypeLabel(candidate.type)}.c_str());
    }
    ImGui::EndCombo();
}

void drawZoneSelector(const Device& device, PickerTarget& target)
{
    if (target.zoneIndex.has_value() && *target.zoneIndex >= device.zones.size()) {
        target.zoneIndex.reset();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, kTextMuted);
    ImGui::TextUnformatted("APPLY TO");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2{0.0f, 6.0f});
    ImGui::SetNextItemWidth(320.0f);

    const char* const current{target.zoneIndex.has_value()
                                  ? device.zones[*target.zoneIndex].name.c_str()
                                  : "All zones"};
    if (!ImGui::BeginCombo("##target", current)) {
        return;
    }
    if (ImGui::Selectable("All zones", !target.zoneIndex.has_value())) {
        target.zoneIndex.reset();
    }
    for (std::size_t index{0}; index < device.zones.size(); ++index) {
        if (ImGui::Selectable(device.zones[index].name.c_str(),
                              target.zoneIndex == index)) {
            target.zoneIndex = index;
        }
    }
    ImGui::EndCombo();
}

void drawPicker(const ColorTools& tools)
{
    const float panelWidth{tools.panelWidth};
    float(&pickerRgb)[3] = tools.rgb;
    Worker& worker{tools.worker};
    const PickerTarget& target{tools.target};
    const std::vector<Device>& devices{tools.devices};
    static bool dirty{false};
    static double lastApply{0.0};

    const float pickerWidth{std::clamp(panelWidth - 72.0f, 260.0f, 430.0f)};
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                         std::max(0.0f, (panelWidth - pickerWidth) * 0.5f));
    ImGui::SetNextItemWidth(pickerWidth);
    if (ImGui::ColorPicker3("##picker", pickerRgb,
                            ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoSidePreview |
                                ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoAlpha |
                                ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV)) {
        dirty = true;
    }

    const double now{ImGui::GetTime()};
    const bool released{ImGui::IsItemDeactivatedAfterEdit()};
    if ((dirty && now - lastApply > 0.15 && worker.pending() == 0) || released) {
        dirty = false;
        lastApply = now;
        applyToTarget(worker, target, devices, colorFromFloats(pickerRgb));
        if (released) {
            rememberLastColor(colorFromFloats(pickerRgb));
        }
    }
}

void drawColorEntry(const ColorTools& tools)
{
    const float panelWidth{tools.panelWidth};
    float(&pickerRgb)[3] = tools.rgb;
    Worker& worker{tools.worker};
    const PickerTarget& target{tools.target};
    const std::vector<Device>& devices{tools.devices};
    static std::array<char, 32> entry{"#FFFFFF"};
    static bool editing{false};
    constexpr float applyWidth{80.0f};
    constexpr float entryWidth{212.0f};

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                         std::max(0.0f, (panelWidth - entryWidth - applyWidth - 8.0f) * 0.5f));
    if (!editing) {
        const Color shown{colorFromFloats(pickerRgb)};
        const std::string text{
            std::format("#{:02X}{:02X}{:02X}", shown.red, shown.green, shown.blue)};
        entry.fill('\0');
        std::copy_n(text.begin(), std::min(text.size(), entry.size() - 1), entry.begin());
    }

    const bool valid{rgbpicker::parseColor(entry.data()).has_value()};
    if (!valid) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{0.35f, 0.16f, 0.12f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_Text, kError);
    }
    ImGui::SetNextItemWidth(entryWidth);
    const bool submitted{ImGui::InputTextWithHint("##hex", "#20a0f0 or purple", entry.data(),
                                                  entry.size(),
                                                  ImGuiInputTextFlags_EnterReturnsTrue)};
    editing = ImGui::IsItemActive();
    if (!valid) {
        ImGui::PopStyleColor(2);
    }
    ImGui::SameLine(0.0f, 8.0f);
    const bool apply{ImGui::Button("Apply", ImVec2{applyWidth, 0.0f})};

    const auto parsed{rgbpicker::parseColor(entry.data())};
    if (!parsed.has_value() || (!apply && !submitted)) {
        return;
    }
    pickerRgb[0] = static_cast<float>(parsed->red) / 255.0f;
    pickerRgb[1] = static_cast<float>(parsed->green) / 255.0f;
    pickerRgb[2] = static_cast<float>(parsed->blue) / 255.0f;
    applyToTarget(worker, target, devices, *parsed);
    rememberLastColor(*parsed);
}

void drawPresets(const ColorTools& tools)
{
    const float panelWidth{tools.panelWidth};
    float(&pickerRgb)[3] = tools.rgb;
    Worker& worker{tools.worker};
    const PickerTarget& target{tools.target};
    const std::vector<Device>& devices{tools.devices};
    constexpr float swatchWidth{26.0f};
    constexpr float swatchGap{6.0f};
    const float presetsWidth{
        static_cast<float>(presetColors.size()) * (swatchWidth + swatchGap) - swatchGap};
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                         std::max(0.0f, (panelWidth - presetsWidth) * 0.5f));

    for (std::size_t index{0}; index < presetColors.size(); ++index) {
        if (index != 0) {
            ImGui::SameLine(0.0f, swatchGap);
        }
        const QuickColor& preset{presetColors[index]};
        ImGui::PushID(preset.label);
        if (ImGui::ColorButton("##preset", toImVec4(preset.color),
                               ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoAlpha,
                               ImVec2{swatchWidth, 26.0f})) {
            pickerRgb[0] = static_cast<float>(preset.color.red) / 255.0f;
            pickerRgb[1] = static_cast<float>(preset.color.green) / 255.0f;
            pickerRgb[2] = static_cast<float>(preset.color.blue) / 255.0f;
            applyToTarget(worker, target, devices, preset.color);
            rememberLastColor(preset.color);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", preset.label);
        }
        ImGui::PopID();
    }
}

}

void drawWorkspace(const std::vector<Device>& devices, const Device* device, PickerTarget& target,
                   Worker& worker, ImVec2 size)
{
    const float width{size.x};
    const float height{size.y};
    static float pickerRgb[3]{1.0f, 1.0f, 1.0f};

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{26.0f, 22.0f});
    ImGui::BeginChild("workspace", ImVec2{width, height}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_AlwaysUseWindowPadding);

    drawDeviceSelector(devices, device, target);
    ImGui::Dummy(ImVec2{0.0f, 18.0f});

    if (device != nullptr && device->zones.size() > 1) {
        drawZoneSelector(*device, target);
        ImGui::Dummy(ImVec2{0.0f, 18.0f});
    }

    const ColorTools tools{worker, target, devices, pickerRgb,
                           ImGui::GetContentRegionAvail().x};
    drawPicker(tools);
    ImGui::Dummy(ImVec2{0.0f, 16.0f});
    drawColorEntry(tools);
    ImGui::Dummy(ImVec2{0.0f, 12.0f});
    drawPresets(tools);

    if (devices.empty()) {
        ImGui::Dummy(ImVec2{0.0f, 18.0f});
        ImGui::TextDisabled("No devices yet");
        ImGui::SameLine(0.0f, 10.0f);
        helpMarker("Supported hardware appears here after its USB or I2C interface is detected. "
                   "The light at the foot of the window shows backend availability.");
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
}
}
