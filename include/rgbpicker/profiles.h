#pragma once

#include "rgbpicker/color.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace rgbpicker {

struct DeviceColor {
    std::string device;
    Color color{};

    bool operator==(const DeviceColor&) const = default;
};

struct Profile {
    std::string name;
    std::vector<DeviceColor> devices;

    bool operator==(const Profile&) const = default;
};

std::string serializeProfiles(const std::vector<Profile>& profiles);

std::vector<Profile> parseProfiles(std::string_view text);

std::filesystem::path profilesFilePath();

std::vector<Profile> loadProfiles();
bool saveProfiles(const std::vector<Profile>& profiles);

class ProfileStore {
public:
    virtual ~ProfileStore() = default;

    virtual std::vector<Profile> load() = 0;
    virtual bool save(const std::vector<Profile>& profiles) = 0;
};

std::unique_ptr<ProfileStore> makeProfileStore();

std::vector<DeviceColor> sortedByDevice(std::vector<DeviceColor> look);

bool sameLook(std::vector<DeviceColor> left, std::vector<DeviceColor> right);

std::string serializeApplied(const std::vector<DeviceColor>& applied);

std::vector<DeviceColor> parseApplied(std::string_view text);

std::filesystem::path appliedFilePath();

std::vector<DeviceColor> loadApplied();
bool saveApplied(const std::vector<DeviceColor>& applied);

class AppliedStore {
public:
    virtual ~AppliedStore() = default;

    virtual std::vector<DeviceColor> load() = 0;
    virtual bool save(const std::vector<DeviceColor>& applied) = 0;
};

std::unique_ptr<AppliedStore> makeAppliedStore();

struct ZoneSize {
    std::string device;
    std::string zone;
    int size{};

    bool operator==(const ZoneSize&) const = default;
};

std::string serializeLayout(const std::vector<ZoneSize>& layout);

std::vector<ZoneSize> parseLayout(std::string_view text);

std::filesystem::path layoutFilePath();

std::vector<ZoneSize> loadLayout();
bool saveLayout(const std::vector<ZoneSize>& layout);

void storeZoneSize(std::vector<ZoneSize>& layout, ZoneSize size);

class LayoutStore {
public:
    virtual ~LayoutStore() = default;

    virtual std::vector<ZoneSize> load() = 0;
    virtual bool save(const std::vector<ZoneSize>& layout) = 0;
};

std::unique_ptr<LayoutStore> makeLayoutStore();

void storeProfile(std::vector<Profile>& profiles, Profile profile);
void removeProfile(std::vector<Profile>& profiles, std::string_view name);

}
