#pragma once

#include "rgbpicker/backend.h"
#include "rgbpicker/backend_session.h"

#include <chrono>
#include <ostream>
#include <string>
#include <vector>

namespace rgbpicker {

struct CliEnvironment {
    BackendFactory& factory;
    SteadyNow now;
    Sleeper sleep;
    std::chrono::milliseconds backendWaitBudget{20000};
};

int runCli(const std::vector<std::string>& arguments, CliEnvironment& environment,
           std::ostream& output, std::ostream& error);

}
