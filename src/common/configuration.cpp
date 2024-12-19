#include "common/common.h"

namespace Raster {
    Configuration::Configuration() {
        this->m_pluginData = {
            {"PluginData", {
                {"empty_plugin", nullptr}
            }}
        };
    }

    Configuration::Configuration(Json data) {
        this->m_pluginData = data;
    }

    Json& Configuration::GetPluginData(std::string t_packageName) {
        if (!m_pluginData.contains("PluginData")) {
            m_pluginData["PluginData"] = Json::object();
        }
        auto& pluginData = m_pluginData["PluginData"];
        if (pluginData.contains(t_packageName)) {
            return pluginData[t_packageName];
        }
        pluginData[t_packageName] = Json::object();
        return pluginData[t_packageName];
    }

    Json Configuration::Serialize() {
        return m_pluginData;
    }
};