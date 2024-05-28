#pragma once

#include "raster.h"

namespace Raster {
    struct UI {
        virtual void Render() = 0;
    };

    using AbstractUI = std::unique_ptr<UI>;

    struct UIFactory {
        static std::unique_ptr<UI> SpawnNodeGraphUI();
        static std::unique_ptr<UI> SpawnNodePropertiesUI();
    };
}