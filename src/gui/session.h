#pragma once

#include "gui/worker.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace rgbpicker::gui {

struct DeviceScratch {
    std::uint64_t revision{std::numeric_limits<std::uint64_t>::max()};
    std::vector<int> zoneSizes;
    bool rainbow{false};
};

inline constexpr std::uint32_t allDevicesId{std::numeric_limits<std::uint32_t>::max()};

struct PickerTarget {
    std::uint32_t deviceId{allDevicesId};
    std::optional<std::size_t> zoneIndex;
};

struct QuickColor {
    const char* label;
    Color color;
};

inline constexpr std::array presetColors{
    QuickColor{"White", Color{255, 255, 255}},  QuickColor{"Warm", Color{255, 190, 120}},
    QuickColor{"Red", Color{255, 0, 0}},        QuickColor{"Orange", Color{255, 120, 0}},
    QuickColor{"Yellow", Color{255, 210, 0}},   QuickColor{"Green", Color{0, 255, 60}},
    QuickColor{"Cyan", Color{0, 200, 255}},     QuickColor{"Blue", Color{0, 70, 255}},
    QuickColor{"Purple", Color{140, 0, 255}},   QuickColor{"Pink", Color{255, 60, 170}},
    QuickColor{"Off", Color{0, 0, 0}},
};

void rememberLastColor(Color color);

void applyToTarget(Worker& worker, const PickerTarget& target, const std::vector<Device>& devices,
                   Color color);

}
