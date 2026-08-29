#pragma once

#include "gui/app/session.h"
#include "gui/app/worker.h"

#include <vector>

namespace rgbpicker::gui {

void drawWorkspace(const std::vector<Device>& devices, const Device* device, PickerTarget& target,
                   Worker& worker, ImVec2 size);

}
