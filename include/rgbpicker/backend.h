#pragma once

#include "rgbpicker/color.h"

#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rgbpicker {

enum class ZoneUnit {
    leds,
    fans,
};

enum class DeviceType {
    motherboard,
    memory,
    gpu,
    cooler,
    ledStrip,
    keyboard,
    mouse,
    mouseMat,
    headset,
    headsetStand,
    gamepad,
    light,
    speaker,
    virtualDevice,
    storage,
    pcCase,
    microphone,
    accessory,
    keypad,
    other,
};

DeviceType deviceTypeFromOpenRgbValue(std::uint32_t value);

std::string_view deviceTypeLabel(DeviceType type);

struct Mode {
    std::string name;
    bool perLedColor{false};

    bool operator==(const Mode&) const = default;
};

std::optional<std::string> chooseDirectMode(const std::vector<Mode>& modes,
                                            std::string_view activeMode);

struct Zone {
    std::uint32_t id{};
    std::string name;
    int size{};
    int minSize{};
    int maxSize{};
    ZoneUnit unit{ZoneUnit::leds};
    Color color{};
};

struct Device {
    std::uint32_t id{};
    std::string name;
    DeviceType type{DeviceType::other};
    std::vector<Zone> zones;
    std::vector<Mode> modes;
    std::string activeMode;
    Color color{};
};

enum class BackendError {
    notFound,
    invalidArgument,
    unavailable,
    operationFailed,
};

class DeviceReader {
public:
    virtual ~DeviceReader() = default;

    virtual std::expected<std::vector<Device>, BackendError> discover() = 0;
};

class Backend : public DeviceReader {
public:
    virtual std::expected<Device, BackendError> changeDeviceColor(std::uint32_t deviceId,
                                                                   Color color) = 0;
    virtual std::expected<Device, BackendError> changeZoneColor(std::uint32_t deviceId,
                                                                 std::uint32_t zoneId,
                                                                 Color color) = 0;
    virtual std::expected<Device, BackendError> resizeZone(std::uint32_t deviceId,
                                                            std::uint32_t zoneId, int size) = 0;
    virtual std::expected<Device, BackendError> changeMode(std::uint32_t deviceId,
                                                            std::string_view mode) = 0;
};

class BackendFactory {
public:
    virtual ~BackendFactory() = default;

    virtual std::expected<std::unique_ptr<Backend>, BackendError>
    createHardware() = 0;
    virtual std::unique_ptr<Backend> createSimulation() = 0;
};

}
