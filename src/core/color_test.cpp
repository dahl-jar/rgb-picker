#include "support/test_harness.h"

#include "rgbpicker/color.h"

#include <array>
#include <string_view>

namespace {

using rgbpicker::Color;

TEST(should_parse_named_colors_case_insensitively)
{
    struct Row {
        std::string_view input;
        Color expected;
    };
    const std::array<Row, 4> rows{{{"red", {255, 0, 0}},
                                   {"PURPLE", {160, 32, 240}},
                                   {"off", {0, 0, 0}},
                                   {"black", {0, 0, 0}}}};

    for (const auto& row : rows) {
        const auto color{rgbpicker::parseColor(row.input)};

        EXPECT_TRUE(color.has_value());
        EXPECT_EQ(*color, row.expected);
    }
}

TEST(should_parse_hex_with_and_without_hash)
{
    const auto prefixed{rgbpicker::parseColor("#20a0f0")};
    const auto bare{rgbpicker::parseColor("20A0F0")};

    EXPECT_TRUE(prefixed.has_value());
    EXPECT_TRUE(bare.has_value());
    EXPECT_EQ(*prefixed, Color({32, 160, 240}));
    EXPECT_EQ(*bare, Color({32, 160, 240}));
}

TEST(should_reject_malformed_color)
{
    const std::array<std::string_view, 5> invalid{
        "chartreuse", "#12345", "#1234567", "#12xz90", ""};

    for (const auto input : invalid) {
        const auto color{rgbpicker::parseColor(input)};

        EXPECT_TRUE(!color.has_value());
    }
}

TEST(should_scale_channels_to_the_brightness_percentage)
{
    const Color full{200, 100, 50};

    EXPECT_EQ(rgbpicker::scaleBrightness(full, 100), full);
    EXPECT_EQ(rgbpicker::scaleBrightness(full, 50), Color({100, 50, 25}));
    EXPECT_EQ(rgbpicker::scaleBrightness(full, 0), Color({0, 0, 0}));
}

TEST(should_clamp_a_brightness_outside_the_percentage_range)
{
    const Color full{200, 100, 50};

    EXPECT_EQ(rgbpicker::scaleBrightness(full, 400), full);
    EXPECT_EQ(rgbpicker::scaleBrightness(full, -20), Color({0, 0, 0}));
}

}
