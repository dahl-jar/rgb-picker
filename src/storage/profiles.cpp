#include "rgbpicker/profiles.h"

#include "storage/config_file.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>

namespace rgbpicker {
namespace {

bool isSectionHeader(std::string_view line)
{
    return line.size() > 2 && line.front() == '[' && line.back() == ']';
}

std::string serializeSections(std::string_view title, const std::vector<Profile>& sections)
{
    std::string text{std::format("# {}\n", title)};
    for (const Profile& section : sections) {
        text += std::format("\n[{}]\n", section.name);
        for (const DeviceColor& entry : section.devices) {
            text += std::format("{}=#{:02x}{:02x}{:02x}\n", entry.device, entry.color.red,
                                entry.color.green, entry.color.blue);
        }
    }
    return text;
}

}

std::string serializeProfiles(const std::vector<Profile>& profiles)
{
    return serializeSections("rgb-picker profiles", profiles);
}

std::vector<Profile> parseProfiles(std::string_view text)
{
    std::vector<Profile> profiles;
    std::istringstream lines{std::string{text}};
    for (std::string line; std::getline(lines, line);) {
        const std::string_view trimmed{config_file::trim(line)};
        if (trimmed.empty() || trimmed.front() == '#') {
            continue;
        }
        if (isSectionHeader(trimmed)) {
            profiles.push_back(Profile{std::string{trimmed.substr(1, trimmed.size() - 2)}, {}});
            continue;
        }
        const auto separator{trimmed.find('=')};
        if (profiles.empty() || separator == std::string_view::npos) {
            continue;
        }
        const auto color{parseColor(config_file::trim(trimmed.substr(separator + 1)))};
        if (!color.has_value()) {
            continue;
        }
        profiles.back().devices.push_back(
            DeviceColor{std::string{config_file::trim(trimmed.substr(0, separator))}, *color});
    }
    return profiles;
}

std::filesystem::path profilesFilePath()
{
    return config_file::directory() / "profiles.ini";
}

std::vector<Profile> loadProfiles()
{
    return parseProfiles(config_file::read(profilesFilePath()));
}

bool saveProfiles(const std::vector<Profile>& profiles)
{
    return config_file::write(profilesFilePath(), serializeProfiles(profiles));
}

namespace {

class FileProfileStore final : public ProfileStore {
public:
    std::vector<Profile> load() override { return loadProfiles(); }
    bool save(const std::vector<Profile>& profiles) override { return saveProfiles(profiles); }
};

}

std::unique_ptr<ProfileStore> makeProfileStore()
{
    return std::make_unique<FileProfileStore>();
}

std::vector<DeviceColor> sortedByDevice(std::vector<DeviceColor> look)
{
    std::ranges::sort(look, {}, &DeviceColor::device);
    return look;
}

bool sameLook(std::vector<DeviceColor> left, std::vector<DeviceColor> right)
{
    return sortedByDevice(std::move(left)) == sortedByDevice(std::move(right));
}

std::string serializeApplied(const std::vector<DeviceColor>& applied)
{
    return serializeSections("rgb-picker applied colors, written by the app",
                             {Profile{"applied", sortedByDevice(applied)}});
}

std::vector<DeviceColor> parseApplied(std::string_view text)
{
    const std::vector<Profile> sections{parseProfiles(text)};
    if (sections.empty()) {
        return {};
    }
    return sections.front().devices;
}

std::filesystem::path appliedFilePath()
{
    return config_file::directory() / "applied.ini";
}

std::vector<DeviceColor> loadApplied()
{
    return parseApplied(config_file::read(appliedFilePath()));
}

bool saveApplied(const std::vector<DeviceColor>& applied)
{
    return config_file::write(appliedFilePath(), serializeApplied(applied));
}

namespace {

class FileAppliedStore final : public AppliedStore {
public:
    std::vector<DeviceColor> load() override { return loadApplied(); }
    bool save(const std::vector<DeviceColor>& applied) override { return saveApplied(applied); }
};

}

std::unique_ptr<AppliedStore> makeAppliedStore()
{
    return std::make_unique<FileAppliedStore>();
}

std::string serializeLayout(const std::vector<ZoneSize>& layout)
{
    std::vector<ZoneSize> ordered{layout};
    std::ranges::sort(ordered, {}, [](const ZoneSize& entry) {
        return std::tie(entry.device, entry.zone);
    });

    std::string text{"# rgb-picker zone sizes, written by the app\n"};
    std::string_view section;
    for (const ZoneSize& entry : ordered) {
        if (entry.device != section) {
            section = entry.device;
            text += std::format("\n[{}]\n", entry.device);
        }
        text += std::format("{}={}\n", entry.zone, entry.size);
    }
    return text;
}

std::vector<ZoneSize> parseLayout(std::string_view text)
{
    std::vector<ZoneSize> layout;
    std::string device;
    std::istringstream lines{std::string{text}};
    for (std::string line; std::getline(lines, line);) {
        const std::string_view trimmed{config_file::trim(line)};
        if (trimmed.empty() || trimmed.front() == '#') {
            continue;
        }
        if (isSectionHeader(trimmed)) {
            device = std::string{trimmed.substr(1, trimmed.size() - 2)};
            continue;
        }
        const auto separator{trimmed.find('=')};
        if (device.empty() || separator == std::string_view::npos) {
            continue;
        }
        const std::string_view value{config_file::trim(trimmed.substr(separator + 1))};
        int size{};
        const auto* const end{value.data() + value.size()};
        const auto read{std::from_chars(value.data(), end, size)};
        if (read.ec != std::errc{} || read.ptr != end || size < 0) {
            continue;
        }
        layout.push_back(
            ZoneSize{device, std::string{config_file::trim(trimmed.substr(0, separator))}, size});
    }
    return layout;
}

std::filesystem::path layoutFilePath()
{
    return config_file::directory() / "layout.ini";
}

std::vector<ZoneSize> loadLayout()
{
    return parseLayout(config_file::read(layoutFilePath()));
}

bool saveLayout(const std::vector<ZoneSize>& layout)
{
    return config_file::write(layoutFilePath(), serializeLayout(layout));
}

void storeZoneSize(std::vector<ZoneSize>& layout, ZoneSize size)
{
    const auto existing{std::ranges::find_if(layout, [&size](const ZoneSize& entry) {
        return entry.device == size.device && entry.zone == size.zone;
    })};
    if (existing == layout.end()) {
        layout.push_back(std::move(size));
        return;
    }
    *existing = std::move(size);
}

namespace {

class FileLayoutStore final : public LayoutStore {
public:
    std::vector<ZoneSize> load() override { return loadLayout(); }
    bool save(const std::vector<ZoneSize>& layout) override { return saveLayout(layout); }
};

}

std::unique_ptr<LayoutStore> makeLayoutStore()
{
    return std::make_unique<FileLayoutStore>();
}

void storeProfile(std::vector<Profile>& profiles, Profile profile)
{
    const auto existing{std::ranges::find(profiles, profile.name, &Profile::name)};
    if (existing == profiles.end()) {
        profiles.push_back(std::move(profile));
        return;
    }
    *existing = std::move(profile);
}

void removeProfile(std::vector<Profile>& profiles, std::string_view name)
{
    const auto removed{std::ranges::remove(profiles, name, &Profile::name)};
    profiles.erase(removed.begin(), removed.end());
}

}
