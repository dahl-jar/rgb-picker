#include "support/test_harness.h"

#include "rgbpicker/backend.h"

#include <string>
#include <vector>

namespace {

using rgbpicker::DeviceType;
using rgbpicker::Mode;

TEST(should_map_known_openrgb_device_type_numbers)
{
    EXPECT_TRUE(rgbpicker::deviceTypeFromOpenRgbValue(0) == DeviceType::motherboard);
    EXPECT_TRUE(rgbpicker::deviceTypeFromOpenRgbValue(5) == DeviceType::keyboard);
    EXPECT_TRUE(rgbpicker::deviceTypeFromOpenRgbValue(6) == DeviceType::mouse);
}

TEST(should_map_unknown_device_type_numbers_to_other)
{
    EXPECT_TRUE(rgbpicker::deviceTypeFromOpenRgbValue(19) == DeviceType::other);
    EXPECT_TRUE(rgbpicker::deviceTypeFromOpenRgbValue(4096) == DeviceType::other);
}

TEST(should_keep_a_mode_that_already_accepts_colors)
{
    const std::vector<Mode> modes{Mode{"Direct", true}, Mode{"Rainbow Wave", false}};

    EXPECT_TRUE(!rgbpicker::chooseDirectMode(modes, "Direct").has_value());
}

TEST(should_prefer_direct_over_other_color_modes)
{
    const std::vector<Mode> modes{Mode{"Spectrum Cycle", false}, Mode{"Direct", true},
                                  Mode{"Static", true}};

    const auto chosenMode{rgbpicker::chooseDirectMode(modes, "Spectrum Cycle")};

    EXPECT_TRUE(chosenMode.has_value());
    EXPECT_EQ(*chosenMode, std::string{"Direct"});
}

TEST(should_fall_back_to_static_when_direct_is_unavailable)
{
    const std::vector<Mode> modes{Mode{"Breathing", false}, Mode{"Static", true}};

    const auto chosenMode{rgbpicker::chooseDirectMode(modes, "Breathing")};

    EXPECT_TRUE(chosenMode.has_value());
    EXPECT_EQ(*chosenMode, std::string{"Static"});
}

TEST(should_use_an_unnamed_color_mode_as_a_last_resort)
{
    const std::vector<Mode> modes{Mode{"Wave", false}, Mode{"Per-key", true}};

    const auto chosenMode{rgbpicker::chooseDirectMode(modes, "Wave")};

    EXPECT_TRUE(chosenMode.has_value());
    EXPECT_EQ(*chosenMode, std::string{"Per-key"});
}

TEST(should_return_no_mode_when_none_accept_colors)
{
    const std::vector<Mode> modes{Mode{"Wave", false}, Mode{"Breathing", false}};

    EXPECT_TRUE(!rgbpicker::chooseDirectMode(modes, "Wave").has_value());
}

TEST(should_return_no_mode_for_an_empty_mode_list)
{
    EXPECT_TRUE(!rgbpicker::chooseDirectMode({}, "").has_value());
}

}
