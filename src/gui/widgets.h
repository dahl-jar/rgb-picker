#pragma once

#include "imgui.h"

#include <string>

namespace rgbpicker::gui {


void helpMarker(const char* text);

void dottedRule();

std::string fitText(const char* text, float maxWidth);

void segmentedToggle(const char* id, bool& value);

bool sectionBand(const char* title, bool& open);

void rowLabel(const char* label, const char* help = nullptr);

void trashIcon(ImDrawList* draw, ImVec2 centre, ImU32 ink);

bool settingsButton(const char* id, float size);

void drawRailHeader(const char* title);

}
