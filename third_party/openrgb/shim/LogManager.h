/*---------------------------------------------------------*\
| LogManager.h                                              |
|                                                           |
|   Keeps OpenRGB logging macros available to embedded      |
|   drivers. Calls remain type-checked and produce no       |
|   runtime output.                                         |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define RGBPICKER_DISCARD_LOG(...) (static_cast<void>(sizeof(__VA_ARGS__)))

#define LOG_TRACE(...) RGBPICKER_DISCARD_LOG(__VA_ARGS__)
#define LOG_DEBUG(...) RGBPICKER_DISCARD_LOG(__VA_ARGS__)
#define LOG_VERBOSE(...) RGBPICKER_DISCARD_LOG(__VA_ARGS__)
#define LOG_INFO(...) RGBPICKER_DISCARD_LOG(__VA_ARGS__)
#define LOG_NOTICE(...) RGBPICKER_DISCARD_LOG(__VA_ARGS__)
#define LOG_WARNING(...) RGBPICKER_DISCARD_LOG(__VA_ARGS__)
#define LOG_ERROR(...) RGBPICKER_DISCARD_LOG(__VA_ARGS__)
#define LOG_CRITICAL(...) RGBPICKER_DISCARD_LOG(__VA_ARGS__)
