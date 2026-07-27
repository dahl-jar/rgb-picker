#pragma once

#include "rgbpicker/backend.h"

#include <expected>
#include <string_view>
#include <vector>

namespace rgbpicker {

class SimulationBackend final : public Backend {
public:
    SimulationBackend();

    std::expected<std::vector<Device>, BackendError> discover() override;
    std::expected<Device, BackendError> changeDeviceColor(std::uint32_t deviceId,
                                                           Color color) override;
    std::expected<Device, BackendError> changeZoneColor(std::uint32_t deviceId,
                                                         std::uint32_t zoneId,
                                                         Color color) override;
    std::expected<Device, BackendError> resizeZone(std::uint32_t deviceId,
                                                    std::uint32_t zoneId, int size) override;
    std::expected<Device, BackendError> changeMode(std::uint32_t deviceId,
                                                    std::string_view mode) override;

private:
    Device* findDevice(std::uint32_t deviceId);
    static Zone* findZone(Device& device, std::uint32_t zoneId);

    std::vector<Device> m_devices;
};

}
