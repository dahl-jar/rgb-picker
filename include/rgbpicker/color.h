#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace rgbpicker {

struct Color {
    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};

    bool operator==(const Color&) const = default;
};

std::optional<Color> parseColor(std::string_view value);

Color scaleBrightness(Color color, int percent);

}
