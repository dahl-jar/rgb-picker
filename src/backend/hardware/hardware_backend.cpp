#include "backend/hardware/hardware_backend.h"

#include "backend/hardware/driver_backend.h"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <hidapi.h>
#endif

namespace rgbpicker {
namespace {

#if defined(_WIN32)
class HidRuntime final {
public:
    HidRuntime() : m_initialized{hid_init() == 0} {}

    ~HidRuntime()
    {
        if (m_initialized) {
            hid_exit();
        }
    }

    HidRuntime(const HidRuntime&) = delete;
    HidRuntime& operator=(const HidRuntime&) = delete;

    bool initialized() const { return m_initialized; }

private:
    bool m_initialized{};
};

HidRuntime& hidRuntime()
{
    static HidRuntime runtime;
    return runtime;
}
#endif

class MergedBackend final : public Backend {
public:
    explicit MergedBackend(std::vector<std::unique_ptr<Backend>> backends)
        : m_backends{std::move(backends)}
    {
    }

    std::expected<std::vector<Device>, BackendError> discover() override
    {
        std::vector<Device> mergedDevices;
        m_routes.clear();
        for (std::size_t backendIndex{0}; backendIndex < m_backends.size(); ++backendIndex) {
            auto devices{m_backends[backendIndex]->discover()};
            if (!devices.has_value()) {
                continue;
            }
            for (Device& device : *devices) {
                if (std::ranges::find(mergedDevices, device.name, &Device::name) !=
                    mergedDevices.end()) {
                    continue;
                }
                m_routes.push_back(Route{backendIndex, device.id});
                device.id = static_cast<std::uint32_t>(m_routes.size() - 1);
                mergedDevices.push_back(std::move(device));
            }
        }
        if (m_routes.empty()) {
            return std::unexpected{BackendError::unavailable};
        }
        return mergedDevices;
    }

    std::expected<Device, BackendError> changeDeviceColor(std::uint32_t deviceId,
                                                          Color color) override
    {
        return routeOperation(deviceId, [&](Backend& backend, std::uint32_t backendDeviceId) {
            return backend.changeDeviceColor(backendDeviceId, color);
        });
    }

    std::expected<Device, BackendError> changeZoneColor(std::uint32_t deviceId,
                                                        std::uint32_t zoneId, Color color) override
    {
        return routeOperation(deviceId, [&](Backend& backend, std::uint32_t backendDeviceId) {
            return backend.changeZoneColor(backendDeviceId, zoneId, color);
        });
    }

    std::expected<Device, BackendError> resizeZone(std::uint32_t deviceId, std::uint32_t zoneId,
                                                   int size) override
    {
        return routeOperation(deviceId, [&](Backend& backend, std::uint32_t backendDeviceId) {
            return backend.resizeZone(backendDeviceId, zoneId, size);
        });
    }

    std::expected<Device, BackendError> changeMode(std::uint32_t deviceId,
                                                    std::string_view mode) override
    {
        return routeOperation(deviceId, [&](Backend& backend, std::uint32_t backendDeviceId) {
            return backend.changeMode(backendDeviceId, mode);
        });
    }

private:
    struct Route {
        std::size_t backendIndex{};
        std::uint32_t backendDeviceId{};
    };

    std::expected<Device, BackendError> routeOperation(
        std::uint32_t deviceId,
        const std::function<
            std::expected<Device, BackendError>(Backend&, std::uint32_t)>& operation)
    {
        if (deviceId >= m_routes.size()) {
            return std::unexpected{BackendError::notFound};
        }
        const Route route{m_routes[deviceId]};
        auto device{
            operation(*m_backends[route.backendIndex], route.backendDeviceId)};
        if (device.has_value()) {
            device->id = deviceId;
        }
        return device;
    }

    std::vector<std::unique_ptr<Backend>> m_backends;
    std::vector<Route> m_routes;
};

}

std::unique_ptr<Backend> makeHardwareBackend()
{
#if defined(_WIN32)
    if (!hidRuntime().initialized()) {
        return nullptr;
    }
    return makeMergedBackend(detectDriverBackends());
#else
    return nullptr;
#endif
}

std::unique_ptr<Backend> makeMergedBackend(std::vector<std::unique_ptr<Backend>> backends)
{
    std::erase_if(backends, [](const std::unique_ptr<Backend>& backend) {
        return backend == nullptr;
    });
    if (backends.empty()) {
        return nullptr;
    }
    if (backends.size() == 1) {
        return std::move(backends.front());
    }
    return std::make_unique<MergedBackend>(std::move(backends));
}

}
