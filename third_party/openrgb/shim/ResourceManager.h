/*---------------------------------------------------------*\
| ResourceManager.h                                         |
|                                                           |
|   Gives embedded OpenRGB drivers access to their settings |
|   manager.                                                |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#pragma once

#include "SettingsManager.h"

class ResourceManager {
public:
    static ResourceManager* get()
    {
        static ResourceManager instance;
        return &instance;
    }

    SettingsManager* GetSettingsManager() { return &m_settings; }

private:
    SettingsManager m_settings;
};
