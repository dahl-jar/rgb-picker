#include "storage/config_file.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

namespace rgbpicker::config_file {
namespace {

constexpr std::string_view appFolder{"rgb-picker"};

}

std::string_view trim(std::string_view value)
{
    const auto first{value.find_first_not_of(" \t\r")};
    if (first == std::string_view::npos) {
        return {};
    }
    return value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
}

std::filesystem::path directory()
{
    const char* const roaming{std::getenv("APPDATA")};
    const std::filesystem::path base{roaming == nullptr ? std::filesystem::path{"."}
                                                        : std::filesystem::path{roaming}};
    return base / appFolder;
}

std::string read(const std::filesystem::path& path)
{
    std::ifstream file{path};
    if (!file.is_open()) {
        return {};
    }
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

bool write(const std::filesystem::path& path, std::string_view text)
{
    std::error_code failure;
    std::filesystem::create_directories(path.parent_path(), failure);
    std::ofstream file{path, std::ios::trunc};
    if (!file.is_open()) {
        return false;
    }
    file << text;
    return file.good();
}

}
