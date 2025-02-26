#pragma once
#include "raster.h"

namespace Raster {
    struct Layouts {
    public:
        static std::optional<std::string> GetRequestedLayout();
        static void RequestLayout(std::string t_path);
        static void LoadLayout(int t_layoutID);

        // t_windowID can be obtained from UserInterfaceBase::id
        static void DestroyWindow(int t_windowID);

        static void Update();
    };
}