#include "support/test_harness.h"

#include "rgbpicker/backend.h"
#include "backend/simulation/simulation_backend.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace {

using rgbpicker::BackendError;
using rgbpicker::Color;
using rgbpicker::SimulationBackend;
using rgbpicker::ZoneUnit;

TEST(should_discover_both_hubs)
{
    SimulationBackend backend;

    const auto result{backend.discover()};

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->at(0).id, std::uint32_t{0});
    EXPECT_EQ(result->at(0).name, std::string{"Lian Li Uni Hub - SL V2 v0.5"});
    EXPECT_EQ(result->at(0).zones.size(), std::size_t{4});
    EXPECT_EQ(result->at(0).zones.at(0).unit, ZoneUnit::leds);
    EXPECT_EQ(result->at(0).zones.at(0).minSize, 0);
    EXPECT_EQ(result->at(0).zones.at(0).maxSize, 96);
    EXPECT_EQ(result->at(1).id, std::uint32_t{1});
    EXPECT_EQ(result->at(1).name, std::string{"Lian Li Uni Hub - SL"});
    EXPECT_EQ(result->at(1).zones.size(), std::size_t{4});
    EXPECT_EQ(result->at(1).zones.at(0).unit, ZoneUnit::fans);
    EXPECT_EQ(result->at(1).zones.at(0).minSize, 0);
    EXPECT_EQ(result->at(1).zones.at(0).maxSize, 4);
}

TEST(should_offer_a_peripheral_running_a_hardware_effect)
{
    SimulationBackend backend;

    const auto result{backend.discover()};

    EXPECT_TRUE(result.has_value());
    const auto keyboard{std::ranges::find(*result, rgbpicker::DeviceType::keyboard,
                                          &rgbpicker::Device::type)};
    EXPECT_TRUE(keyboard != result->end());
    EXPECT_EQ(keyboard->activeMode, std::string{"Spectrum Cycle"});
    const auto target{rgbpicker::chooseDirectMode(keyboard->modes, keyboard->activeMode)};
    EXPECT_TRUE(target.has_value());
    EXPECT_EQ(*target, std::string{"Direct"});
}

TEST(should_color_device_and_all_zones)
{
    SimulationBackend backend;
    const Color cyan{0, 255, 255};

    const auto changed{backend.changeDeviceColor(0, cyan)};

    EXPECT_TRUE(changed.has_value());
    EXPECT_EQ(changed->color, cyan);
    for (const auto& zone : changed->zones) {
        EXPECT_EQ(zone.color, cyan);
    }
    const auto refreshed{backend.discover()};
    EXPECT_EQ(refreshed->at(0).color, cyan);
}

TEST(should_color_one_zone_only)
{
    SimulationBackend backend;
    const Color purple{160, 32, 240};

    const auto changed{backend.changeZoneColor(0, 2, purple)};

    EXPECT_TRUE(changed.has_value());
    EXPECT_EQ(changed->zones.at(2).color, purple);
    EXPECT_EQ(changed->zones.at(1).color, Color({0, 0, 0}));
    const auto refreshed{backend.discover()};
    EXPECT_EQ(refreshed->at(0).zones.at(2).color, purple);
}

TEST(should_persist_zone_resize)
{
    struct Row {
        std::uint32_t deviceId;
        int size;
    };
    const std::array<Row, 4> rows{{{0, 0}, {0, 96}, {1, 0}, {1, 4}}};

    for (const auto& row : rows) {
        SimulationBackend backend;

        const auto changed{backend.resizeZone(row.deviceId, 0, row.size)};

        EXPECT_TRUE(changed.has_value());
        EXPECT_EQ(changed->zones.at(0).size, row.size);
        const auto refreshed{backend.discover()};
        EXPECT_EQ(refreshed->at(row.deviceId).zones.at(0).size, row.size);
    }
}

TEST(should_reject_out_of_range_resize)
{
    struct Row {
        std::uint32_t deviceId;
        int size;
        int originalSize;
    };
    const std::array<Row, 4> rows{{{0, -1, 64}, {0, 97, 64}, {1, -1, 4}, {1, 5, 4}}};

    for (const auto& row : rows) {
        SimulationBackend backend;

        const auto changed{backend.resizeZone(row.deviceId, 0, row.size)};

        EXPECT_TRUE(!changed.has_value());
        EXPECT_EQ(changed.error(), BackendError::invalidArgument);
        const auto refreshed{backend.discover()};
        EXPECT_EQ(refreshed->at(row.deviceId).zones.at(0).size, row.originalSize);
    }
}

TEST(should_persist_mode_change)
{
    SimulationBackend backend;

    const auto changed{backend.changeMode(1, "Rainbow Wave")};

    EXPECT_TRUE(changed.has_value());
    EXPECT_EQ(changed->activeMode, std::string{"Rainbow Wave"});
    const auto refreshed{backend.discover()};
    EXPECT_EQ(refreshed->at(1).activeMode, std::string{"Rainbow Wave"});
}

TEST(should_reject_missing_device)
{
    SimulationBackend backend;

    const auto result{backend.changeDeviceColor(9, Color{255, 0, 0})};

    EXPECT_TRUE(!result.has_value());
    EXPECT_EQ(result.error(), BackendError::notFound);
}

TEST(should_reject_missing_zone)
{
    SimulationBackend backend;

    const auto result{backend.changeZoneColor(0, 9, Color{255, 0, 0})};

    EXPECT_TRUE(!result.has_value());
    EXPECT_EQ(result.error(), BackendError::notFound);
}

TEST(should_reject_missing_mode)
{
    SimulationBackend backend;

    const auto result{backend.changeMode(0, "Missing")};

    EXPECT_TRUE(!result.has_value());
    EXPECT_EQ(result.error(), BackendError::notFound);
}

}
