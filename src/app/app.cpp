#include "app/app.h"
#include "gpu/gpu.h"
#include "../ImGui/imgui.h"

namespace Raster {
    void App::Initialize() {
        GPU::Initialize();
        ImGui::SetCurrentContext((ImGuiContext*) GPU::GetImGuiContext());
    }

    void App::RenderLoop() {
        while (!GPU::MustTerminate()) {
            GPU::BeginFrame();
                ImGui::ShowDemoWindow();
            GPU::EndFrame();
        }
    }

    void App::Terminate() {
        GPU::Terminate();
    }
}