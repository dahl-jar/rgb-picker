#include "rgbpicker/cli.h"

#include "cli/cli_options.h"

#include "rgbpicker/color.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace rgbpicker {
namespace {

constexpr std::string_view defaultFilter{"lian"};
constexpr float hueStep{3.0F};
constexpr float hueCircle{360.0F};
constexpr auto rainbowDelay{std::chrono::milliseconds{50}};

std::string lowercase(std::string_view value)
{
    std::string lowered{value};
    std::ranges::transform(lowered, lowered.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return lowered;
}

bool nameMatches(const Device& device, std::string_view filter)
{
    return lowercase(device.name).find(lowercase(filter)) != std::string::npos;
}

const Device* findDevice(const std::vector<Device>& devices, std::uint32_t deviceId)
{
    const auto device{std::ranges::find(devices, deviceId, &Device::id)};
    return device == devices.end() ? nullptr : &*device;
}

const Zone* findZone(const Device& device, std::uint32_t zoneId)
{
    const auto zone{std::ranges::find(device.zones, zoneId, &Zone::id)};
    return zone == device.zones.end() ? nullptr : &*zone;
}

std::string_view unitName(ZoneUnit unit)
{
    return unit == ZoneUnit::fans ? "fans" : "LEDs";
}

void printDevices(const std::vector<Device>& devices, std::ostream& output)
{
    for (const auto& device : devices) {
        output << '[' << device.id << "] " << device.name << "  ("
               << deviceTypeLabel(device.type) << ")\n";
        for (const auto& zone : device.zones) {
            output << "      zone " << zone.id << ": " << zone.name << ' ' << zone.size << ' '
                   << unitName(zone.unit) << " (min " << zone.minSize << ", max " << zone.maxSize
                   << ")\n";
        }
        for (const auto& mode : device.modes) {
            output << "      mode: " << mode.name;
            if (mode.name == device.activeMode) {
                output << " (active)";
            }
            if (mode.perLedColor) {
                output << " [takes colors]";
            }
            output << '\n';
        }
    }
}

int mutationFailure(BackendError backendError, std::string_view operation, std::ostream& error)
{
    if (backendError == BackendError::invalidArgument) {
        error << "invalid argument for " << operation << '\n';
        return 1;
    }
    error << operation << " failed\n";
    return 2;
}

Color hsvToRgb(float hue)
{
    const float chroma{1.0F};
    const float second{chroma * (1.0F - std::fabs(std::fmod(hue / 60.0F, 2.0F) - 1.0F))};
    float red{};
    float green{};
    float blue{};
    if (hue < 60.0F) {
        red = chroma;
        green = second;
    } else if (hue < 120.0F) {
        red = second;
        green = chroma;
    } else if (hue < 180.0F) {
        green = chroma;
        blue = second;
    } else if (hue < 240.0F) {
        green = second;
        blue = chroma;
    } else if (hue < 300.0F) {
        red = second;
        blue = chroma;
    } else {
        red = chroma;
        blue = second;
    }
    return Color{static_cast<std::uint8_t>(red * 255.0F),
                 static_cast<std::uint8_t>(green * 255.0F),
                 static_cast<std::uint8_t>(blue * 255.0F)};
}

struct CommandContext {
    const Options& options;
    const CommandArguments& parsed;
    const std::vector<Device>& devices;
    Backend& backend;
    std::ostream& output;
    std::ostream& error;
};

int runSet(const CommandContext& context)
{
    const Options& options{context.options};
    const CommandArguments& parsed{context.parsed};
    const std::vector<Device>& devices{context.devices};
    Backend& backend{context.backend};
    std::ostream& output{context.output};
    std::ostream& error{context.error};
    const std::string_view filter{options.command.size() == 3
                                      ? std::string_view{options.command.at(2)}
                                      : defaultFilter};
    int matches{};
    for (const auto& device : devices) {
        if (!nameMatches(device, filter)) {
            continue;
        }
        ++matches;
        const auto changed{backend.changeDeviceColor(device.id, *parsed.color)};
        if (!changed.has_value()) {
            return mutationFailure(changed.error(), "device color", error);
        }
        output << "Set " << device.name << " to " << options.command.at(1) << '\n';
    }
    if (matches == 0) {
        error << "No devices match filter: " << filter << '\n';
        return 1;
    }
    return 0;
}

int runRainbow(const CommandContext& context)
{
    const Options& options{context.options};
    const std::vector<Device>& devices{context.devices};
    Backend& backend{context.backend};
    std::ostream& output{context.output};
    std::ostream& error{context.error};
    const std::string_view filter{options.command.size() == 2
                                      ? std::string_view{options.command.at(1)}
                                      : defaultFilter};
    std::vector<std::uint32_t> targetIds;
    for (const auto& device : devices) {
        if (nameMatches(device, filter)) {
            targetIds.push_back(device.id);
        }
    }
    if (targetIds.empty()) {
        error << "no device matched filter: " << filter << '\n';
        return 1;
    }
    output << "rainbow active on " << targetIds.size() << " devices; Ctrl+C stops it\n";
    float hue{};
    while (true) {
        const Color rainbowColor{hsvToRgb(hue)};
        for (const std::uint32_t targetId : targetIds) {
            const auto changed{backend.changeDeviceColor(targetId, rainbowColor)};
            if (!changed.has_value()) {
                return mutationFailure(changed.error(), "rainbow color", error);
            }
        }
        hue = std::fmod(hue + hueStep, hueCircle);
        std::this_thread::sleep_for(rainbowDelay);
    }
}

int runMode(const CommandContext& context, const Device& device)
{
    const Options& options{context.options};
    Backend& backend{context.backend};
    std::ostream& output{context.output};
    std::ostream& error{context.error};
    const std::string& mode{options.command.at(2)};
    if (std::ranges::find(device.modes, mode, &Mode::name) == device.modes.end()) {
        error << "Unsupported mode for " << device.name << ": " << mode << '\n';
        return 1;
    }
    const auto changed{backend.changeMode(device.id, mode)};
    if (!changed.has_value()) {
        return mutationFailure(changed.error(), "mode change", error);
    }
    output << "Set " << changed->name << " mode to " << changed->activeMode << '\n';
    return 0;
}

int runZone(const CommandContext& context, const Device& device, const Zone& zone)
{
    const Options& options{context.options};
    const CommandArguments& parsed{context.parsed};
    Backend& backend{context.backend};
    std::ostream& output{context.output};
    std::ostream& error{context.error};
    const auto changed{backend.changeZoneColor(device.id, zone.id, *parsed.color)};
    if (!changed.has_value()) {
        if (changed.error() == BackendError::invalidArgument) {
            return mutationFailure(changed.error(), "zone color", error);
        }
        error << "zone color operation failed: device " << device.id << ", zone " << zone.id
              << '\n';
        return 2;
    }
    output << "Set " << changed->name << " / " << zone.name << " to "
           << options.command.at(3) << '\n';
    return 0;
}

int runResize(const CommandContext& context, const Device& device, const Zone& zone)
{
    const CommandArguments& parsed{context.parsed};
    Backend& backend{context.backend};
    std::ostream& output{context.output};
    std::ostream& error{context.error};
    if (*parsed.size < zone.minSize || *parsed.size > zone.maxSize) {
        error << "invalid argument for zone resize\n";
        return 1;
    }
    const auto changed{backend.resizeZone(device.id, zone.id, *parsed.size)};
    if (!changed.has_value()) {
        return mutationFailure(changed.error(), "zone resize", error);
    }
    const Zone* const changedZone{findZone(*changed, zone.id)};
    output << "Resized " << changed->name << " / " << changedZone->name << " to "
           << changedZone->size << ' ' << unitName(changedZone->unit) << '\n';
    return 0;
}

int runList(const CommandContext& context)
{
    printDevices(context.devices, context.output);
    return 0;
}

const Device* requireDevice(const CommandContext& context)
{
    const Device* const device{findDevice(context.devices, *context.parsed.deviceId)};
    if (device == nullptr) {
        context.error << "Device not found: " << *context.parsed.deviceId << '\n';
    }
    return device;
}

const Zone* requireZone(const CommandContext& context, const Device& device)
{
    const Zone* const zone{findZone(device, *context.parsed.zoneId)};
    if (zone == nullptr) {
        context.error << "Zone not found on " << device.name << ": " << *context.parsed.zoneId
                      << '\n';
    }
    return zone;
}

int runModeCommand(const CommandContext& context)
{
    const Device* const device{requireDevice(context)};
    return device == nullptr ? 1 : runMode(context, *device);
}

int runZoneCommand(const CommandContext& context)
{
    const Device* const device{requireDevice(context)};
    if (device == nullptr) {
        return 1;
    }
    const Zone* const zone{requireZone(context, *device)};
    return zone == nullptr ? 1 : runZone(context, *device, *zone);
}

int runResizeCommand(const CommandContext& context)
{
    const Device* const device{requireDevice(context)};
    if (device == nullptr) {
        return 1;
    }
    const Zone* const zone{requireZone(context, *device)};
    return zone == nullptr ? 1 : runResize(context, *device, *zone);
}

struct Command {
    std::string_view name;
    std::size_t minimum;
    std::size_t maximum;
    int (*run)(const CommandContext&);
};

constexpr std::array<Command, 6> commands{{
    {"list", 1, 1, runList},
    {"set", 2, 3, runSet},
    {"zone", 4, 4, runZoneCommand},
    {"resize", 4, 4, runResizeCommand},
    {"mode", 3, 3, runModeCommand},
    {"rainbow", 1, 2, runRainbow},
}};

const Command* findCommand(const std::vector<std::string>& words)
{
    if (words.empty()) {
        return nullptr;
    }
    const auto command{std::ranges::find(commands, words.front(), &Command::name)};
    return command == commands.end() ? nullptr : &*command;
}

bool hasValidArity(const std::vector<std::string>& words)
{
    const Command* const command{findCommand(words)};
    return command != nullptr && words.size() >= command->minimum &&
           words.size() <= command->maximum;
}

int runCommand(const CommandContext& context)
{
    const Command* const command{findCommand(context.options.command)};
    return command == nullptr ? 1 : command->run(context);
}

}

int runCli(const std::vector<std::string>& arguments, CliEnvironment& environment,
           std::ostream& output, std::ostream& error)
{
    const auto options{parseOptions(arguments)};
    if (!options.has_value()) {
        return 1;
    }
    if (!hasValidArity(options->command)) {
        printUsage(error);
        return 1;
    }

    const auto parsed{parseCommandArguments(*options, error)};
    if (!parsed.has_value()) {
        return 1;
    }

    BackendSessionConfig config;
    config.mode = options->simulate ? BackendMode::simulation : BackendMode::hardware;
    BackendSession session{environment.factory, config, environment.now};

    Backend* const backend{
        session.waitUntilReady(environment.backendWaitBudget, environment.sleep)};
    if (backend == nullptr) {
        error << "no supported RGB hardware found\n";
        return 2;
    }

    const auto discovered{backend->discover()};
    if (!discovered.has_value()) {
        error << "device listing failed\n";
        return 2;
    }
    const CommandContext context{*options, *parsed, *discovered, *backend, output,
                                 error};
    return runCommand(context);
}

}
