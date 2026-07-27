#include "backend/hardware/driver_backend.h"

#include <algorithm>
#include <string>
#include <utility>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "storage/config_file.h"

#include "DetectionManager.h"
#include "RGBController.h"
#include "ResourceManager.h"
#include "i2c_smbus.h"
#include <hidapi.h>
#endif

namespace rgbpicker {

#if defined(_WIN32)

namespace {

Color fromRgbColor(RGBColor color)
{
    return Color{static_cast<std::uint8_t>(RGBGetRValue(color)),
                 static_cast<std::uint8_t>(RGBGetGValue(color)),
                 static_cast<std::uint8_t>(RGBGetBValue(color))};
}

RGBColor toRgbColor(Color color)
{
    return ToRGBColor(static_cast<unsigned int>(color.red), static_cast<unsigned int>(color.green),
                      static_cast<unsigned int>(color.blue));
}

class DriverBackend final : public Backend {
public:
    explicit DriverBackend(std::unique_ptr<RGBController> controller)
        : m_controller{std::move(controller)}
    {
    }

    DriverBackend(const DriverBackend&) = delete;
    DriverBackend& operator=(const DriverBackend&) = delete;

    std::expected<std::vector<Device>, BackendError> discover() override
    {
        return std::vector<Device>{snapshot()};
    }

    std::expected<Device, BackendError> changeDeviceColor(std::uint32_t deviceId,
                                                           Color color) override
    {
        if (deviceId != 0) {
            return std::unexpected{BackendError::notFound};
        }
        m_controller->SetAllColors(toRgbColor(color));
        m_controller->UpdateLEDs();
        return snapshot();
    }

    std::expected<Device, BackendError> changeZoneColor(std::uint32_t deviceId,
                                                         std::uint32_t zoneId, Color color) override
    {
        if (deviceId != 0 || zoneId >= m_controller->GetZoneCount()) {
            return std::unexpected{BackendError::notFound};
        }
        m_controller->SetAllZoneColors(static_cast<int>(zoneId), toRgbColor(color));
        m_controller->UpdateZoneLEDs(static_cast<int>(zoneId));
        return snapshot();
    }

    std::expected<Device, BackendError> resizeZone(std::uint32_t deviceId, std::uint32_t zoneId,
                                                    int size) override
    {
        if (deviceId != 0 || zoneId >= m_controller->GetZoneCount()) {
            return std::unexpected{BackendError::notFound};
        }
        const auto requested{static_cast<unsigned int>(std::max(size, 0))};
        if (requested < m_controller->GetZoneLEDsMin(zoneId) ||
            requested > m_controller->GetZoneLEDsMax(zoneId)) {
            return std::unexpected{BackendError::invalidArgument};
        }
        m_controller->ResizeZone(static_cast<int>(zoneId), size);
        m_controller->UpdateLEDs();
        ResourceManager::get()->GetSettingsManager()->SaveSettings();
        return snapshot();
    }

    std::expected<Device, BackendError> changeMode(std::uint32_t deviceId,
                                                    std::string_view mode) override
    {
        if (deviceId != 0) {
            return std::unexpected{BackendError::notFound};
        }
        const auto index{modeIndex(mode)};
        if (!index.has_value()) {
            return std::unexpected{BackendError::notFound};
        }
        m_controller->SetActiveMode(static_cast<int>(*index));
        m_controller->UpdateMode();
        return snapshot();
    }

private:
    std::optional<unsigned int> modeIndex(std::string_view name)
    {
        for (unsigned int index{0}; index < m_controller->GetModeCount(); ++index) {
            if (m_controller->GetModeName(index) == name) {
                return index;
            }
        }
        return std::nullopt;
    }

    bool takesColors(unsigned int mode)
    {
        return (m_controller->GetModeFlags(mode) & MODE_FLAG_HAS_PER_LED_COLOR) != 0;
    }

    std::vector<Mode> modes()
    {
        std::vector<Mode> listed;
        listed.reserve(m_controller->GetModeCount());
        for (unsigned int index{0}; index < m_controller->GetModeCount(); ++index) {
            listed.push_back(Mode{m_controller->GetModeName(index), takesColors(index)});
        }
        return listed;
    }

