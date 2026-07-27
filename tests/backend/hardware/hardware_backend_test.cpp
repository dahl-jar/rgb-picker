#include "support/test_harness.h"

#include "backend/hardware/hardware_backend.h"
#include "backend/simulation/simulation_backend.h"

#include <memory>
#include <utility>
#include <vector>

namespace {

TEST(should_return_one_backend_without_wrapping_it)
{
    std::vector<std::unique_ptr<rgbpicker::Backend>> backends;
    backends.push_back(std::make_unique<rgbpicker::SimulationBackend>());
    rgbpicker::Backend* const originalBackend{backends.front().get()};

    const auto mergedBackend{rgbpicker::makeMergedBackend(std::move(backends))};

    EXPECT_TRUE(mergedBackend.get() == originalBackend);
}

TEST(should_remove_devices_claimed_by_multiple_backends)
{
    std::vector<std::unique_ptr<rgbpicker::Backend>> backends;
    backends.push_back(std::make_unique<rgbpicker::SimulationBackend>());
    backends.push_back(std::make_unique<rgbpicker::SimulationBackend>());

    const auto mergedBackend{rgbpicker::makeMergedBackend(std::move(backends))};
    const auto devices{mergedBackend->discover()};

    EXPECT_TRUE(devices.has_value());
    const auto expectedDevices{rgbpicker::SimulationBackend{}.discover()};
    EXPECT_EQ(devices->size(), expectedDevices->size());
}

TEST(should_return_null_when_no_backend_is_available)
{
    EXPECT_TRUE(rgbpicker::makeMergedBackend({}) == nullptr);
}

}
