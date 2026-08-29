#include "gui/app/session.h"

#include "gui/app/app_state.h"

#include <algorithm>

namespace rgbpicker::gui {

void rememberLastColor(Color color)
{
    if (g_settings.lastColor == color) {
        return;
    }
    g_settings.lastColor = color;
    saveSettings();
}

void applyToTarget(Worker& worker, const PickerTarget& target, const std::vector<Device>& devices,
                   Color color)
{
    if (target.deviceId == allDevicesId) {
        worker.postAllDevicesColor(color);
        return;
    }
    const auto device{std::ranges::find(devices, target.deviceId, &Device::id)};
    if (device == devices.end()) {
        return;
    }
    if (target.zoneIndex.has_value() && *target.zoneIndex < device->zones.size()) {
        worker.postZoneColor(device->id, device->zones[*target.zoneIndex].id, color);
    } else {
        worker.postDeviceColor(device->id, color);
    }
}
}
