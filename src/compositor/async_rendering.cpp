#include "compositor/async_rendering.h"

namespace Raster {
    void* AsyncRendering::s_context = nullptr;
    bool AsyncRendering::m_running = false;
    bool AsyncRendering::m_allowRendering = false;
    std::thread AsyncRendering::m_renderingThread;
    float AsyncRendering::s_renderTime = 0;

    void AsyncRendering::Initialize() {
        s_context = GPU::ReserveContext();
        RASTER_LOG("booting up async renderer");
        m_running = true;
        m_renderingThread = std::thread(AsyncRendering::RenderingLoop);
    }

    void AsyncRendering::RenderingLoop() {
        GPU::SetCurrentContext(s_context);
        Compositor::Initialize();
        DoubleBufferingIndex::s_index.Set(0);
        while (m_running) {
            if (Workspace::IsProjectLoaded()) {
                double firstTime = GPU::GetTime();

                auto& project = Workspace::GetProject();
                Compositor::EnsureResolutionConstraints();
                Compositor::s_bundles.Get().clear();
                project.Traverse({
                    {"RENDERING_PASS", true},
                    {"INCREMENT_EPF", true},
                    {"RESET_WORKSPACE_STATE", true},
                    {"ALLOW_MEDIA_DECODING", true},
                    {"ONLY_AUDIO_NODES", false},
                    {"ONLY_RENDERING_NODES", true}
                });
                project.Traverse({
                    {"RENDERING_PASS", true},
                    {"INCREMENT_EPF", true},
                    {"RESET_WORKSPACE_STATE", false},
                    {"ALLOW_MEDIA_DECODING", false},
                    {"ONLY_AUDIO_NODES", true}
                });
                Compositor::PerformComposition();
                DoubleBufferingIndex::s_index.Set((DoubleBufferingIndex::s_index.Get() + 1) % 2);
                GPU::Flush();

                double secondTime = GPU::GetTime();
                double timeDifference = (secondTime - firstTime) * 1000;
                s_renderTime = timeDifference;
                double idealTime = (1.0 / (double) project.framerate) * 1000;
                if (idealTime > timeDifference) {
                    double idealTimeDifference = idealTime - timeDifference;
                    ExperimentalSleepFor(idealTimeDifference / 1000.0);
                }
                double finalTime = GPU::GetTime();
                m_allowRendering = false;
            }
        }
    }

    void AsyncRendering::AllowRendering() {
        m_allowRendering = true;
    }

    void AsyncRendering::Terminate() {
        m_running = false;
        m_renderingThread.join();
    }
};
