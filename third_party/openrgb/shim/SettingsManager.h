/*---------------------------------------------------------*\
| SettingsManager.h                                         |
|                                                           |
|   Stores OpenRGB driver settings as JSON and delegates    |
|   persistence to an injected store.                       |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#pragma once

#include <map>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

/** Persists serialized OpenRGB driver settings. */
class SettingsStore {
public:
    virtual ~SettingsStore() = default;

    /** Returns an empty string when no settings have been stored. */
    virtual std::string load() = 0;
    virtual void save(const std::string& settings) = 0;
};

class SettingsManager {
public:
    /** Returns an empty object when the settings key is absent. */
    nlohmann::json GetSettings(std::string settings_key)
    {
        loadOnce();
        const auto found{m_settings.find(settings_key)};
        return found == m_settings.end() ? nlohmann::json::object() : found->second;
    }

    nlohmann::json GetSettingsSchema(std::string) { return nlohmann::json::object(); }

    void SetSettings(std::string settings_key, nlohmann::json new_settings)
    {
        loadOnce();
        m_settings[std::move(settings_key)] = std::move(new_settings);
        m_dirty = true;
    }

    void SaveSettings()
    {
        if (!m_dirty || m_store == nullptr) {
            return;
        }
        m_dirty = false;
        nlohmann::json all = nlohmann::json::object();
        for (const auto& [key, settings] : m_settings) {
            all[key] = settings;
        }
        m_store->save(all.dump(2));
    }

    void UseStore(SettingsStore* store) { m_store = store; }

private:
    void loadOnce()
    {
        if (m_loaded || m_store == nullptr) {
            return;
        }
        m_loaded = true;
        const nlohmann::json all = nlohmann::json::parse(m_store->load(), nullptr, false);
        if (!all.is_object()) {
            return;
        }
        for (const auto& [key, settings] : all.items()) {
            m_settings[key] = settings;
        }
    }

    std::map<std::string, nlohmann::json> m_settings;
    SettingsStore* m_store{nullptr};
    bool m_loaded{false};
    bool m_dirty{false};
};
