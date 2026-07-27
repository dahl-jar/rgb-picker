#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace rgbpicker::config_file {

std::string_view trim(std::string_view value);

std::filesystem::path directory();

std::string read(const std::filesystem::path& path);

bool write(const std::filesystem::path& path, std::string_view text);

}
