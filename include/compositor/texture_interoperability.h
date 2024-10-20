#pragma once

#include "compositor.h"

namespace Raster {
    
    // This utility class helphs to cast Framebuffer to Texture and vice versa
    // Must be used in pair with GetDynamicAttribute()
    struct TextureInteroperability {
        static std::optional<Framebuffer> GetFramebuffer(std::optional<std::any> t_value);
        static std::optional<Texture> GetTexture(std::optional<std::any> t_value);
    };
};