/*---------------------------------------------------------*\
| DetectionManager.h                                        |
|                                                           |
|   Collects the detector registrations used by rgb-picker. |
|   OpenRGB driver files keep their registration macros,    |
|   while this shim retains the HID, PCI and bus entries    |
|   used by the embedded backend.                           |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#pragma once

#include <cstdio>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <hidapi.h>

class i2c_smbus_interface;
class RGBController;
class SPDWrapper;

typedef std::vector<RGBController*> DetectedControllers;

typedef std::function<bool()> I2CBusDetectorFunction;
typedef std::function<DetectedControllers()> DeviceDetectorFunction;
typedef std::function<DetectedControllers(std::vector<i2c_smbus_interface*>&)>
    I2CDeviceDetectorFunction;
typedef std::function<DetectedControllers(i2c_smbus_interface*, std::vector<SPDWrapper*>&,
                                          const std::string&)>
    I2CDRAMDeviceDetectorFunction;
typedef std::function<DetectedControllers(i2c_smbus_interface*, uint8_t, const std::string&)>
    I2CPCIDeviceDetectorFunction;
typedef std::function<DetectedControllers(hid_device_info*, const std::string&)>
    HIDDeviceDetectorFunction;
typedef std::function<void()> DynamicDetectorFunction;
typedef std::function<void()> PreDetectionHookFunction;

#define HID_PID_ANY                     -1
#define HID_VID_ANY                     -1
#define HID_INTERFACE_ANY               -1
#define HID_USAGE_ANY                   -1
#define HID_USAGE_PAGE_ANY              -1

struct HidDetectorBlock {
    std::string name;
    HIDDeviceDetectorFunction function;
    int vid{HID_VID_ANY};
    int pid{HID_PID_ANY};
    int interface{HID_INTERFACE_ANY};
    int usage_page{HID_USAGE_PAGE_ANY};
    int usage{HID_USAGE_ANY};

    bool matches(hid_device_info* info) const
    {
        return (vid == HID_VID_ANY || vid == info->vendor_id) &&
               (pid == HID_PID_ANY || pid == info->product_id) &&
               (usage_page == HID_USAGE_PAGE_ANY || usage_page == info->usage_page) &&
               (usage == HID_USAGE_ANY || usage == info->usage) &&
               (interface == HID_INTERFACE_ANY || interface == info->interface_number);
    }
};

struct I2CPciDetectorBlock {
    std::string name;
    I2CPCIDeviceDetectorFunction function;
    uint16_t ven_id{};
    uint16_t dev_id{};
    uint16_t subven_id{};
    uint16_t subdev_id{};
    uint8_t i2c_addr{};
};

/**
 * Stores detector registrations required by the embedded driver set.
 * Registration types outside that set are ignored.
 */
class DetectionManager {
public:
    static DetectionManager* get()
    {
        static DetectionManager instance;
        return &instance;
    }

    void RegisterI2CBus(i2c_smbus_interface* bus) { m_buses.push_back(bus); }

    void RegisterHIDDeviceDetector(HidDetectorBlock block)
    {
        m_hidDetectors.push_back(std::move(block));
    }

    void RegisterI2CPCIDeviceDetector(I2CPciDetectorBlock block)
    {
        m_i2cPciDetectors.push_back(std::move(block));
    }

    void RegisterI2CBusDetector(I2CBusDetectorFunction detector)
    {
        m_busDetectors.push_back(std::move(detector));
    }

    void RegisterDeviceDetector(std::string, DeviceDetectorFunction) {}
    void RegisterI2CDeviceDetector(std::string, I2CDeviceDetectorFunction) {}
    void RegisterI2CDRAMDeviceDetector(std::string, I2CDRAMDeviceDetectorFunction, uint16_t,
                                       uint8_t)
    {
    }
    void RegisterDynamicDetector(std::string, DynamicDetectorFunction) {}
    void RegisterPreDetectionHook(PreDetectionHookFunction) {}

    /** Runs every registered bus detector once. */
    void findBuses()
    {
        if (m_busesFound) {
            return;
        }
        m_busesFound = true;
        for (const I2CBusDetectorFunction& detector : m_busDetectors) {
            detector();
        }
    }

    const std::vector<i2c_smbus_interface*>& buses() const { return m_buses; }
    const std::vector<HidDetectorBlock>& hidDetectors() const { return m_hidDetectors; }
    const std::vector<I2CPciDetectorBlock>& i2cPciDetectors() const { return m_i2cPciDetectors; }

private:
    std::vector<i2c_smbus_interface*> m_buses;
    std::vector<I2CBusDetectorFunction> m_busDetectors;
    bool m_busesFound{false};
    std::vector<HidDetectorBlock> m_hidDetectors;
    std::vector<I2CPciDetectorBlock> m_i2cPciDetectors;
};

class HidDeviceDetectorRegistration {
public:
    explicit HidDeviceDetectorRegistration(HidDetectorBlock block)
    {
        DetectionManager::get()->RegisterHIDDeviceDetector(std::move(block));
    }
};

