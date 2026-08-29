#include "gui/devices/properties.h"

#include "gui/app/app_state.h"
#include "gui/ui/theme.h"
#include "gui/ui/widgets.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace rgbpicker::gui {

namespace {



}

void drawBrightnessSection(Worker& worker)
{
    static double lastApply{0.0};

    rowLabel("Level", "Scales every color on its way to the hardware. Profiles store the "
                      "color you picked, so this dims what is lit without rewriting them.");
    ImGui::SetNextItemWidth(rowControlWidth);
    const bool moved{ImGui::SliderInt("##brightness", &g_settings.brightness, 0, 100, "%d%%")};

    const double now{ImGui::GetTime()};
    if (moved && now - lastApply > 0.1 && worker.pending() == 0) {
        lastApply = now;
        worker.postBrightness(g_settings.brightness);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        worker.postBrightness(g_settings.brightness);
        saveSettings();
    }
}

void drawEffectsSection(const Device& device, DeviceScratch& scratch, Worker& worker)
{
    rowLabel("Rainbow cycle", "Cycles the hue on this device until it is switched off again.");
    const bool wasOn{scratch.rainbow};
    segmentedToggle("rainbow", scratch.rainbow);
    if (wasOn && !scratch.rainbow) {
        worker.postDeviceColor(device.id, rgbpicker::Color{255, 255, 255});
    }
}

void drawZoneRow(const Device& device, const rgbpicker::Zone& zone, int& size, Worker& worker)
{
    const char* const unit{zone.unit == ZoneUnit::fans ? "fans" : "LEDs"};
    rowLabel(zone.name.c_str());
    if (zone.minSize == zone.maxSize) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%d %s", zone.size, unit);
        return;
    }

    bool edited{false};
    constexpr float stepWidth{32.0f};
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4.0f, 9.0f});
    if (ImGui::Button("-", ImVec2{stepWidth, 0.0f})) {
        size = std::clamp(size - 1, zone.minSize, zone.maxSize);
        edited = true;
    }
    ImGui::SameLine(0.0f, 6.0f);
    ImGui::SetNextItemWidth(64.0f);
    ImGui::DragInt("##size", &size, 0.2f, zone.minSize, zone.maxSize, "%d",
                   ImGuiSliderFlags_AlwaysClamp);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        size = std::clamp(size, zone.minSize, zone.maxSize);
        edited = true;
    }
    ImGui::SameLine(0.0f, 6.0f);
    if (ImGui::Button("+", ImVec2{stepWidth, 0.0f})) {
        size = std::clamp(size + 1, zone.minSize, zone.maxSize);
        edited = true;
    }
    ImGui::PopStyleVar();
    ImGui::SameLine(0.0f, 10.0f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s", unit);

    if (edited && size != zone.size) {
        worker.postResize(device.id, zone.id, size);
    }
}

void drawZonesSection(const Device& device, DeviceScratch& scratch, Worker& worker)
{
    for (std::size_t index{0}; index < device.zones.size(); ++index) {
        ImGui::PushID(static_cast<int>(device.zones[index].id));
        drawZoneRow(device, device.zones[index], scratch.zoneSizes[index], worker);
        ImGui::PopID();
        if (index + 1 < device.zones.size()) {
            dottedRule();
        }
    }
}

void drawModeSection(const Device& device, Worker& worker)
{
    rowLabel("Mode", "Effects that run on the hub itself. A hardware effect drives its own "
                     "colors, so it overrides the picker until it is set back.");
    ImGui::SetNextItemWidth(rowControlWidth);
    const char* const active{device.activeMode.empty() ? "Select..." : device.activeMode.c_str()};
    if (!ImGui::BeginCombo("##mode", active)) {
        return;
    }
    for (const rgbpicker::Mode& mode : device.modes) {
        const bool selected{mode.name == device.activeMode};
        if (ImGui::Selectable(mode.name.c_str(), selected) && !selected) {
            worker.postMode(device.id, mode.name);
        }
        if (mode.perLedColor) {
            ImGui::SameLine();
            ImGui::TextDisabled("takes colors");
        }
    }
    ImGui::EndCombo();
}

void drawProperties(const Device* device, DeviceScratch* scratch, Worker& worker, float height)
{
    static bool brightnessOpen{true};
    static bool effectsOpen{true};
    static bool zonesOpen{true};
    static bool modeOpen{true};
    constexpr float sectionTail{8.0f};

    ImGui::PushStyleColor(ImGuiCol_ChildBg, kPanel);
    ImGui::BeginChild("properties", ImVec2{propertiesWidth, height}, ImGuiChildFlags_None);

    if (sectionBand("BRIGHTNESS", brightnessOpen)) {
        drawBrightnessSection(worker);
        ImGui::Dummy(ImVec2{0.0f, sectionTail});
    }

    if (device != nullptr && scratch != nullptr) {
        if (sectionBand("EFFECTS", effectsOpen)) {
            drawEffectsSection(*device, *scratch, worker);
            ImGui::Dummy(ImVec2{0.0f, sectionTail});
        }
        if (sectionBand("ZONES", zonesOpen)) {
            drawZonesSection(*device, *scratch, worker);
            ImGui::Dummy(ImVec2{0.0f, sectionTail});
        }
        if (!device->modes.empty() && sectionBand("HARDWARE MODE", modeOpen)) {
            drawModeSection(*device, worker);
            ImGui::Dummy(ImVec2{0.0f, sectionTail});
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}
}
