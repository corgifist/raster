#include "common/ui_helpers.h"
#include "ui.h"
#include "common/configuration.h"
#include "common/generic_video_decoder.h"
#include "common/localization.h"
#include "common/plugin_base.h"
#include "common/user_interface.h"
#include "common/workspace.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "raster.h"
#include "../../ImGui/imgui.h"
#include "gpu/gpu.h"
#include "common/user_interfaces.h"
#include "windows/node_graph.h"
#include "windows/node_properties.h"
#include "windows/timeline.h"
#include "windows/rendering.h"
#include "windows/asset_manager.h"
#include "windows/easing_editor.h"
#include "windows/audio_buses.h"
#include "windows/audio_monitor.h"
#include <memory>

namespace Raster {
    std::string UIPlugin::AbstractName() {
        return "Built-in UI";
    }

    std::string UIPlugin::AbstractDescription() {
        return "Node Graph, Timeline, Asset Manager, Audio Monitor and etc.";
    }

    std::string UIPlugin::AbstractIcon() {
        return ICON_FA_WINDOW_RESTORE;
    }

    std::string UIPlugin::AbstractPackageName() {
        return RASTER_PACKAGED "ui";
    }

    void UIPlugin::AbstractSetupUI() {
        UserInterfaces::s_implementations.push_back(
            UserInterfaceImplementation{
                .libraryName = RASTER_PACKAGED "node_graph",
                .description = UserInterfaceDescription{
                    .prettyName = "Node Graph",
                    .packageName = RASTER_PACKAGED "node_graph",
                    .icon = ICON_FA_CIRCLE_NODES
                },
                .spawn = []() -> AbstractUserInterface {
                    return RASTER_SPAWN_ABSTRACT(Raster::AbstractUserInterface, Raster::NodeGraphUI);
                }
            }
        );

        UserInterfaces::s_implementations.push_back(
            UserInterfaceImplementation{
                .libraryName = RASTER_PACKAGED "node_properties",
                .description = UserInterfaceDescription{
                    .prettyName = "Node Properties",
                    .packageName = RASTER_PACKAGED "node_properties",
                    .icon = ICON_FA_LIST
                },
                .spawn = []() -> AbstractUserInterface {
                    return RASTER_SPAWN_ABSTRACT(Raster::AbstractUserInterface, Raster::NodePropertiesUI);
                }
            }
        );

        UserInterfaces::s_implementations.push_back(
            UserInterfaceImplementation{
                .libraryName = RASTER_PACKAGED "timeline",
                .description = UserInterfaceDescription{
                    .prettyName = "Timeline",
                    .packageName = RASTER_PACKAGED "timeline",
                    .icon = ICON_FA_TIMELINE
                },
                .spawn = []() -> AbstractUserInterface {
                    return RASTER_SPAWN_ABSTRACT(Raster::AbstractUserInterface, Raster::TimelineUI);
                }
            }
        );

        UserInterfaces::s_implementations.push_back(
            UserInterfaceImplementation{
                .libraryName = RASTER_PACKAGED "rendering",
                .description = UserInterfaceDescription{
                    .prettyName = "Rendering",
                    .packageName = RASTER_PACKAGED "rendering",
                    .icon = ICON_FA_IMAGE
                },
                .spawn = []() -> AbstractUserInterface {
                    return RASTER_SPAWN_ABSTRACT(Raster::AbstractUserInterface, Raster::RenderingUI);
                }
            }
        );

        UserInterfaces::s_implementations.push_back(
            UserInterfaceImplementation{
                .libraryName = RASTER_PACKAGED "asset_manager",
                .description = UserInterfaceDescription{
                    .prettyName = "Asset Manager",
                    .packageName = RASTER_PACKAGED "Asset Manager",
                    .icon = ICON_FA_FOLDER_OPEN
                },
                .spawn = []() -> AbstractUserInterface {
                    return RASTER_SPAWN_ABSTRACT(Raster::AbstractUserInterface, Raster::AssetManagerUI);
                }
            }
        );

        UserInterfaces::s_implementations.push_back(
            UserInterfaceImplementation{
                .libraryName = RASTER_PACKAGED "easing_editor",
                .description = UserInterfaceDescription{
                    .prettyName = "Easing Editor",
                    .packageName = RASTER_PACKAGED "easing_editor",
                    .icon = ICON_FA_BEZIER_CURVE
                },
                .spawn = []() -> AbstractUserInterface {
                    return RASTER_SPAWN_ABSTRACT(Raster::AbstractUserInterface, Raster::EasingEditorUI);
                }
            }
        );

        UserInterfaces::s_implementations.push_back(
            UserInterfaceImplementation{
                .libraryName = RASTER_PACKAGED "audio_buses",
                .description = UserInterfaceDescription{
                    .prettyName = "Audio Buses",
                    .packageName = RASTER_PACKAGED "audio_buses",
                    .icon = ICON_FA_VOLUME_HIGH
                },
                .spawn = []() -> AbstractUserInterface {
                    return RASTER_SPAWN_ABSTRACT(Raster::AbstractUserInterface, Raster::AudioBusesUI);
                }
            }
        );

        UserInterfaces::s_implementations.push_back(
            UserInterfaceImplementation{
                .libraryName = RASTER_PACKAGED "audio_monitor",
                .description = UserInterfaceDescription{
                    .prettyName = "Audio Monitor",
                    .packageName = RASTER_PACKAGED "audio_monitor",
                    .icon = ICON_FA_VOLUME_HIGH
                },
                .spawn = []() -> AbstractUserInterface {
                    return RASTER_SPAWN_ABSTRACT(Raster::AbstractUserInterface, Raster::AudioMonitorUI);
                }
            }
        );

    }

    void UIPlugin::AbstractRenderProperties() {
        UIHelpers::RenderNothingToShowText();
    }

    void UIPlugin::AbstractRenderWindowPopup() {
        if (!Workspace::IsProjectLoaded()) return;
        auto& project = Workspace::GetProject();
        auto& configuration = Workspace::s_configuration;
        project.audioBusesMutex->lock();
        for (auto& bus : project.audioBuses) {
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PLUS " " ICON_FA_VOLUME_HIGH, FormatString(Localization::GetString("NEW_AUDIO_MONITOR_FORMAT"), bus.name.c_str()).c_str()).c_str())) {
                for (auto& layout : configuration.layouts) {
                    if (layout.id == configuration.selectedLayout) {
                        auto audioMonitor = std::make_shared<AudioMonitorUI>(bus.id);
                        layout.windows.push_back((AbstractUserInterface) audioMonitor);
                        break;
                    }
                }
            }
        }
        project.audioBusesMutex->unlock();
    }
};

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractPlugin SpawnPlugin() {
        return RASTER_SPAWN_PLUGIN(Raster::UIPlugin);
    }
}