#pragma once

#include "gui/worker.h"

#include "rgbpicker/profiles.h"

#include <vector>

namespace rgbpicker::gui {

void drawProfileRail(Worker& worker, const std::vector<rgbpicker::DeviceColor>& applied,
                     float height);

}
