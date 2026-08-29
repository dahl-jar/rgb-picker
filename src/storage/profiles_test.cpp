#include "support/test_harness.h"

#include "rgbpicker/profiles.h"

#include <string>
#include <vector>

namespace {

using rgbpicker::Color;
using rgbpicker::DeviceColor;
using rgbpicker::Profile;

std::vector<Profile> twoProfiles()
{
    return {Profile{"Gaming",
                    {DeviceColor{"Lian Li Uni Hub - SL", Color{255, 0, 0}},
                     DeviceColor{"Lian Li Uni Hub - SL V2 v0.5", Color{32, 160, 240}}}},
            Profile{"Chill", {DeviceColor{"Lian Li Uni Hub - SL", Color{255, 190, 120}}}}};
}

TEST(should_round_trip_profiles)
{
    const std::vector<Profile> saved{twoProfiles()};

    const std::vector<Profile> loaded{
        rgbpicker::parseProfiles(rgbpicker::serializeProfiles(saved))};

    EXPECT_TRUE(loaded == saved);
}

TEST(should_keep_device_names_containing_spaces_and_dashes)
{
    const std::vector<Profile> loaded{
        rgbpicker::parseProfiles("[Gaming]\nLian Li Uni Hub - SL=#ff0000\n")};

    EXPECT_EQ(loaded.size(), std::size_t{1});
    EXPECT_EQ(loaded.at(0).devices.at(0).device, std::string{"Lian Li Uni Hub - SL"});
    EXPECT_EQ(loaded.at(0).devices.at(0).color, Color({255, 0, 0}));
}

TEST(should_skip_entries_with_an_unreadable_color)
{
    const std::vector<Profile> loaded{
        rgbpicker::parseProfiles("[Gaming]\nGood=#00ff00\nBad=#12xz90\n")};

    EXPECT_EQ(loaded.at(0).devices.size(), std::size_t{1});
    EXPECT_EQ(loaded.at(0).devices.at(0).device, std::string{"Good"});
}

TEST(should_ignore_device_lines_before_any_profile)
{
    const std::vector<Profile> loaded{
        rgbpicker::parseProfiles("Orphan=#ff0000\n[Gaming]\nA=#ff0000\n")};

    EXPECT_EQ(loaded.size(), std::size_t{1});
    EXPECT_EQ(loaded.at(0).name, std::string{"Gaming"});
    EXPECT_EQ(loaded.at(0).devices.size(), std::size_t{1});
}

TEST(should_replace_a_profile_saved_under_an_existing_name)
{
    std::vector<Profile> profiles{twoProfiles()};

    rgbpicker::storeProfile(profiles, Profile{"Gaming", {DeviceColor{"A", Color{1, 2, 3}}}});

    EXPECT_EQ(profiles.size(), std::size_t{2});
    EXPECT_EQ(profiles.at(0).devices.size(), std::size_t{1});
    EXPECT_EQ(profiles.at(0).devices.at(0).color, Color({1, 2, 3}));
}

TEST(should_append_a_profile_saved_under_a_new_name)
{
    std::vector<Profile> profiles{twoProfiles()};

    rgbpicker::storeProfile(profiles, Profile{"Work", {DeviceColor{"A", Color{1, 2, 3}}}});

    EXPECT_EQ(profiles.size(), std::size_t{3});
    EXPECT_EQ(profiles.at(2).name, std::string{"Work"});
}

TEST(should_remove_a_profile_by_name)
{
    std::vector<Profile> profiles{twoProfiles()};

    rgbpicker::removeProfile(profiles, "Gaming");

    EXPECT_EQ(profiles.size(), std::size_t{1});
    EXPECT_EQ(profiles.at(0).name, std::string{"Chill"});
}

TEST(should_round_trip_the_applied_look)
{
    const std::vector<DeviceColor> saved{
        DeviceColor{"B850 AORUS ELITE WIFI7 ICE", Color{255, 0, 0}},
        DeviceColor{"Lian Li Uni Hub - SL V2 v0.5", Color{32, 160, 240}}};

    const std::vector<DeviceColor> loaded{
        rgbpicker::parseApplied(rgbpicker::serializeApplied(saved))};

    EXPECT_TRUE(loaded == saved);
}

TEST(should_write_the_applied_look_in_device_order)
{
    const std::vector<DeviceColor> unordered{DeviceColor{"Zone B", Color{1, 2, 3}},
                                             DeviceColor{"Alpha", Color{4, 5, 6}}};

    const std::vector<DeviceColor> loaded{
        rgbpicker::parseApplied(rgbpicker::serializeApplied(unordered))};

    EXPECT_EQ(loaded.at(0).device, std::string{"Alpha"});
    EXPECT_EQ(loaded.at(1).device, std::string{"Zone B"});
}

TEST(should_match_looks_recorded_in_different_orders)
{
    const std::vector<DeviceColor> profile{DeviceColor{"Alpha", Color{1, 2, 3}},
                                           DeviceColor{"Zone B", Color{4, 5, 6}}};
    const std::vector<DeviceColor> lit{DeviceColor{"Zone B", Color{4, 5, 6}},
                                       DeviceColor{"Alpha", Color{1, 2, 3}}};

    EXPECT_TRUE(rgbpicker::sameLook(profile, lit));
}

TEST(should_tell_looks_apart_when_one_device_changed_color)
{
    const std::vector<DeviceColor> profile{DeviceColor{"Alpha", Color{1, 2, 3}},
                                           DeviceColor{"Zone B", Color{4, 5, 6}}};
    const std::vector<DeviceColor> lit{DeviceColor{"Alpha", Color{1, 2, 3}},
                                       DeviceColor{"Zone B", Color{255, 0, 0}}};

    EXPECT_TRUE(!rgbpicker::sameLook(profile, lit));
}

TEST(should_tell_looks_apart_when_one_covers_more_devices)
{
    const std::vector<DeviceColor> profile{DeviceColor{"Alpha", Color{1, 2, 3}}};
    const std::vector<DeviceColor> lit{DeviceColor{"Alpha", Color{1, 2, 3}},
                                       DeviceColor{"Zone B", Color{4, 5, 6}}};

    EXPECT_TRUE(!rgbpicker::sameLook(profile, lit));
}

TEST(should_read_an_empty_look_from_a_missing_file)
{
    const std::vector<DeviceColor> loaded{rgbpicker::parseApplied("")};

    EXPECT_TRUE(loaded.empty());
}

TEST(should_round_trip_zone_sizes_through_the_layout_file)
{
    const std::vector<rgbpicker::ZoneSize> layout{
        rgbpicker::ZoneSize{"Board", "ARGB 1", 64},
        rgbpicker::ZoneSize{"Board", "ARGB 2", 30},
        rgbpicker::ZoneSize{"Hub", "Channel 1", 96}};

    const std::vector<rgbpicker::ZoneSize> loaded{
        rgbpicker::parseLayout(rgbpicker::serializeLayout(layout))};

    EXPECT_EQ(loaded.size(), std::size_t{3});
    EXPECT_EQ(loaded[0].device, std::string{"Board"});
    EXPECT_EQ(loaded[0].zone, std::string{"ARGB 1"});
    EXPECT_EQ(loaded[0].size, 64);
    EXPECT_EQ(loaded[2].device, std::string{"Hub"});
    EXPECT_EQ(loaded[2].size, 96);
}

TEST(should_keep_a_zone_of_no_leds_rather_than_treat_it_as_unset)
{
    const std::vector<rgbpicker::ZoneSize> layout{rgbpicker::ZoneSize{"Board", "ARGB 3", 0}};

    const std::vector<rgbpicker::ZoneSize> loaded{
        rgbpicker::parseLayout(rgbpicker::serializeLayout(layout))};

    EXPECT_EQ(loaded.size(), std::size_t{1});
    EXPECT_EQ(loaded[0].size, 0);
}

TEST(should_skip_a_zone_size_that_will_not_parse)
{
    const std::vector<rgbpicker::ZoneSize> loaded{
        rgbpicker::parseLayout("[Board]\nARGB 1=sixty\nARGB 2=48\nARGB 3=-4\n")};

    EXPECT_EQ(loaded.size(), std::size_t{1});
    EXPECT_EQ(loaded[0].zone, std::string{"ARGB 2"});
    EXPECT_EQ(loaded[0].size, 48);
}

TEST(should_replace_a_zone_size_rather_than_add_a_second)
{
    std::vector<rgbpicker::ZoneSize> layout{rgbpicker::ZoneSize{"Board", "ARGB 1", 64}};

    rgbpicker::storeZoneSize(layout, rgbpicker::ZoneSize{"Board", "ARGB 1", 30});
    rgbpicker::storeZoneSize(layout, rgbpicker::ZoneSize{"Board", "ARGB 2", 12});

    EXPECT_EQ(layout.size(), std::size_t{2});
    EXPECT_EQ(layout[0].size, 30);
    EXPECT_EQ(layout[1].zone, std::string{"ARGB 2"});
}

TEST(should_read_an_empty_layout_from_a_missing_file)
{
    const std::vector<rgbpicker::ZoneSize> loaded{rgbpicker::parseLayout("")};

    EXPECT_TRUE(loaded.empty());
}

}
