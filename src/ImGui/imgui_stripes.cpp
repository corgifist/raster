#include "imgui_stripes.h"
#include <cmath>

namespace ImGui {

    void Stripes(ImVec4 background_color, ImVec4 stripe_color, float stripes_width, float stripes_shift, ImVec2 canvas_size) {
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        if (canvas_size.x == 0 || canvas_size.y == 0) canvas_size = ImGui::GetContentRegionAvail();
        ImGui::PushClipRect(canvas_pos, canvas_pos + canvas_size, true);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_pos, canvas_pos + canvas_size, ImGui::GetColorU32(background_color));

            // stripes_width = stripes_width * (canvas_size.y / 387);
            auto stripe_offset = stripes_width;
            auto line_width = stripe_offset / 2.7f;
            
            auto time_offset = std::fmod(ImGui::GetTime() * 10 + stripes_shift, stripe_offset);
            auto h = canvas_size.y;
            auto stripe_count = (int)((canvas_size.x * 2.5f + h + 3 * line_width + time_offset) / stripe_offset);
            auto p = canvas_pos;
            auto offset = ImVec2(h + 2 * line_width, h + 2 * line_width);


            for (int i = 0; i < stripe_count; i++) {
                ImVec2 cp = p;
                cp.x -= offset.x;
                cp.x += time_offset;
                cp.y -= 2;
                if (cp.x > canvas_pos.x + canvas_size.x) break;
                if (cp.x + offset.y < canvas_pos.x) {
                    p.x += stripe_offset;
                    continue;
                } 
                draw_list->AddLine(cp, cp + offset, ImGui::GetColorU32(stripe_color), line_width);
                p.x += stripe_offset;
            }

        ImGui::PopClipRect();
    }

};