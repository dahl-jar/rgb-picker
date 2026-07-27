#pragma once

#include "rgbpicker/backend.h"

#include <memory>
#include <vector>

namespace rgbpicker {

std::vector<std::unique_ptr<Backend>> detectDriverBackends();

}
