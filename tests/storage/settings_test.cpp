#include "support/test_harness.h"

#include "rgbpicker/settings.h"

#include <string>

namespace {

using rgbpicker::Color;
using rgbpicker::Settings;

TEST(should_round_trip_settings)
{
    const Settings saved{true, false, 40, Color{32, 160, 240}, "Basic"};

    const Settings loaded{rgbpicker::parseSettings(rgbpicker::serializeSettings(saved))};

    EXPECT_TRUE(loaded == saved);
}

TEST(should_round_trip_a_profile_name_holding_the_separator)
{
    const Settings saved{false, true, 100, Color{}, "red=blue"};

    const Settings loaded{rgbpicker::parseSettings(rgbpicker::serializeSettings(saved))};

    EXPECT_EQ(loaded.activeProfile, std::string{"red=blue"});
}

TEST(should_forget_the_active_profile_when_the_file_has_none)
{
    const Settings loaded{rgbpicker::parseSettings("active-profile=\n")};

    EXPECT_TRUE(loaded.activeProfile.empty());
}

TEST(should_keep_defaults_for_absent_keys)
{
    const Settings loaded{rgbpicker::parseSettings("run-at-login=true\n")};

    EXPECT_EQ(loaded.runAtLogin, true);
    EXPECT_EQ(loaded.restoreColor, Settings{}.restoreColor);
    EXPECT_EQ(loaded.lastColor, Settings{}.lastColor);
}

TEST(should_keep_defaults_for_malformed_values)
{
    const Settings loaded{rgbpicker::parseSettings(
        "run-at-login=maybe\nlast-color=chartreuse\nbrightness=400\nrogue-key=1\n")};

    EXPECT_EQ(loaded.runAtLogin, Settings{}.runAtLogin);
    EXPECT_EQ(loaded.lastColor, Settings{}.lastColor);
    EXPECT_EQ(loaded.brightness, Settings{}.brightness);
}

TEST(should_ignore_blank_lines_and_comments)
{
    const Settings loaded{
        rgbpicker::parseSettings("\n# written by the app\n\nrestore-color=false\n")};

    EXPECT_EQ(loaded.restoreColor, false);
}

}
