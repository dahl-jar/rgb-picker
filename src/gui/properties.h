#pragma once

#include "gui/session.h"
#include "gui/worker.h"

namespace rgbpicker::gui {

void drawProperties(const Device* device, DeviceScratch* scratch, Worker& worker, float height);

}
