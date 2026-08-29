#pragma once

#include "rgbpicker/backend.h"
#include "rgbpicker/backend_session.h"
#include "rgbpicker/color.h"
#include "rgbpicker/profiles.h"

#include "imgui.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <expected>
#include <format>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace rgbpicker::gui {

using rgbpicker::Backend;
using rgbpicker::BackendError;
using rgbpicker::BackendSessionPhase;
using rgbpicker::Color;
using rgbpicker::Device;
using rgbpicker::ZoneUnit;

constexpr auto maintenanceInterval{std::chrono::milliseconds{250}};

constexpr auto backendProbeInterval{std::chrono::seconds{3}};

constexpr auto restoreWindow{std::chrono::seconds{30}};

constexpr auto firstRestoreRetry{std::chrono::milliseconds{2000}};

inline std::string errorText(BackendError error)
{
    switch (error) {
    case BackendError::notFound: return "device or zone not found";
    case BackendError::invalidArgument: return "value out of range";
    case BackendError::unavailable: return "RGB hardware unavailable";
    case BackendError::operationFailed: return "hardware operation failed";
    }
    return "unknown error";
}

inline Color colorFromFloats(const float (&rgb)[3])
{
    const auto channel{[](float value) {
        return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
    }};
    return Color{channel(rgb[0]), channel(rgb[1]), channel(rgb[2])};
}

inline Color colorFromHue(float hueDegrees)
{
    float red{};
    float green{};
    float blue{};
    ImGui::ColorConvertHSVtoRGB(hueDegrees / 360.0f, 1.0f, 1.0f, red, green, blue);
    const float rgb[3]{red, green, blue};
    return colorFromFloats(rgb);
}

inline ImVec4 toImVec4(Color color)
{
    return ImVec4{static_cast<float>(color.red) / 255.0f, static_cast<float>(color.green) / 255.0f,
                  static_cast<float>(color.blue) / 255.0f, 1.0f};
}

struct UiState {
    std::mutex mutex;
    std::vector<Device> devices;
    std::string status{"Hardware not ready"};
    bool backendReady{false};
    int pendingJobs{0};
    std::uint64_t revision{0};
    Color lastColor{255, 255, 255};
    std::vector<rgbpicker::DeviceColor> applied;
};

struct WorkerConfig {
    bool restoreColor{true};
    int brightness{100};
    rgbpicker::AppliedStore* appliedStore{nullptr};
    rgbpicker::LayoutStore* layoutStore{nullptr};
};

class Worker {
public:
    Worker(rgbpicker::BackendFactory& factory, UiState& state, const WorkerConfig& config)
        : m_session{factory, rgbpicker::BackendSessionConfig{},
                    [] { return std::chrono::steady_clock::now(); }},
          m_state{state},
          m_restoreColor{config.restoreColor},
          m_brightness{config.brightness},
          m_appliedStore{config.appliedStore},
          m_layoutStore{config.layoutStore},
          m_layout{config.layoutStore == nullptr ? std::vector<rgbpicker::ZoneSize>{}
                                                 : config.layoutStore->load()},
          m_savedLook{rgbpicker::sortedByDevice(state.applied)}
    {
    }

    void postRestorePreference(bool restoreColor)
    {
        post([this, restoreColor] { m_restoreColor = restoreColor; });
    }

    void postBrightness(int brightness);

    void postDeviceColor(std::uint32_t deviceId, Color color)
    {
        post([this, deviceId, color] { writeDeviceColor(deviceId, color); });
    }

    void postProfile(std::vector<rgbpicker::DeviceColor> entries)
    {
        post([this, entries = std::move(entries)] { applyEntries(entries); });
    }

    void postZoneColor(std::uint32_t deviceId, std::uint32_t zoneId, Color color);

    void postResize(std::uint32_t deviceId, std::uint32_t zoneId, int size);

    void postMode(std::uint32_t deviceId, std::string mode);

    void postAllDevicesColor(Color color);

    int pending();

private:
    void post(std::function<void()> job);

    void run(const std::stop_token& stopToken);

    void maintainBackend();

    void probeBackend();

    void refreshOnWorker();

    std::vector<rgbpicker::DeviceColor> restoreLastLook();

    void beginRestore();

    void continueRestore();

    void restoreLayout();

    void rememberZoneSize(std::uint32_t deviceId, std::uint32_t zoneId, int size);

    void colorEveryDevice(Color color);

    std::vector<rgbpicker::DeviceColor>
    applyEntries(const std::vector<rgbpicker::DeviceColor>& entries);

    void flushLook();

    bool writeDeviceColor(std::uint32_t deviceId, Color color);

    void ensureColorableMode(std::uint32_t deviceId);

    void rememberColor(Color color);

    void rememberDeviceColor(std::uint32_t deviceId, Color color);

    std::optional<std::uint32_t> findDeviceId(const std::string& name);

    std::optional<std::uint32_t> findZoneId(std::uint32_t deviceId, const std::string& name);

    void runDeviceOp(const std::function<std::expected<Device, BackendError>(Backend&)>& op);

    void noteError(BackendError error);

    void storeDevice(Device device);

    void setStatus(std::string text);

    void publishStatus();

    std::mutex m_mutex;
    std::condition_variable_any m_wake;
    std::deque<std::function<void()>> m_jobs;
    rgbpicker::BackendSession m_session;
    UiState& m_state;
    bool m_restoreColor{true};
    int m_brightness{100};
    bool m_backendReady{false};
    bool m_restoreStarted{false};
    std::vector<rgbpicker::DeviceColor> m_pendingRestore;
    std::chrono::steady_clock::time_point m_restoreDeadline{};
    std::chrono::steady_clock::time_point m_nextRestoreAttempt{};
    std::chrono::milliseconds m_restoreRetry{firstRestoreRetry};
    std::chrono::steady_clock::time_point m_lastProbe{};
    BackendSessionPhase m_publishedPhase{BackendSessionPhase::idle};
    std::string m_publishedDetail;
    rgbpicker::AppliedStore* m_appliedStore{nullptr};
    rgbpicker::LayoutStore* m_layoutStore{nullptr};
    std::vector<rgbpicker::ZoneSize> m_layout;
    std::vector<rgbpicker::DeviceColor> m_savedLook;
    std::jthread m_thread{[this](const std::stop_token& stopToken) { run(stopToken); }};
};
}