    std::vector<Zone> zones()
    {
        std::vector<Zone> listed;
        listed.reserve(m_controller->GetZoneCount());
        for (unsigned int index{0}; index < m_controller->GetZoneCount(); ++index) {
            const unsigned int count{m_controller->GetZoneLEDsCount(index)};
            listed.push_back(
                Zone{index, m_controller->GetZoneName(index), static_cast<int>(count),
                     static_cast<int>(m_controller->GetZoneLEDsMin(index)),
                     static_cast<int>(m_controller->GetZoneLEDsMax(index)), ZoneUnit::leds,
                     count == 0 ? Color{} : fromRgbColor(m_controller->GetZoneColor(index, 0))});
        }
        return listed;
    }

    std::string activeModeName()
    {
        const int active{m_controller->GetActiveMode()};
        if (active < 0 || static_cast<unsigned int>(active) >= m_controller->GetModeCount()) {
            return {};
        }
        return m_controller->GetModeName(static_cast<unsigned int>(active));
    }

    Device snapshot()
    {
        return Device{0,
                      m_controller->GetName(),
                      deviceTypeFromOpenRgbValue(
                          static_cast<std::uint32_t>(m_controller->GetDeviceType())),
                      zones(),
                      modes(),
                      activeModeName(),
                      m_controller->GetLEDCount() == 0
                          ? Color{}
                          : fromRgbColor(m_controller->GetColor(0))};
    }

    std::unique_ptr<RGBController> m_controller;
};

class DriverSettingsFile final : public SettingsStore {
public:
    std::string load() override { return config_file::read(path()); }

    void save(const std::string& settings) override
    {
        config_file::write(path(), settings);
    }

private:
    static std::filesystem::path path()
    {
        return config_file::directory() / "driver-settings.json";
    }
};

void useDriverSettingsFile()
{
    static DriverSettingsFile file;
    ResourceManager::get()->GetSettingsManager()->UseStore(&file);
}

DetectedControllers detectOverHid()
{
    DetectedControllers found;
    hid_device_info* const list{hid_enumerate(0, 0)};
    for (hid_device_info* entry{list}; entry != nullptr; entry = entry->next) {
        for (const HidDetectorBlock& block : DetectionManager::get()->hidDetectors()) {
            if (!block.matches(entry)) {
                continue;
            }
            const DetectedControllers made{block.function(entry, block.name)};
            found.insert(found.end(), made.begin(), made.end());
        }
    }
    hid_free_enumeration(list);
    return found;
}

DetectedControllers detectOverI2c()
{
    DetectedControllers found;
    DetectionManager::get()->findBuses();
    for (i2c_smbus_interface* const bus : DetectionManager::get()->buses()) {
        if (bus == nullptr) {
            continue;
        }
        for (const I2CPciDetectorBlock& block : DetectionManager::get()->i2cPciDetectors()) {
            if (bus->info.pci_vendor != block.ven_id || bus->info.pci_device != block.dev_id ||
                bus->info.pci_subsystem_vendor != block.subven_id ||
                bus->info.pci_subsystem_device != block.subdev_id) {
                continue;
            }
            const DetectedControllers made{block.function(bus, block.i2c_addr, block.name)};
            found.insert(found.end(), made.begin(), made.end());
        }
    }
    return found;
}

}

std::vector<std::unique_ptr<Backend>> detectDriverBackends()
{
    useDriverSettingsFile();

    DetectedControllers controllers{detectOverHid()};
    const DetectedControllers onI2c{detectOverI2c()};
    controllers.insert(controllers.end(), onI2c.begin(), onI2c.end());

    std::vector<std::unique_ptr<Backend>> backends;
    backends.reserve(controllers.size());
    for (RGBController* const controller : controllers) {
        if (controller != nullptr) {
            backends.push_back(
                std::make_unique<DriverBackend>(std::unique_ptr<RGBController>{controller}));
        }
    }
    ResourceManager::get()->GetSettingsManager()->SaveSettings();
    return backends;
}

#else

std::vector<std::unique_ptr<Backend>> detectDriverBackends()
{
    return {};
}

#endif

}
