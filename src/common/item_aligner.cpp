#include "common/item_aligner.h"
#include "../ImGui/imgui.h"
#include <algorithm>

namespace Raster {
    void ItemAligner::AddCursorPos() {
        m_cursors.push_back(ImGui::GetCursorPosX());
    }

    float ItemAligner::GetLargestCursor() {
        if (m_cursors.empty()) return m_lastLargestCursor;
        auto max = *(std::max_element(m_cursors.begin(), m_cursors.end()));
        return max;
    }

    void ItemAligner::ClearCursors() {
        m_lastLargestCursor = GetLargestCursor();
        m_cursors.clear();
    }

    void ItemAligner::AlignCursor() {
        AddCursorPos();
        if (m_lastLargestCursor != 0.0f )ImGui::SetCursorPosX(m_lastLargestCursor);
    }
};