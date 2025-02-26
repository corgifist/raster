#include "common/common.h"

namespace Raster {
    Configuration::Configuration() {
        this->m_pluginData = {
            {"PluginData", {
                {"empty_plugin", nullptr}
            }}
        };
        this->selectedLayout = -1;
    }

    Configuration::Configuration(Json data) {
        this->m_pluginData = data["PluginData"];
        if (data.contains("SelectedLayout")) this->selectedLayout = data["SelectedLayout"];
        if (data.contains("Layouts")) {
            for (auto& layout : data["Layouts"]) {
                layouts.push_back(Layout(layout));
            }
        }
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
        Json result = Json::object();
        result["PluginData"] = m_pluginData;
        result["SelectedLayout"] = selectedLayout;
        result["Layouts"] = Json::array();
        for (auto& layout : layouts) {
            result["Layouts"].push_back(layout.Serialize());
        }
        return result;
    }
};