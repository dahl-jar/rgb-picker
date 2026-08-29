#include "support/fixtures.h"
#include "support/test_harness.h"

#include "rgbpicker/cli.h"

#include <array>
#include <sstream>
#include <string>
#include <vector>

namespace {

using rgbpicker::Color;
using test_fixture::RecordingBackendFactory;

struct CliResult {
    int exitCode{};
    std::string output;
    std::string error;
};

CliResult invoke(std::vector<std::string> arguments, RecordingBackendFactory& factory)
{
    test_fixture::ManualClock clock;
    rgbpicker::CliEnvironment environment{factory, clock.reader(), clock.sleeper()};
    std::ostringstream output;
    std::ostringstream error;
    const int exitCode{rgbpicker::runCli(arguments, environment, output, error)};
    return CliResult{exitCode, output.str(), error.str()};
}

TEST(should_list_devices_zones_and_modes)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"list"}, factory)};

    EXPECT_EQ(result.exitCode, 0);
    EXPECT_CONTAINS(result.output, "[0] Lian Li Uni Hub - SL V2 v0.5");
    EXPECT_CONTAINS(result.output, "64 LEDs (min 0, max 96)");
    EXPECT_CONTAINS(result.output, "[1] Lian Li Uni Hub - SL");
    EXPECT_CONTAINS(result.output, "4 fans (min 0, max 4)");
    EXPECT_CONTAINS(result.output, "Static (active)");
    EXPECT_EQ(result.error, std::string{});
}

TEST(should_default_to_the_lian_filter)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"set", "purple"}, factory)};

    EXPECT_EQ(result.exitCode, 0);
    EXPECT_EQ(factory.state->calls.size(), std::size_t{2});
    EXPECT_EQ(factory.state->calls.at(0).operation, std::string{"change-device-color"});
    EXPECT_EQ(factory.state->calls.at(0).color, Color({160, 32, 240}));
    EXPECT_EQ(factory.state->calls.at(1).deviceId, std::uint32_t{1});
    EXPECT_EQ(result.output,
              std::string{"Set Lian Li Uni Hub - SL V2 v0.5 to purple\n"
                          "Set Lian Li Uni Hub - SL to purple\n"});
}

TEST(should_match_filter_case_insensitively)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"set", "#20a0f0", "SL V2"}, factory)};

    EXPECT_EQ(result.exitCode, 0);
    EXPECT_EQ(factory.state->calls.size(), std::size_t{1});
    EXPECT_EQ(factory.state->calls.at(0).deviceId, std::uint32_t{0});
    EXPECT_EQ(factory.state->calls.at(0).color, Color({32, 160, 240}));
}

TEST(should_forward_zone_color)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"zone", "0", "2", "cyan"}, factory)};

    EXPECT_EQ(result.exitCode, 0);
    EXPECT_EQ(factory.state->calls.size(), std::size_t{1});
    EXPECT_EQ(factory.state->calls.at(0).operation, std::string{"change-zone-color"});
    EXPECT_EQ(factory.state->calls.at(0).zoneId, std::uint32_t{2});
    EXPECT_EQ(factory.state->calls.at(0).color, Color({0, 255, 255}));
    EXPECT_EQ(result.output,
              std::string{"Set Lian Li Uni Hub - SL V2 v0.5 / Channel 3 to cyan\n"});
}

TEST(should_report_resized_zone)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"resize", "0", "1", "96"}, factory)};

    EXPECT_EQ(result.exitCode, 0);
    EXPECT_EQ(factory.state->calls.size(), std::size_t{1});
    EXPECT_EQ(factory.state->calls.at(0).operation, std::string{"resize-zone"});
    EXPECT_EQ(factory.state->calls.at(0).size, 96);
    EXPECT_EQ(result.output,
              std::string{"Resized Lian Li Uni Hub - SL V2 v0.5 / Channel 2 to 96 LEDs\n"});
}

TEST(should_forward_multiword_mode)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"mode", "1", "Rainbow Wave"}, factory)};

    EXPECT_EQ(result.exitCode, 0);
    EXPECT_EQ(factory.state->calls.size(), std::size_t{1});
    EXPECT_EQ(factory.state->calls.at(0).operation, std::string{"change-mode"});
    EXPECT_EQ(factory.state->calls.at(0).mode, std::string{"Rainbow Wave"});
    EXPECT_EQ(result.output,
              std::string{"Set Lian Li Uni Hub - SL mode to Rainbow Wave\n"});
}

TEST(should_reject_wrong_arity_before_connecting)
{
    const std::array<std::vector<std::string>, 5> commands{{
        {"set"},
        {"zone", "0", "1"},
        {"resize", "0", "1"},
        {"mode", "0"},
        {"list", "extra"},
    }};

    for (const auto& command : commands) {
        RecordingBackendFactory factory;

        const CliResult result{invoke(command, factory)};

        EXPECT_EQ(result.exitCode, 1);
        EXPECT_EQ(result.error,
                  std::string{"usage: rgb-ctl <command>\n"
                              "commands: list, set, zone, resize, mode, rainbow\n"});
        EXPECT_EQ(factory.hardwareCreations, 0);
    }
}

