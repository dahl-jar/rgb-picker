#pragma once

#include "rgbpicker/color.h"

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace rgbpicker {

struct Options {
    std::vector<std::string> command;
};

struct CommandArguments {
    std::optional<Color> color;
    std::optional<std::uint32_t> deviceId;
    std::optional<std::uint32_t> zoneId;
    std::optional<int> size;
};

void printUsage(std::ostream& error);

std::optional<CommandArguments> parseCommandArguments(const Options& options,
                                                      std::ostream& error);

}
