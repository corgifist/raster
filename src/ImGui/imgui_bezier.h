// ImGui Bezier widget. @r-lyeh, public domain
// v1.03: improve grabbing, confine grabbers to area option, adaptive size, presets, preview.
// v1.02: add BezierValue(); comments; usage
// v1.01: out-of-bounds coord snapping; custom border width; spacing; cosmetics
// v1.00: initial version
//
// [ref] http://robnapier.net/faster-bezier
// [ref] http://easings.net/es#easeInSine
//
// Usage:
// {  static float v[5] = { 0.390f, 0.575f, 0.565f, 1.000f }; 
//    ImGui::Bezier( "easeOutSine", v );       // draw
//    float y = ImGui::BezierValue( 0.5f, v ); // x delta in [0..1] range
// }

#include <algorithm>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include <optional>
#include <vector>
#include <time.h>

namespace ImGui
{
    template<int steps>
    void bezier_table(ImVec2 P[4], ImVec2 results[steps + 1]) {
        static float C[(steps + 1) * 4], *K = 0;
        if (!K) {
            K = C;
            for (unsigned step = 0; step <= steps; ++step) {
                float t = (float) step / (float) steps;
                C[step * 4 + 0] = (1 - t)*(1 - t)*(1 - t);   // * P0
                C[step * 4 + 1] = 3 * (1 - t)*(1 - t) * t; // * P1
                C[step * 4 + 2] = 3 * (1 - t) * t*t;     // * P2
                C[step * 4 + 3] = t * t*t;               // * P3
            }
        }
        for (unsigned step = 0; step <= steps; ++step) {
            ImVec2 point = {
                K[step * 4 + 0] * P[0].x + K[step * 4 + 1] * P[1].x + K[step * 4 + 2] * P[2].x + K[step * 4 + 3] * P[3].x,
                K[step * 4 + 0] * P[0].y + K[step * 4 + 1] * P[1].y + K[step * 4 + 2] * P[2].y + K[step * 4 + 3] * P[3].y
            };
            results[step] = point;
        }
    }


#define STEPS 256

    float BezierValue(float dt01, float P[4]) {
        ImVec2 Q[4] = { { 0, 0 }, { P[0], P[1] }, { P[2], P[3] }, { 1, 1 } };
        ImVec2 results[STEPS + 1];
        bezier_table<STEPS>(Q, results);
        return results[(int) ((dt01 < 0 ? 0 : dt01 > 1 ? 1 : dt01) * STEPS)].y;
    }

#define AREA_WIDTH area_width
#define AREA_CONSTRAINED constrained
#define SMOOTHNESS 64
#define CURVE_WIDTH 4
#define LINE_WIDTH 1
#define GRAB_RADIUS 8
#define GRAB_BORDER 2
    int Bezier(const char *label, float P[5], int area_width = 0, bool constrained = false, std::optional<std::vector<float>> customValues = std::nullopt, bool useHandles = true) {

        // bezier widget

        const ImGuiStyle& Style = GetStyle();
        const ImGuiIO& IO = GetIO();
        ImDrawList* DrawList = GetWindowDrawList();
        ImGuiWindow* Window = GetCurrentWindow();
        if (Window->SkipItems)
            return false;

/*         // header and spacing
        int changed = SliderFloat4(label, P, 0, 1, "%.3f", 1.0f);
        int hovered = IsItemActive() || IsItemHovered(); // IsItemDragged() ?
        Dummy(ImVec2(0, 3)); */

        // prepare canvas
        const float avail = GetContentRegionAvail().x;
        const float dim = AREA_WIDTH > 0 ? AREA_WIDTH : avail;
        ImVec2 Canvas(dim, dim);

        ImRect bb(Window->DC.CursorPos, Window->DC.CursorPos + Canvas);
        ItemSize(bb);
        if (!ItemAdd(bb, 0, 0))
            return 0;

        const ImGuiID id = Window->GetID(label);
        // hovered |= 0 != ItemHoverable(ImRect(bb.Min, bb.Min + ImVec2(avail, dim)), id, 0);

        RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg, 1), true, Style.FrameRounding);

        // background grid
        for (int i = 0; i <= Canvas.x; i += (Canvas.x / 4)) {
            DrawList->AddLine(
                ImVec2(bb.Min.x + i, bb.Min.y),
                ImVec2(bb.Min.x + i, bb.Max.y),
                GetColorU32(ImGuiCol_TextDisabled));
        }
        for (int i = 0; i <= Canvas.y; i += (Canvas.y / 4)) {
            DrawList->AddLine(
                ImVec2(bb.Min.x, bb.Min.y + i),
                ImVec2(bb.Max.x, bb.Min.y + i),
                GetColorU32(ImGuiCol_TextDisabled));
        }

        // eval curve
        ImVec2 Q[4] = { { 0, 0 }, { P[0], P[1] }, { P[2], P[3] }, { 1, 1 } };
        ImVec2 results[SMOOTHNESS + 1];
        bezier_table<SMOOTHNESS>(Q, results);

        // control points: 2 lines and 2 circles
        if (useHandles) {
            // handle grabbers
            ImVec2 mouse = GetIO().MousePos, pos[2];
            float distance[2];

            for (int i = 0; i < 2; ++i) {
                pos[i] = ImVec2(P[i * 2 + 0], 1 - P[i * 2 + 1]) * (bb.Max - bb.Min) + bb.Min;
                distance[i] = (pos[i].x - mouse.x) * (pos[i].x - mouse.x) + (pos[i].y - mouse.y) * (pos[i].y - mouse.y);
            }

            int selected = distance[0] < distance[1] ? 0 : 1;
            if( distance[selected] < (4*GRAB_RADIUS * 4*GRAB_RADIUS) )
            {
                SetTooltip("(%4.3f, %4.3f)", P[selected * 2 + 0], P[selected * 2 + 1]);

                if (/*hovered &&*/ (IsMouseClicked(0) || IsMouseDragging(0))) {
                    float &px = (P[selected * 2 + 0] += GetIO().MouseDelta.x / Canvas.x);
                    float &py = (P[selected * 2 + 1] -= GetIO().MouseDelta.y / Canvas.y);

                    float constrainPower = 0;
                    float lowerLimit = -constrainPower;
                    float upperLimit = 1;

                    if (AREA_CONSTRAINED) {
                        px = std::clamp(px, lowerLimit, upperLimit);
                        py = std::clamp(py, lowerLimit, upperLimit);
                    }

                    // changed = true;
                }
            }
        }

        // if (hovered || changed) DrawList->PushClipRectFullScreen();

        // draw curve
        {
            ImColor color(GetStyle().Colors[ImGuiCol_PlotLines]);
            int targetSmoothness = customValues.has_value() ? customValues.value().size() : SMOOTHNESS;
            for (int i = 0; i < targetSmoothness; ++i) {
                ImVec2 p, q;
                if (!customValues.has_value()) {
                    p = { results[i + 0].x, 1 - results[i + 0].y };
                    q = { results[i + 1].x, 1 - results[i + 1].y };
                } else {
                    auto& values = customValues.value();
                    p = {(float) i / (float) targetSmoothness, 1 - values[std::min(i, targetSmoothness - 1)]};
                    q = {(float) (i + 1) / (float) targetSmoothness, 1 - values[std::min(i + 1, targetSmoothness - 1)]};
                }
                ImVec2 r(p.x * (bb.Max.x - bb.Min.x) + bb.Min.x, p.y * (bb.Max.y - bb.Min.y) + bb.Min.y);
                ImVec2 s(q.x * (bb.Max.x - bb.Min.x) + bb.Min.x, q.y * (bb.Max.y - bb.Min.y) + bb.Min.y);
                DrawList->AddLine(r, s, color, CURVE_WIDTH);
            }
        }

        // draw preview (cycles every 1s)
