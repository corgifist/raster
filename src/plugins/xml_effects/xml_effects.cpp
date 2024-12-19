#include "xml_effect_provider.h"
#include "xml_effects.h"
#include "common/plugin_base.h"
#include "font/IconsFontAwesome5.h"
#include "raster.h"

namespace Raster {

    using namespace pugi;

    std::string XMLEffectsPlugin::AbstractName() {
        return "XML Effects";
    }

    std::string XMLEffectsPlugin::AbstractDescription() {
        return "Allows creating custom effects without coding/compiling anything!";
    }

    std::string XMLEffectsPlugin::AbstractIcon() {
        return ICON_FA_CODE;
    }

    std::string XMLEffectsPlugin::AbstractPackageName() {
        return RASTER_PACKAGED "xml_effects_plugin";
    }

    void XMLEffectsPlugin::AbstractOnWorkspaceInitialization() {
        auto xmlIterator = std::filesystem::directory_iterator("misc/xml/effects");
        for (auto& entry : xmlIterator) {
            std::function<AbstractNode()> spawnFunction = [entry]() {
                return std::make_shared<XMLEffectProvider>(entry.path().string());
            };

            xml_document doc;
            if (!doc.load_file(entry.path().string().c_str())) {
                print("failed to load xml effect '" << entry << "'");
                continue;
            }
            print("loading xml effect '" << entry << "'");

            auto description = doc.child("effect").child("description");
            auto packaged = description.attribute("packaged").as_bool();
            auto packageName = description.attribute("packageName").as_string();
            auto name = description.attribute("name").as_string();

            NodeImplementation implementation;
            implementation.libraryName = packageName;
            implementation.spawn = spawnFunction;
            implementation.description.packageName = std::string(packaged ? RASTER_PACKAGED : "") + packageName;
            implementation.description.prettyName = name;
            implementation.description.category = DefaultNodeCategories::s_rendering;

            Workspace::s_nodeImplementations.push_back(implementation);
        }
    }
};

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractPlugin SpawnPlugin() {
        return RASTER_SPAWN_PLUGIN(Raster::XMLEffectsPlugin);
    }
}