#pragma once

namespace Raster {
    enum class ProjectColorPrecision {
        Usual = 8, // uint8_t per color (1 byte)
        Half = 16, // using half float per color (2 bytes)
        Full = 32, // using full float per color (4 bytes)
    };
};