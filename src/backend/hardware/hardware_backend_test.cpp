#include "support/fixtures.h"
#include "support/test_harness.h"

#include "backend/hardware/hardware_backend.h"

#include <memory>
#include <utility>
#include <vector>

namespace {

TEST(should_return_one_backend_without_wrapping_it)
{
    std::vector<std::unique_ptr<rgbpicker::Backend>> backends;
    backends.push_back(
        std::make_unique<test_fixture::RecordingBackend>(
            std::make_shared<test_fixture::BackendState>()));
    rgbpicker::Backend* const originalBackend{backends.front().get()};

    const auto mergedBackend{rgbpicker::makeMergedBackend(std::move(backends))};

    EXPECT_TRUE(mergedBackend.get() == originalBackend);
}

TEST(should_remove_devices_claimed_by_multiple_backends)
{
    std::vector<std::unique_ptr<rgbpicker::Backend>> backends;
    backends.push_back(
        std::make_unique<test_fixture::RecordingBackend>(
            std::make_shared<test_fixture::BackendState>()));
    backends.push_back(
        std::make_unique<test_fixture::RecordingBackend>(
            std::make_shared<test_fixture::BackendState>()));

    const auto mergedBackend{rgbpicker::makeMergedBackend(std::move(backends))};
    const auto devices{mergedBackend->discover()};

    EXPECT_TRUE(devices.has_value());
    EXPECT_EQ(devices->size(), test_fixture::primaryDevices().size());
}

TEST(should_return_null_when_no_backend_is_available)
{
    EXPECT_TRUE(rgbpicker::makeMergedBackend({}) == nullptr);
}

}
