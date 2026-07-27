#include "rgbpicker/color.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <string>
#include <system_error>
#include <utility>

namespace rgbpicker {

Color scaleBrightness(Color color, int percent)
{
    const int level{std::clamp(percent, 0, 100)};
    const auto scale{[level](std::uint8_t channel) {
        return static_cast<std::uint8_t>((static_cast<int>(channel) * level + 50) / 100);
    }};
    return Color{scale(color.red), scale(color.green), scale(color.blue)};
}

namespace {

std::string lowercase(std::string_view value)
{
    std::string lowered{value};
    std::ranges::transform(lowered, lowered.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return lowered;
}

}

std::optional<Color> parseColor(std::string_view value)
{
    static const std::array<std::pair<std::string_view, Color>, 12> namedColors{{
        {"red", {255, 0, 0}},
        {"green", {0, 255, 0}},
        {"blue", {0, 0, 255}},
        {"white", {255, 255, 255}},
        {"yellow", {255, 200, 0}},
        {"cyan", {0, 255, 255}},
        {"magenta", {255, 0, 255}},
        {"orange", {255, 90, 0}},
        {"purple", {160, 32, 240}},
        {"pink", {255, 64, 128}},
        {"off", {0, 0, 0}},
        {"black", {0, 0, 0}},
    }};

    const std::string lowered{lowercase(value)};
    for (const auto& [name, color] : namedColors) {
        if (lowered == name) {
            return color;
        }
    }

    std::string_view hex{lowered};
    if (hex.starts_with('#')) {
        hex.remove_prefix(1);
    }
    if (hex.size() != 6) {
        return std::nullopt;
    }

    std::uint32_t packed{};
    const auto result{std::from_chars(hex.data(), hex.data() + hex.size(), packed, 16)};
    if (result.ec != std::errc{} || result.ptr != hex.data() + hex.size()) {
        return std::nullopt;
    }

    return Color{static_cast<std::uint8_t>(packed >> 16U),
                 static_cast<std::uint8_t>((packed >> 8U) & 0xffU),
                 static_cast<std::uint8_t>(packed & 0xffU)};
}

}
