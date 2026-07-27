#include "backend/simulation/simulation_backend.h"

#include <algorithm>
#include <string>

namespace rgbpicker {
namespace {

std::vector<Zone> makeZones(int size, int maxSize, ZoneUnit unit)
{
    const Color off{0, 0, 0};
    return {
        Zone{0, "Channel 1", size, 0, maxSize, unit, off},
        Zone{1, "Channel 2", size, 0, maxSize, unit, off},
        Zone{2, "Channel 3", size, 0, maxSize, unit, off},
        Zone{3, "Channel 4", size, 0, maxSize, unit, off},
    };
}

std::vector<Device> makeDevices()
{
    const Color off{0, 0, 0};
    return {
        Device{.id = 0,
               .name = "Lian Li Uni Hub - SL V2 v0.5",
               .type = DeviceType::cooler,
               .zones = makeZones(64, 96, ZoneUnit::leds),
               .modes = {Mode{"Static", true}, Mode{"Rainbow Wave", false}},
               .activeMode = "Static",
               .color = off},
        Device{.id = 1,
               .name = "Lian Li Uni Hub - SL",
               .type = DeviceType::cooler,
               .zones = makeZones(4, 4, ZoneUnit::fans),
               .modes = {Mode{"Static", true}, Mode{"Rainbow Wave", false}},
               .activeMode = "Static",
               .color = off},
        Device{.id = 2,
               .name = "Wireless Gaming Keyboard",
               .type = DeviceType::keyboard,
               .zones = {Zone{0, "Keyboard", 104, 104, 104, ZoneUnit::leds, off}},
               .modes = {Mode{"Spectrum Cycle", false}, Mode{"Direct", true},
                         Mode{"Static", true}},
               .activeMode = "Spectrum Cycle",
               .color = off},
        Device{.id = 3,
               .name = "ASUS ROG STRIX PG32UCDM",
               .type = DeviceType::other,
               .zones = {Zone{0, "Backlight", 12, 12, 12, ZoneUnit::leds, off}},
               .modes = {Mode{"Breathing", false}, Mode{"Static", true}},
               .activeMode = "Breathing",
               .color = off},
    };
}

}

SimulationBackend::SimulationBackend() : m_devices{makeDevices()} {}

std::expected<std::vector<Device>, BackendError> SimulationBackend::discover()
{
    return m_devices;
}

std::expected<Device, BackendError> SimulationBackend::changeDeviceColor(std::uint32_t deviceId,
                                                                          Color color)
{
    Device* const device{findDevice(deviceId)};
    if (device == nullptr) {
        return std::unexpected{BackendError::notFound};
    }

    device->color = color;
    for (auto& zone : device->zones) {
        zone.color = color;
    }
    return *device;
}

std::expected<Device, BackendError> SimulationBackend::changeZoneColor(std::uint32_t deviceId,
                                                                        std::uint32_t zoneId,
                                                                        Color color)
{
    Device* const device{findDevice(deviceId)};
    if (device == nullptr) {
        return std::unexpected{BackendError::notFound};
    }
    Zone* const zone{findZone(*device, zoneId)};
    if (zone == nullptr) {
        return std::unexpected{BackendError::notFound};
    }

    zone->color = color;
    return *device;
}

std::expected<Device, BackendError> SimulationBackend::resizeZone(std::uint32_t deviceId,
                                                                   std::uint32_t zoneId, int size)
{
    Device* const device{findDevice(deviceId)};
    if (device == nullptr) {
        return std::unexpected{BackendError::notFound};
    }
    Zone* const zone{findZone(*device, zoneId)};
    if (zone == nullptr) {
        return std::unexpected{BackendError::notFound};
    }
    if (size < zone->minSize || size > zone->maxSize) {
        return std::unexpected{BackendError::invalidArgument};
    }

    zone->size = size;
    return *device;
}

std::expected<Device, BackendError> SimulationBackend::changeMode(std::uint32_t deviceId,
                                                                   std::string_view mode)
{
    Device* const device{findDevice(deviceId)};
    if (device == nullptr) {
        return std::unexpected{BackendError::notFound};
    }
    const auto selected{std::ranges::find(device->modes, mode, &Mode::name)};
    if (selected == device->modes.end()) {
        return std::unexpected{BackendError::notFound};
    }

    device->activeMode = selected->name;
    return *device;
}

Device* SimulationBackend::findDevice(std::uint32_t deviceId)
{
    const auto device{std::ranges::find(m_devices, deviceId, &Device::id)};
    return device == m_devices.end() ? nullptr : &*device;
}

Zone* SimulationBackend::findZone(Device& device, std::uint32_t zoneId)
{
    const auto zone{std::ranges::find(device.zones, zoneId, &Zone::id)};
    return zone == device.zones.end() ? nullptr : &*zone;
}

}
