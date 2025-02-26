#include "matchbox_effect_provider.h"
#include "common/node_category.h"
#include "matchbox_effects.h"
#include "common/plugin_base.h"
#include "font/IconsFontAwesome5.h"
#include "raster.h"
#include <exception>

namespace Raster {

    using namespace pugi;

    std::string MatchboxEffectsPlugin::AbstractName() {
        return "Matchbox Effects";
    }

    std::string MatchboxEffectsPlugin::AbstractDescription() {
        return "Allows usage of Autodesk Matchbox shaders in Raster\nNotice: GLSL 130 is not supported by Raster! Matchbox Shaders require additional preparation";
    }

    std::string MatchboxEffectsPlugin::AbstractIcon() {
        return ICON_FA_BOX;
    }

    std::string MatchboxEffectsPlugin::AbstractPackageName() {
        return RASTER_PACKAGED "matchbox_effects_plugin";
    }

    void MatchboxEffectsPlugin::LoadMatchboxEffects() {
        auto xmlIterator = std::filesystem::directory_iterator("xml/matchbox/");
        static NodeCategory matchboxCategory = NodeCategoryUtils::RegisterCategory(ICON_FA_BOX, "Matchbox Effects");
        for (auto& entry : xmlIterator) {
            std::function<AbstractNode()> spawnFunction = [entry]() {
                return std::make_shared<MatchboxEffectProvider>(entry.path().string());
            };

            xml_document doc;
            if (!doc.load_file(entry.path().string().c_str())) {
                print("failed to load matchbox effect '" << entry << "'");
                continue;
            }
            print("loading matchbox effect '" << entry.path().string() << "'");

            auto metadata = MatchboxEffectProvider::ParseMetadata(doc);

            NodeImplementation implementation;
            implementation.libraryName = metadata.name;
            implementation.spawn = spawnFunction;
            implementation.description.packageName = "adsk.matchbox." + RemoveExtension(GetBaseName(entry.path().string()));
            implementation.description.prettyName = metadata.name;
            implementation.description.category = matchboxCategory;

            Workspace::s_nodeImplementations.push_back(implementation);
        }
    }

    void MatchboxEffectsPlugin::AbstractOnWorkspaceInitialization() {
        try {
            LoadMatchboxEffects();
        } catch (std::exception& e) {
            print("failed to load matchbox effects!");
            print("\t" << e.what());
        }
    }
};

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractPlugin SpawnPlugin() {
        return RASTER_SPAWN_PLUGIN(Raster::MatchboxEffectsPlugin);
    }
}