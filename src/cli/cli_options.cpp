#include "cli/cli_options.h"

#include <algorithm>
#include <charconv>
#include <limits>
#include <string_view>

namespace rgbpicker {
namespace {

template <typename Integer>
std::optional<Integer> parseInteger(std::string_view value, Integer minimum, Integer maximum)
{
    Integer parsed{};
    const auto result{std::from_chars(value.data(), value.data() + value.size(), parsed)};
    if (value.empty() || result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
        parsed < minimum || parsed > maximum) {
        return std::nullopt;
    }
    return parsed;
}

}

void printUsage(std::ostream& error)
{
    error << "usage: rgb-ctl <command>\n"
             "commands: list, set, zone, resize, mode, rainbow\n";
}

namespace {

bool parseColorArgument(const Options& options, CommandArguments& parsed, std::ostream& error)
{
    const std::string& name{options.command.front()};
    if (name != "set" && name != "zone") {
        return true;
    }
    const std::size_t index{name == "set" ? 1U : 3U};
    parsed.color = parseColor(options.command.at(index));
    if (parsed.color.has_value()) {
        return true;
    }
    error << "unknown color: " << options.command.at(index) << '\n';
    return false;
}

bool parseDeviceArgument(const Options& options, CommandArguments& parsed, std::ostream& error)
{
    const std::string& name{options.command.front()};
    if (name != "zone" && name != "resize" && name != "mode") {
        return true;
    }
    parsed.deviceId = parseInteger<std::uint32_t>(
        options.command.at(1), 0, std::numeric_limits<std::uint32_t>::max());
    if (parsed.deviceId.has_value()) {
        return true;
    }
    error << "invalid device ID: " << options.command.at(1) << '\n';
    return false;
}

bool parseZoneArgument(const Options& options, CommandArguments& parsed, std::ostream& error)
{
    const std::string& name{options.command.front()};
    if (name != "zone" && name != "resize") {
        return true;
    }
    parsed.zoneId = parseInteger<std::uint32_t>(
        options.command.at(2), 0, std::numeric_limits<std::uint32_t>::max());
    if (parsed.zoneId.has_value()) {
        return true;
    }
    error << "invalid zone ID: " << options.command.at(2) << '\n';
    return false;
}

bool parseSizeArgument(const Options& options, CommandArguments& parsed, std::ostream& error)
{
    if (options.command.front() != "resize") {
        return true;
    }
    parsed.size = parseInteger<int>(options.command.at(3), std::numeric_limits<int>::min(),
                                    std::numeric_limits<int>::max());
    if (parsed.size.has_value()) {
        return true;
    }
    error << "invalid size: " << options.command.at(3) << '\n';
    return false;
}

}

std::optional<CommandArguments> parseCommandArguments(const Options& options, std::ostream& error)
{
    CommandArguments parsed;
    if (!parseColorArgument(options, parsed, error) ||
        !parseDeviceArgument(options, parsed, error) ||
        !parseZoneArgument(options, parsed, error) || !parseSizeArgument(options, parsed, error)) {
        return std::nullopt;
    }
    return parsed;
}


}
