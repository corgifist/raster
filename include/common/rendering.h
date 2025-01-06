#pragma once

#include "raster.h"

namespace Raster {
    struct Rendering {
        
        // returns true if current frame needs to be re-rendered
        static bool MustRenderFrame();

        // reverts ForceRenderFrame() call
        static void CancelRenderFrame();

        // force the rendering system to re-render current frame
        static void ForceRenderFrame();
    };
};