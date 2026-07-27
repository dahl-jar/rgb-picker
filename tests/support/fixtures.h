#pragma once

#include "rgbpicker/backend.h"
#include "rgbpicker/backend_session.h"

#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace test_fixture {

inline std::vector<rgbpicker::Device> primaryDevices()
{
    using rgbpicker::Color;
    using rgbpicker::Device;
    using rgbpicker::Zone;
    using rgbpicker::ZoneUnit;

    using rgbpicker::DeviceType;
    using rgbpicker::Mode;

    const Color off{0, 0, 0};
    return {
        Device{.id = 0,
               .name = "Lian Li Uni Hub - SL V2 v0.5",
               .type = DeviceType::cooler,
               .zones = {Zone{0, "Channel 1", 64, 0, 96, ZoneUnit::leds, off},
                         Zone{1, "Channel 2", 64, 0, 96, ZoneUnit::leds, off},
                         Zone{2, "Channel 3", 64, 0, 96, ZoneUnit::leds, off},
                         Zone{3, "Channel 4", 64, 0, 96, ZoneUnit::leds, off}},
               .modes = {Mode{"Static", true}, Mode{"Rainbow Wave", false}},
               .activeMode = "Static",
               .color = off},
        Device{.id = 1,
               .name = "Lian Li Uni Hub - SL",
               .type = DeviceType::cooler,
               .zones = {Zone{0, "Channel 1", 4, 0, 4, ZoneUnit::fans, off},
                         Zone{1, "Channel 2", 4, 0, 4, ZoneUnit::fans, off},
                         Zone{2, "Channel 3", 4, 0, 4, ZoneUnit::fans, off},
                         Zone{3, "Channel 4", 4, 0, 4, ZoneUnit::fans, off}},
               .modes = {Mode{"Static", true}, Mode{"Rainbow Wave", false}},
               .activeMode = "Static",
               .color = off},
    };
}

struct BackendCall {
    std::string operation;
    std::uint32_t deviceId{};
    std::uint32_t zoneId{};
    int size{};
    rgbpicker::Color color{};
    std::string mode;
};

struct BackendState {
    std::vector<rgbpicker::Device> devices{primaryDevices()};
    std::vector<BackendCall> calls;
    bool failDiscovery{};
    bool failMutation{};
};

class RecordingBackend final : public rgbpicker::Backend {
public:
    explicit RecordingBackend(std::shared_ptr<BackendState> state) : m_state{std::move(state)} {}

    std::expected<std::vector<rgbpicker::Device>, rgbpicker::BackendError> discover() override
    {
        if (m_state->failDiscovery) {
            return std::unexpected{rgbpicker::BackendError::unavailable};
        }
        return m_state->devices;
    }

    std::expected<rgbpicker::Device, rgbpicker::BackendError>
    changeDeviceColor(std::uint32_t deviceId, rgbpicker::Color color) override
    {
        m_state->calls.push_back(BackendCall{"change-device-color", deviceId, 0, 0, color, {}});
        auto result{mutationResult(deviceId)};
        if (result.has_value()) {
            result->color = color;
            for (auto& zone : result->zones) {
                zone.color = color;
            }
            store(*result);
        }
        return result;
    }

    std::expected<rgbpicker::Device, rgbpicker::BackendError>
    changeZoneColor(std::uint32_t deviceId, std::uint32_t zoneId, rgbpicker::Color color) override
    {
        m_state->calls.push_back(
            BackendCall{"change-zone-color", deviceId, zoneId, 0, color, {}});
        auto result{mutationResult(deviceId)};
        if (result.has_value() && zoneId < result->zones.size()) {
            result->zones.at(zoneId).color = color;
            store(*result);
        }
        return result;
    }

    std::expected<rgbpicker::Device, rgbpicker::BackendError>
    resizeZone(std::uint32_t deviceId, std::uint32_t zoneId, int size) override
    {
        m_state->calls.push_back(BackendCall{"resize-zone", deviceId, zoneId, size, {}, {}});
        auto result{mutationResult(deviceId)};
        if (result.has_value() && zoneId < result->zones.size()) {
            result->zones.at(zoneId).size = size;
            store(*result);
        }
        return result;
    }

    std::expected<rgbpicker::Device, rgbpicker::BackendError>
    changeMode(std::uint32_t deviceId, std::string_view mode) override
    {
        m_state->calls.push_back(
            BackendCall{"change-mode", deviceId, 0, 0, {}, std::string{mode}});
        auto result{mutationResult(deviceId)};
        if (result.has_value()) {
            result->activeMode = mode;
            store(*result);
        }
        return result;
    }

private:
    void store(const rgbpicker::Device& changed)
    {
        for (auto& device : m_state->devices) {
            if (device.id == changed.id) {
                device = changed;
                return;
            }
        }
    }

    std::expected<rgbpicker::Device, rgbpicker::BackendError> mutationResult(std::uint32_t deviceId)
    {
        if (m_state->failMutation) {
            return std::unexpected{rgbpicker::BackendError::operationFailed};
        }
        for (const auto& device : m_state->devices) {
            if (device.id == deviceId) {
                return device;
            }
        }
        return std::unexpected{rgbpicker::BackendError::notFound};
    }

    std::shared_ptr<BackendState> m_state;
};

class RecordingBackendFactory final : public rgbpicker::BackendFactory {
public:
    std::shared_ptr<BackendState> state{std::make_shared<BackendState>()};
    int hardwareCreations{};
    int simulationCreations{};
    bool failHardwareCreation{};

    std::expected<std::unique_ptr<rgbpicker::Backend>, rgbpicker::BackendError>
    createHardware() override
    {
        ++hardwareCreations;
        if (failHardwareCreation) {
            return std::unexpected{rgbpicker::BackendError::unavailable};
        }
        return std::make_unique<RecordingBackend>(state);
    }

    std::unique_ptr<rgbpicker::Backend> createSimulation() override
    {
        ++simulationCreations;
        return std::make_unique<RecordingBackend>(state);
    }
};

class ManualClock {
public:
    std::chrono::steady_clock::time_point now() const { return m_now; }

    void advance(std::chrono::milliseconds delta) { m_now += delta; }

    rgbpicker::SteadyNow reader() { return [this] { return m_now; }; }

    rgbpicker::Sleeper sleeper()
    {
        return [this](std::chrono::milliseconds delta) { m_now += delta; };
    }

private:
    std::chrono::steady_clock::time_point m_now{std::chrono::steady_clock::time_point{} +
                                                std::chrono::hours{1}};
};

}
