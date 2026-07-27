#include "rgbpicker/settings.h"

#include "storage/config_file.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <format>
#include <sstream>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace rgbpicker {
namespace {

constexpr std::string_view runAtLoginKey{"run-at-login"};
constexpr std::string_view restoreColorKey{"restore-color"};
constexpr std::string_view brightnessKey{"brightness"};
constexpr std::string_view lastColorKey{"last-color"};
constexpr std::string_view activeProfileKey{"active-profile"};

void readBool(std::string_view value, bool& field)
{
    if (value == "true" || value == "false") {
        field = value == "true";
    }
}

void readPercent(std::string_view value, int& field)
{
    int percent{};
    const auto* const end{value.data() + value.size()};
    const auto read{std::from_chars(value.data(), end, percent)};
    if (read.ec == std::errc{} && read.ptr == end && percent >= 0 && percent <= 100) {
        field = percent;
    }
}

void readColor(std::string_view value, Color& field)
{
    if (const auto color{parseColor(value)}; color.has_value()) {
        field = *color;
    }
}

struct Entry {
    std::string_view key;
    void (*read)(std::string_view, Settings&);
};

constexpr std::array<Entry, 5> entries{{
    {runAtLoginKey, [](std::string_view v, Settings& s) { readBool(v, s.runAtLogin); }},
    {restoreColorKey, [](std::string_view v, Settings& s) { readBool(v, s.restoreColor); }},
    {brightnessKey, [](std::string_view v, Settings& s) { readPercent(v, s.brightness); }},
    {lastColorKey, [](std::string_view v, Settings& s) { readColor(v, s.lastColor); }},
    {activeProfileKey, [](std::string_view v, Settings& s) { s.activeProfile = v; }},
}};

void applyEntry(Settings& settings, std::string_view key, std::string_view value)
{
    const auto entry{std::ranges::find(entries, key, &Entry::key)};
    if (entry != entries.end()) {
        entry->read(value, settings);
    }
}

#if defined(_WIN32)

constexpr std::wstring_view runKey{L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"};

std::wstring widen(std::string_view value)
{
    if (value.empty()) {
        return {};
    }
    const int length{MultiByteToWideChar(CP_ACP, 0, value.data(), static_cast<int>(value.size()),
                                         nullptr, 0)};
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_ACP, 0, value.data(), static_cast<int>(value.size()), wide.data(),
                        length);
    return wide;
}

std::wstring loginCommand()
{
    std::array<wchar_t, MAX_PATH> module{};
    const DWORD length{
        GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()))};
    return L'"' + std::wstring{module.data(), length} + L"\" --startminimized";
}

class WindowsLoginStartup final : public LoginStartup {
public:
    explicit WindowsLoginStartup(std::string_view appName) : m_valueName{widen(appName)} {}

    bool isEnabled() override
    {
        DWORD size{0};
        const LSTATUS status{RegGetValueW(HKEY_CURRENT_USER, runKey.data(), m_valueName.c_str(),
                                          RRF_RT_REG_SZ, nullptr, nullptr, &size)};
        return status == ERROR_SUCCESS;
    }

    bool setEnabled(bool enabled) override
    {
        HKEY key{};
        if (RegOpenKeyExW(HKEY_CURRENT_USER, runKey.data(), 0, KEY_SET_VALUE, &key) !=
            ERROR_SUCCESS) {
            return false;
        }
        LSTATUS status{};
        if (enabled) {
            const std::wstring command{loginCommand()};
            const auto* const bytes{reinterpret_cast<const BYTE*>(command.c_str())};
            status = RegSetValueExW(key, m_valueName.c_str(), 0, REG_SZ, bytes,
                                    static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
        } else {
            status = RegDeleteValueW(key, m_valueName.c_str());
            if (status == ERROR_FILE_NOT_FOUND) {
                status = ERROR_SUCCESS;
            }
        }
        RegCloseKey(key);
        return status == ERROR_SUCCESS;
    }

    bool refresh() override
    {
        const std::wstring stored{storedCommand()};
        if (stored.empty()) {
            return false;
        }
        if (stored != loginCommand()) {
            setEnabled(true);
        }
        return true;
    }

private:
    std::wstring storedCommand()
    {
        DWORD size{0};
        if (RegGetValueW(HKEY_CURRENT_USER, runKey.data(), m_valueName.c_str(), RRF_RT_REG_SZ,
                         nullptr, nullptr, &size) != ERROR_SUCCESS ||
            size < sizeof(wchar_t)) {
            return {};
        }
        std::wstring value(size / sizeof(wchar_t), L'\0');
        if (RegGetValueW(HKEY_CURRENT_USER, runKey.data(), m_valueName.c_str(), RRF_RT_REG_SZ,
                         nullptr, value.data(), &size) != ERROR_SUCCESS ||
            size < sizeof(wchar_t)) {
            return {};
        }
        value.resize(size / sizeof(wchar_t) - 1);
        return value;
    }

    std::wstring m_valueName;
};

#else

class UnsupportedLoginStartup final : public LoginStartup {
public:
    bool isEnabled() override { return false; }
    bool setEnabled(bool) override { return false; }
    bool refresh() override { return false; }
};

#endif

}

std::string serializeSettings(const Settings& settings)
{
    return std::format(
        "# RGB Picker settings\n{}={}\n{}={}\n{}={}\n{}=#{:02x}{:02x}{:02x}\n{}={}\n",
        runAtLoginKey, settings.runAtLogin ? "true" : "false", restoreColorKey,
        settings.restoreColor ? "true" : "false", brightnessKey, settings.brightness, lastColorKey,
        settings.lastColor.red, settings.lastColor.green, settings.lastColor.blue,
        activeProfileKey, settings.activeProfile);
}

Settings parseSettings(std::string_view text)
{
    Settings settings;
    std::istringstream lines{std::string{text}};
    for (std::string line; std::getline(lines, line);) {
        const std::string_view trimmed{config_file::trim(line)};
        if (trimmed.empty() || trimmed.front() == '#') {
            continue;
        }
        const auto separator{trimmed.find('=')};
        if (separator == std::string_view::npos) {
            continue;
        }
        applyEntry(settings, config_file::trim(trimmed.substr(0, separator)),
                   config_file::trim(trimmed.substr(separator + 1)));
    }
    return settings;
}

std::filesystem::path settingsFilePath()
{
    return config_file::directory() / "settings.ini";
}

Settings loadSettings()
{
    return parseSettings(config_file::read(settingsFilePath()));
}

bool saveSettings(const Settings& settings)
{
    return config_file::write(settingsFilePath(), serializeSettings(settings));
}

namespace {

class FileSettingsStore final : public SettingsStore {
public:
    Settings load() override { return loadSettings(); }
    bool save(const Settings& settings) override { return saveSettings(settings); }
};

}

std::unique_ptr<SettingsStore> makeSettingsStore()
{
    return std::make_unique<FileSettingsStore>();
}

std::unique_ptr<LoginStartup> makeLoginStartup(std::string_view appName)
{
#if defined(_WIN32)
    return std::make_unique<WindowsLoginStartup>(appName);
#else
    static_cast<void>(appName);
    return std::make_unique<UnsupportedLoginStartup>();
#endif
}

}
