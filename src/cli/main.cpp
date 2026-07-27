#include "rgbpicker/cli.h"
#include "rgbpicker/runtime_backend_factory.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char* argv[])
{
    std::vector<std::string> arguments;
    arguments.reserve(static_cast<std::size_t>(argc > 0 ? argc - 1 : 0));
    for (int index{1}; index < argc; ++index) {
        arguments.emplace_back(argv[index]);
    }

    rgbpicker::RuntimeBackendFactory factory;
    rgbpicker::CliEnvironment environment{
        factory, [] { return std::chrono::steady_clock::now(); },
        [](std::chrono::milliseconds delay) { std::this_thread::sleep_for(delay); }};
    return rgbpicker::runCli(arguments, environment, std::cout, std::cerr);
}
