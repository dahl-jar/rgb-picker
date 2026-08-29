#include "support/fixtures.h"
#include "support/test_harness.h"

#include "rgbpicker/backend_session.h"

#include <chrono>

namespace {

using rgbpicker::BackendError;
using rgbpicker::BackendSession;
using rgbpicker::BackendSessionConfig;
using rgbpicker::BackendSessionPhase;
using std::chrono::milliseconds;
using test_fixture::ManualClock;
using test_fixture::RecordingBackendFactory;

TEST(should_open_hardware_on_first_poll)
{
    RecordingBackendFactory factory;
    ManualClock clock;
    BackendSession session{factory, BackendSessionConfig{}, clock.reader()};

    const rgbpicker::Backend* const backend{session.poll()};

    EXPECT_TRUE(backend != nullptr);
    EXPECT_EQ(session.status().phase, BackendSessionPhase::ready);
    EXPECT_EQ(factory.hardwareCreations, 1);
}

TEST(should_retry_when_hardware_is_unavailable)
{
    RecordingBackendFactory factory;
    factory.failHardwareCreation = true;
    ManualClock clock;
    const BackendSessionConfig config;
    BackendSession session{factory, config, clock.reader()};

    EXPECT_TRUE(session.poll() == nullptr);
    EXPECT_EQ(session.status().phase, BackendSessionPhase::waiting);

    session.poll();
    EXPECT_EQ(factory.hardwareCreations, 1);

    factory.failHardwareCreation = false;
    clock.advance(config.firstRetryDelay);
    EXPECT_TRUE(session.poll() != nullptr);
    EXPECT_EQ(factory.hardwareCreations, 2);
}

TEST(should_wait_for_a_nonempty_device_list)
{
    RecordingBackendFactory factory;
    factory.state->devices.clear();
    ManualClock clock;
    const BackendSessionConfig config;
    BackendSession session{factory, config, clock.reader()};

    EXPECT_TRUE(session.poll() == nullptr);
    EXPECT_EQ(session.status().phase, BackendSessionPhase::waiting);

    factory.state->devices = test_fixture::primaryDevices();
    clock.advance(config.firstRetryDelay);

    EXPECT_TRUE(session.poll() != nullptr);
    EXPECT_EQ(session.status().phase, BackendSessionPhase::ready);
}

TEST(should_recreate_after_hardware_failure)
{
    RecordingBackendFactory factory;
    ManualClock clock;
    const BackendSessionConfig config;
    BackendSession session{factory, config, clock.reader()};
    session.poll();

    session.reportFailure(BackendError::unavailable);

    EXPECT_TRUE(session.backend() == nullptr);
    clock.advance(config.firstRetryDelay);
    EXPECT_TRUE(session.poll() != nullptr);
    EXPECT_EQ(factory.hardwareCreations, 2);
}

TEST(should_recreate_when_requested)
{
    RecordingBackendFactory factory;
    ManualClock clock;
    const BackendSessionConfig config;
    BackendSession session{factory, config, clock.reader()};
    session.poll();

    session.recreate();

    EXPECT_TRUE(session.backend() == nullptr);
    clock.advance(config.firstRetryDelay);
    EXPECT_TRUE(session.poll() != nullptr);
    EXPECT_EQ(factory.hardwareCreations, 2);
}

TEST(should_keep_backend_after_request_validation_failure)
{
    RecordingBackendFactory factory;
    ManualClock clock;
    BackendSession session{factory, BackendSessionConfig{}, clock.reader()};
    session.poll();

    session.reportFailure(BackendError::notFound);

    EXPECT_TRUE(session.backend() != nullptr);
    EXPECT_EQ(session.status().phase, BackendSessionPhase::ready);
    EXPECT_EQ(factory.hardwareCreations, 1);
}

TEST(should_double_retry_delay_up_to_ceiling)
{
    BackendSessionConfig config;
    config.firstRetryDelay = milliseconds{500};
    config.maxRetryDelay = milliseconds{4000};

    EXPECT_EQ(rgbpicker::retryDelay(config, 1), milliseconds{500});
    EXPECT_EQ(rgbpicker::retryDelay(config, 2), milliseconds{1000});
    EXPECT_EQ(rgbpicker::retryDelay(config, 3), milliseconds{2000});
    EXPECT_EQ(rgbpicker::retryDelay(config, 4), milliseconds{4000});
    EXPECT_EQ(rgbpicker::retryDelay(config, 30), milliseconds{4000});
}

TEST(should_stop_waiting_when_budget_expires)
{
    RecordingBackendFactory factory;
    factory.failHardwareCreation = true;
    ManualClock clock;
    BackendSession session{factory, BackendSessionConfig{}, clock.reader()};

    const rgbpicker::Backend* const backend{
        session.waitUntilReady(milliseconds{3000}, clock.sleeper())};

    EXPECT_TRUE(backend == nullptr);
    EXPECT_EQ(session.status().phase, BackendSessionPhase::waiting);
}

}
