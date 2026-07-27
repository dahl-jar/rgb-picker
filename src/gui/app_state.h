#pragma once

#include "rgbpicker/profiles.h"
#include "rgbpicker/settings.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace rgbpicker::gui {

inline rgbpicker::Settings g_settings;
inline std::vector<rgbpicker::Profile> g_profiles;
inline rgbpicker::LoginStartup* g_loginStartup{nullptr};
inline rgbpicker::SettingsStore* g_settingsStore{nullptr};
inline rgbpicker::ProfileStore* g_profileStore{nullptr};

inline void saveSettings()
{
    if (g_settingsStore != nullptr) {
        g_settingsStore->save(g_settings);
    }
}

inline void saveProfiles()
{
    if (g_profileStore != nullptr) {
        g_profileStore->save(g_profiles);
    }
}

inline const rgbpicker::Profile* activeProfile()
{
    if (g_settings.activeProfile.empty()) {
        return nullptr;
    }
    const auto found{
        std::ranges::find(g_profiles, g_settings.activeProfile, &rgbpicker::Profile::name)};
    return found == g_profiles.end() ? nullptr : &*found;
}

inline void setActiveProfile(std::string name)
{
    if (g_settings.activeProfile == name) {
        return;
    }
    g_settings.activeProfile = std::move(name);
    saveSettings();
}

}
