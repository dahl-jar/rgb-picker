#pragma once

#include "gui/app/session.h"
#include "gui/app/worker.h"

namespace rgbpicker::gui {

void drawProperties(const Device* device, DeviceScratch* scratch, Worker& worker, float height);

}