/*         static clock_t epoch = clock();
        for (int i = 0; i < 3; ++i) {
            double now = ((clock() - epoch) / (double)CLOCKS_PER_SEC);
            float delta = ((int) (now * 1000) % 1000) / 1000.f; delta += i / 3.f; if (delta > 1) delta -= 1;
            int idx = (int) (delta * SMOOTHNESS);
            float evalx = results[idx].x; // 
            float evaly = results[idx].y; // ImGui::BezierValue( delta, P );
            ImVec2 p0 = ImVec2(evalx, 1 - 0) * (bb.Max - bb.Min) + bb.Min;
            ImVec2 p1 = ImVec2(0, 1 - evaly) * (bb.Max - bb.Min) + bb.Min;
            ImVec2 p2 = ImVec2(evalx, 1 - evaly) * (bb.Max - bb.Min) + bb.Min;
            DrawList->AddCircleFilled(p0, GRAB_RADIUS / 2, ImColor(white));
            DrawList->AddCircleFilled(p1, GRAB_RADIUS / 2, ImColor(white));
            DrawList->AddCircleFilled(p2, GRAB_RADIUS / 2, ImColor(white));
        } */

        ImVec4 white(GetStyle().Colors[ImGuiCol_Text]);
        if (useHandles) {
            // draw lines and grabbers
            float luma = IsItemActive() || IsItemHovered() ? 0.5f : 0.8f;
            ImVec4 pink(1, 1, 1, luma), cyan(1, 1, 1, luma);
            ImVec2 p1 = ImVec2(P[0], 1 - P[1]) * (bb.Max - bb.Min) + bb.Min;
            ImVec2 p2 = ImVec2(P[2], 1 - P[3]) * (bb.Max - bb.Min) + bb.Min;
            DrawList->AddLine(ImVec2(bb.Min.x, bb.Max.y), p1, ImColor(white), LINE_WIDTH);
            DrawList->AddLine(ImVec2(bb.Max.x, bb.Min.y), p2, ImColor(white), LINE_WIDTH);
            DrawList->AddCircleFilled(p1, GRAB_RADIUS, ImColor(white));
            DrawList->AddCircleFilled(p1, GRAB_RADIUS - GRAB_BORDER, ImColor(pink));
            DrawList->AddCircleFilled(p2, GRAB_RADIUS, ImColor(white));
            DrawList->AddCircleFilled(p2, GRAB_RADIUS - GRAB_BORDER, ImColor(cyan));
        }

        // if (hovered || changed) DrawList->PopClipRect();

        return 0;
    }
}