TEST(should_reject_nonnumeric_ids)
{
    struct Row {
        std::vector<std::string> command;
        std::string error;
    };
    const std::array<Row, 3> rows{{
        {{{"zone", "abc", "0", "red"}}, "invalid device ID: abc\n"},
        {{{"resize", "0", "1x", "4"}}, "invalid zone ID: 1x\n"},
        {{{"resize", "0", "1", "4x"}}, "invalid size: 4x\n"},
    }};

    for (const auto& row : rows) {
        RecordingBackendFactory factory;

        const CliResult result{invoke(row.command, factory)};

        EXPECT_EQ(result.exitCode, 1);
        EXPECT_EQ(result.error, row.error);
        EXPECT_TRUE(factory.state->calls.empty());
    }
}

TEST(should_reject_negative_ids)
{
    struct Row {
        std::vector<std::string> command;
        std::string error;
    };
    const std::array<Row, 2> rows{{
        {{{"zone", "-1", "0", "red"}}, "invalid device ID: -1\n"},
        {{{"zone", "0", "-1", "red"}}, "invalid zone ID: -1\n"},
    }};

    for (const auto& row : rows) {
        RecordingBackendFactory factory;

        const CliResult result{invoke(row.command, factory)};

        EXPECT_EQ(result.exitCode, 1);
        EXPECT_EQ(result.error, row.error);
        EXPECT_TRUE(factory.state->calls.empty());
    }
}

TEST(should_reject_unknown_color_before_connecting)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"set", "not-a-color"}, factory)};

    EXPECT_EQ(result.exitCode, 1);
    EXPECT_EQ(result.error, std::string{"unknown color: not-a-color\n"});
    EXPECT_EQ(factory.hardwareCreations, 0);
}

TEST(should_reject_unmatched_filter)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"set", "red", "missing"}, factory)};

    EXPECT_EQ(result.exitCode, 1);
    EXPECT_EQ(result.error, std::string{"No devices match filter: missing\n"});
    EXPECT_TRUE(factory.state->calls.empty());
}

TEST(should_reject_missing_device_argument)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"zone", "9", "0", "red"}, factory)};

    EXPECT_EQ(result.exitCode, 1);
    EXPECT_EQ(result.error, std::string{"Device not found: 9\n"});
    EXPECT_TRUE(factory.state->calls.empty());
}

TEST(should_reject_missing_zone_argument)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"zone", "0", "9", "red"}, factory)};

    EXPECT_EQ(result.exitCode, 1);
    EXPECT_EQ(result.error,
              std::string{"Zone not found on Lian Li Uni Hub - SL V2 v0.5: 9\n"});
    EXPECT_TRUE(factory.state->calls.empty());
}

TEST(should_reject_unsupported_mode)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"mode", "0", "Missing"}, factory)};

    EXPECT_EQ(result.exitCode, 1);
    EXPECT_EQ(result.error,
              std::string{"Unsupported mode for Lian Li Uni Hub - SL V2 v0.5: Missing\n"});
    EXPECT_TRUE(factory.state->calls.empty());
}

TEST(should_exit_2_when_hardware_is_unavailable)
{
    RecordingBackendFactory factory;
    factory.failHardwareCreation = true;

    const CliResult result{invoke({"list"}, factory)};

    EXPECT_EQ(result.exitCode, 2);
    EXPECT_EQ(result.error, std::string{"no supported RGB hardware found\n"});
}

TEST(should_exit_2_when_discovery_fails)
{
    RecordingBackendFactory factory;
    factory.state->discoveryFailureAfterCalls = 1;

    const CliResult result{invoke({"list"}, factory)};

    EXPECT_EQ(result.exitCode, 2);
    EXPECT_EQ(result.error, std::string{"device listing failed\n"});
}

TEST(should_exit_2_when_zone_color_fails)
{
    RecordingBackendFactory factory;
    factory.state->failMutation = true;

    const CliResult result{invoke({"zone", "0", "0", "red"}, factory)};

    EXPECT_EQ(result.exitCode, 2);
    EXPECT_EQ(result.error,
              std::string{"zone color operation failed: device 0, zone 0\n"});
}

TEST(should_reject_removed_simulate_option)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"--simulate", "list"}, factory)};

    EXPECT_EQ(result.exitCode, 1);
    EXPECT_EQ(result.error,
              std::string{"usage: rgb-ctl <command>\n"
                          "commands: list, set, zone, resize, mode, rainbow\n"});
    EXPECT_EQ(factory.hardwareCreations, 0);
}

TEST(should_use_hardware_backend_by_default)
{
    RecordingBackendFactory factory;

    const CliResult result{invoke({"list"}, factory)};

    EXPECT_EQ(result.exitCode, 0);
    EXPECT_EQ(factory.hardwareCreations, 1);
    EXPECT_CONTAINS(result.output, "Lian Li Uni Hub");
}

}
