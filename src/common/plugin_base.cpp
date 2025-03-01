#include "common/plugin_base.h"
#include "common/configuration.h"
#include "common/workspace.h"
#include "raster.h"

namespace Raster {
    PluginBase::PluginBase() {}

    std::string PluginBase::Name() {
        return AbstractName();
    }

    std::string PluginBase::Description() {
        return AbstractDescription();
    }

    std::string PluginBase::Icon() {
        return AbstractIcon();
    }

    std::string PluginBase::PackageName() {
        return AbstractPackageName();
    } 

    void PluginBase::OnWorkspaceInitialization() {
        RASTER_LOG("initializing plugin '" << Name() << "'");
        AbstractOnWorkspaceInitialization();
    }

    void PluginBase::OnEarlyInitialization() {
        RASTER_LOG("early initialization of plugin '" << PackageName() << "'");
        AbstractOnEarlyInitialization();
    }

    void PluginBase::OnLateInitialization() {
        RASTER_LOG("late initialization of plugin '" << PackageName() << "'");
        AbstractOnLateInitialization();
    }

    void PluginBase::SetupUI() {
        AbstractSetupUI();
    }

    Json& PluginBase::GetPluginData() {
        return Workspace::s_configuration.GetPluginData(PackageName());
    }

    void PluginBase::RenderProperties() {
        AbstractRenderProperties();
    }

    void PluginBase::WriteConfigs() {
        AbstractWriteConfigs();
    }

    void PluginBase::RenderWindowPopup() {
        AbstractRenderWindowPopup();
    }
}