#pragma once

#include "gui/session.h"
#include "gui/worker.h"

#include <vector>

namespace rgbpicker::gui {

void drawWorkspace(const std::vector<Device>& devices, const Device* device, PickerTarget& target,
                   Worker& worker, ImVec2 size);

}
