#pragma once

#include "raster.h"

namespace Raster {
    struct ItemAligner {
    public:
        ItemAligner() : m_lastLargestCursor(0) {}

        float GetLargestCursor();
        void AlignCursor();
        void AddCursorPos();
        void ClearCursors();

    private:
        std::vector<float> m_cursors;
        float m_lastLargestCursor;
    };
}