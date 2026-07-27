#include "gui/shell.h"

#include "gui/app_state.h"
#include "gui/properties.h"
#include "gui/rail.h"
#include "gui/session.h"
#include "gui/settings_dialog.h"
#include "gui/theme.h"
#include "gui/widgets.h"
#include "gui/workspace.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace rgbpicker::gui {

namespace {

void drawStatusBar(bool backendReady, const std::string& status, std::size_t deviceCount)
{
    constexpr float height{30.0f};
    constexpr float lightRadius{4.5f};
    const ImVec2 windowPos{ImGui::GetWindowPos()};
    const ImVec2 windowSize{ImGui::GetWindowSize()};
    const float width{windowSize.x};
    const ImVec2 origin{windowPos.x, windowPos.y + windowSize.y - height};

    ImDrawList* draw{ImGui::GetWindowDrawList()};
    draw->AddRectFilled(origin, ImVec2{origin.x + width, origin.y + height},
                        ImGui::ColorConvertFloat4ToU32(kPanel));
    draw->AddLine(origin, ImVec2{origin.x + width, origin.y},
                  ImGui::ColorConvertFloat4ToU32(kHairline));

    const float centreY{origin.y + height * 0.5f};
    draw->AddCircleFilled(ImVec2{origin.x + rowGutter + lightRadius, centreY}, lightRadius,
                          ImGui::ColorConvertFloat4ToU32(backendReady ? kOnline : kError));

    const float textY{centreY - ImGui::GetTextLineHeight() * 0.5f};
    const ImU32 ink{ImGui::ColorConvertFloat4ToU32(kTextSoft)};
    const float statusX{origin.x + rowGutter + lightRadius * 2.0f + 10.0f};
    draw->AddText(ImVec2{statusX, textY}, ink, backendReady ? "Ready" : status.c_str());

    const std::string devices{std::format("{} device{}", deviceCount, deviceCount == 1 ? "" : "s")};
    draw->AddText(ImVec2{origin.x + width - rowGutter - ImGui::CalcTextSize(devices.c_str()).x,
                         textY},
                  ImGui::ColorConvertFloat4ToU32(kTextMuted), devices.c_str());
}

void drawTopBar(bool& settingsRequested)
{
    constexpr float height{52.0f};
    constexpr float iconSize{28.0f};
    const float width{ImGui::GetContentRegionAvail().x};
    const ImVec2 origin{ImGui::GetCursorScreenPos()};

    ImDrawList* draw{ImGui::GetWindowDrawList()};
    draw->AddRectFilled(origin, ImVec2{origin.x + width, origin.y + height},
                        ImGui::ColorConvertFloat4ToU32(kPanel));
    draw->AddLine(ImVec2{origin.x, origin.y + height}, ImVec2{origin.x + width, origin.y + height},
                  ImGui::ColorConvertFloat4ToU32(kHairline));

    ImGui::SetCursorScreenPos(ImVec2{origin.x + rowGutter + 4.0f,
                                     origin.y + (height - ImGui::GetTextLineHeight()) * 0.5f});
    ImGui::PushStyleColor(ImGuiCol_Text, kText);
    ImGui::TextUnformatted("RGB PICKER");
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2{origin.x + width - rowGutter - iconSize,
                                     origin.y + (height - iconSize) * 0.5f});
    if (settingsButton("settings", iconSize)) {
        settingsRequested = true;
    }

    ImGui::SetCursorScreenPos(ImVec2{origin.x, origin.y + height});
}

struct FrameState {
    std::vector<Device> devices;
    std::string status;
    std::vector<rgbpicker::DeviceColor> applied;
    std::uint64_t revision{};
    bool busy{false};
    bool backendReady{false};
};

FrameState takeFrameState(UiState& state)
{
    FrameState frame;
    {
        const std::lock_guard lock{state.mutex};
        frame.devices = state.devices;
        frame.status = state.status;
        frame.applied = state.applied;
        frame.revision = state.revision;
        frame.busy = state.pendingJobs > 0;
        frame.backendReady = state.backendReady;
    }
    std::ranges::sort(frame.devices, {}, &Device::name);
    return frame;
}

using ScratchByDevice = std::unordered_map<std::uint32_t, DeviceScratch>;

void stepRainbow(const FrameState& frame, ScratchByDevice& scratchByDevice, Worker& worker)
{
    static double lastStep{0.0};
    static float hue{0.0f};

    const double now{ImGui::GetTime()};
    if (frame.busy || now - lastStep <= 0.12) {
        return;
    }
    lastStep = now;
    hue = std::fmod(hue + 4.0f, 360.0f);
    for (const Device& device : frame.devices) {
        const auto scratch{scratchByDevice.find(device.id)};
        if (scratch != scratchByDevice.end() && scratch->second.rainbow) {
            worker.postDeviceColor(device.id, colorFromHue(hue));
        }
    }
}

const Device* resolveSelection(const std::vector<Device>& devices, PickerTarget& target)
{
    if (target.deviceId == allDevicesId) {
        return nullptr;
    }
    const auto found{std::ranges::find(devices, target.deviceId, &Device::id)};
    if (found == devices.end()) {
        target = PickerTarget{allDevicesId, std::nullopt};
        return nullptr;
    }
    return &*found;
}

DeviceScratch* scratchFor(const Device* device, std::uint64_t revision,
                          ScratchByDevice& scratchByDevice)
{
    if (device == nullptr) {
        return nullptr;
    }
    DeviceScratch& scratch{scratchByDevice[device->id]};
    if (scratch.revision != revision) {
        scratch.revision = revision;
        scratch.zoneSizes.resize(device->zones.size());
        for (std::size_t index{0}; index < device->zones.size(); ++index) {
            scratch.zoneSizes[index] = device->zones[index].size;
        }
    }
    return &scratch;
}

}


void drawUi(UiState& state, Worker& worker)
{
    static ScratchByDevice scratchByDevice;
    static PickerTarget target;
    constexpr float statusBarHeight{30.0f};

    const FrameState frame{takeFrameState(state)};
    stepRainbow(frame, scratchByDevice, worker);

    const ImGuiViewport* viewport{ImGui::GetMainViewport()};
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("##main", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const Device* const device{resolveSelection(frame.devices, target)};
    DeviceScratch* const scratch{scratchFor(device, frame.revision, scratchByDevice)};

    bool settingsRequested{false};
    drawTopBar(settingsRequested);

    const float bodyHeight{ImGui::GetContentRegionAvail().y - statusBarHeight};
    drawProfileRail(worker, frame.applied, bodyHeight);
    ImGui::SameLine(0.0f, 0.0f);
    const float workspaceWidth{ImGui::GetContentRegionAvail().x - propertiesWidth};
    drawWorkspace(frame.devices, device, target, worker, ImVec2{workspaceWidth, bodyHeight});
    ImGui::SameLine(0.0f, 0.0f);
    drawProperties(device, scratch, worker, bodyHeight);
    drawStatusBar(frame.backendReady, frame.status, frame.devices.size());

    if (settingsRequested) {
        ImGui::OpenPopup("Settings");
    }
    drawSettingsPopup(worker);

    ImGui::End();
}
}