class I2CPciDeviceDetectorRegistration {
public:
    explicit I2CPciDeviceDetectorRegistration(I2CPciDetectorBlock block)
    {
        DetectionManager::get()->RegisterI2CPCIDeviceDetector(std::move(block));
    }
};

class I2CBusDetectorRegistration {
public:
    explicit I2CBusDetectorRegistration(I2CBusDetectorFunction detector)
    {
        DetectionManager::get()->RegisterI2CBusDetector(std::move(detector));
    }
};

#define RGBPICKER_DISCARD_REGISTRATION(tag, expression) \
    static_assert(sizeof(&expression) > 0, #tag)

#define REGISTER_DETECTOR(name, func)                                                              \
    RGBPICKER_DISCARD_REGISTRATION(func, func)
#define REGISTER_I2C_DETECTOR(name, func)                                                          \
    RGBPICKER_DISCARD_REGISTRATION(func, func)
#define REGISTER_I2C_DRAM_DETECTOR(name, func, jedec_id, dram_type)                                \
    RGBPICKER_DISCARD_REGISTRATION(func, func)
#define REGISTER_I2C_BUS_DETECTOR(func)                                                            \
    static I2CBusDetectorRegistration device_detector_obj_##func(func)
#define REGISTER_DYNAMIC_DETECTOR(name, func) RGBPICKER_DISCARD_REGISTRATION(func, func)
#define REGISTER_PRE_DETECTION_HOOK(func) RGBPICKER_DISCARD_REGISTRATION(func, func)
#define REGISTER_HID_WRAPPED_DETECTOR(name, func, vid, pid)                                        \
    RGBPICKER_DISCARD_REGISTRATION(func, func)
#define REGISTER_HID_WRAPPED_DETECTOR_IPU(name, func, vid, pid, interface, page, usage)            \
    RGBPICKER_DISCARD_REGISTRATION(func, func)

#define REGISTER_I2C_PCI_DETECTOR(name, func, ven, dev, subven, subdev, addr)                      \
    static I2CPciDeviceDetectorRegistration                                                        \
        device_detector_obj_##ven##dev##subven##subdev##addr##func(                                \
            I2CPciDetectorBlock{name, func, ven, dev, subven, subdev, addr})

#define REGISTER_HID_DETECTOR(name, func, vid, pid)                                                \
    static HidDeviceDetectorRegistration device_detector_obj_##vid##pid(HidDetectorBlock{          \
        name, func, vid, pid, HID_INTERFACE_ANY, HID_USAGE_PAGE_ANY, HID_USAGE_ANY})
#define REGISTER_HID_DETECTOR_I(name, func, vid, pid, interface)                                   \
    static HidDeviceDetectorRegistration device_detector_obj_##vid##pid##_##interface(             \
        HidDetectorBlock{name, func, vid, pid, interface, HID_USAGE_PAGE_ANY, HID_USAGE_ANY})
#define REGISTER_HID_DETECTOR_IP(name, func, vid, pid, interface, page)                            \
    static HidDeviceDetectorRegistration device_detector_obj_##vid##pid##_##interface##_##page(    \
        HidDetectorBlock{name, func, vid, pid, interface, page, HID_USAGE_ANY})
#define REGISTER_HID_DETECTOR_IPU(name, func, vid, pid, interface, page, usage)                    \
    static HidDeviceDetectorRegistration                                                           \
        device_detector_obj_##vid##pid##_##interface##_##page##_##usage(                           \
            HidDetectorBlock{name, func, vid, pid, interface, page, usage})
#define REGISTER_HID_DETECTOR_P(name, func, vid, pid, page)                                        \
    static HidDeviceDetectorRegistration device_detector_obj_##vid##pid##__##page(                 \
        HidDetectorBlock{name, func, vid, pid, HID_INTERFACE_ANY, page, HID_USAGE_ANY})
#define REGISTER_HID_DETECTOR_PU(name, func, vid, pid, page, usage)                                \
    static HidDeviceDetectorRegistration device_detector_obj_##vid##pid##__##page##_##usage(       \
        HidDetectorBlock{name, func, vid, pid, HID_INTERFACE_ANY, page, usage})
#define REGISTER_HID_DETECTOR_I_ONLY(name, func, interface)                                        \
    static HidDeviceDetectorRegistration device_detector_obj_##interface(HidDetectorBlock{         \
        name, func, HID_VID_ANY, HID_PID_ANY, interface, HID_USAGE_PAGE_ANY, HID_USAGE_ANY})
#define REGISTER_HID_DETECTOR_IP_ONLY(name, func, interface, page)                                 \
    static HidDeviceDetectorRegistration device_detector_obj_##interface##_##page(                 \
        HidDetectorBlock{name, func, HID_VID_ANY, HID_PID_ANY, interface, page, HID_USAGE_ANY})
