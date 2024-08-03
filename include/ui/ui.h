#pragma once

#include "raster.h"

namespace Raster {
    struct UI {
        virtual void Render() = 0;
    };

    using AbstractUI = std::unique_ptr<UI>;

    struct UIFactory {
        static AbstractUI SpawnNodeGraphUI();
        static AbstractUI SpawnNodePropertiesUI();
        static AbstractUI SpawnRenderingUI();
        static AbstractUI SpawnTimelineUI();
        static AbstractUI SpawnAssetManagerUI();
    };
}