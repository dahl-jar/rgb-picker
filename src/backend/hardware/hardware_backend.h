#pragma once

#include "rgbpicker/backend.h"

#include <memory>
#include <vector>

namespace rgbpicker {

std::unique_ptr<Backend> makeHardwareBackend();

std::unique_ptr<Backend> makeMergedBackend(std::vector<std::unique_ptr<Backend>> backends);

}
