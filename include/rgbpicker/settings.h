#pragma once

#include "rgbpicker/color.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace rgbpicker {

struct Settings {
    bool runAtLogin{false};
    bool restoreColor{true};
    int brightness{100};
    Color lastColor{255, 255, 255};
    std::string activeProfile;

    bool operator==(const Settings&) const = default;
};

std::string serializeSettings(const Settings& settings);

Settings parseSettings(std::string_view text);

std::filesystem::path settingsFilePath();

class SettingsStore {
public:
    virtual ~SettingsStore() = default;

    virtual Settings load() = 0;
    virtual bool save(const Settings& settings) = 0;
};

std::unique_ptr<SettingsStore> makeSettingsStore();

Settings loadSettings();
bool saveSettings(const Settings& settings);

class LoginStartup {
public:
    virtual ~LoginStartup() = default;

    virtual bool isEnabled() = 0;
    virtual bool setEnabled(bool enabled) = 0;

    virtual bool refresh() = 0;
};

std::unique_ptr<LoginStartup> makeLoginStartup(std::string_view appName);

}
