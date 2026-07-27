#include "rgbpicker/backend.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace rgbpicker {
namespace {

constexpr std::string_view unknownTypeLabel{"Device"};

struct OpenRgbDeviceType {
    DeviceType type;
    std::string_view label;
};

constexpr std::array openRgbDeviceTypes{
    OpenRgbDeviceType{DeviceType::motherboard, "Motherboard"},
    OpenRgbDeviceType{DeviceType::memory, "Memory"},
    OpenRgbDeviceType{DeviceType::gpu, "Graphics"},
    OpenRgbDeviceType{DeviceType::cooler, "Cooler"},
    OpenRgbDeviceType{DeviceType::ledStrip, "LED strip"},
    OpenRgbDeviceType{DeviceType::keyboard, "Keyboard"},
    OpenRgbDeviceType{DeviceType::mouse, "Mouse"},
    OpenRgbDeviceType{DeviceType::mouseMat, "Mouse mat"},
    OpenRgbDeviceType{DeviceType::headset, "Headset"},
    OpenRgbDeviceType{DeviceType::headsetStand, "Headset stand"},
    OpenRgbDeviceType{DeviceType::gamepad, "Gamepad"},
    OpenRgbDeviceType{DeviceType::light, "Light"},
    OpenRgbDeviceType{DeviceType::speaker, "Speaker"},
    OpenRgbDeviceType{DeviceType::virtualDevice, "Virtual"},
    OpenRgbDeviceType{DeviceType::storage, "Storage"},
    OpenRgbDeviceType{DeviceType::pcCase, "Case"},
    OpenRgbDeviceType{DeviceType::microphone, "Microphone"},
    OpenRgbDeviceType{DeviceType::accessory, "Accessory"},
    OpenRgbDeviceType{DeviceType::keypad, "Keypad"},
};

bool sameNameIgnoringCase(std::string_view left, std::string_view right)
{
    return std::ranges::equal(left, right, [](unsigned char leftCharacter,
                                              unsigned char rightCharacter) {
        return std::tolower(leftCharacter) == std::tolower(rightCharacter);
    });
}

constexpr std::array<std::string_view, 3> preferredModes{"Direct", "Custom", "Static"};

const Mode* findMode(const std::vector<Mode>& modes, std::string_view name)
{
    const auto found{std::ranges::find_if(modes, [name](const Mode& mode) {
        return mode.perLedColor && sameNameIgnoringCase(mode.name, name);
    })};
    return found == modes.end() ? nullptr : &*found;
}

}

DeviceType deviceTypeFromOpenRgbValue(std::uint32_t value)
{
    if (value >= openRgbDeviceTypes.size()) {
        return DeviceType::other;
    }
    return openRgbDeviceTypes[value].type;
}

std::string_view deviceTypeLabel(DeviceType type)
{
    const auto matchingType{
        std::ranges::find(openRgbDeviceTypes, type, &OpenRgbDeviceType::type)};
    return matchingType == openRgbDeviceTypes.end() ? unknownTypeLabel : matchingType->label;
}

std::optional<std::string> chooseDirectMode(const std::vector<Mode>& modes,
                                           std::string_view activeMode)
{
    const auto active{std::ranges::find(modes, activeMode, &Mode::name)};
    if (active != modes.end() && active->perLedColor) {
        return std::nullopt;
    }
    for (const std::string_view preferred : preferredModes) {
        if (const Mode* const mode{findMode(modes, preferred)}) {
            return mode->name;
        }
    }
    const auto usable{std::ranges::find(modes, true, &Mode::perLedColor)};
    if (usable == modes.end()) {
        return std::nullopt;
    }
    return usable->name;
}

}
