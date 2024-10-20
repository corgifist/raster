#include "compositor/async_rendering.h"

namespace Raster {
    void* AsyncRendering::s_context = nullptr;
    bool AsyncRendering::m_running = false;
    bool AsyncRendering::m_allowRendering = false;
    std::thread AsyncRendering::m_renderingThread;

    void AsyncRendering::Initialize() {
        s_context = GPU::ReserveContext();
        RASTER_LOG("booting up async renderer");
        m_running = true;
        m_renderingThread = std::thread(AsyncRendering::RenderingLoop);
    }

    void AsyncRendering::RenderingLoop() {
        GPU::SetCurrentContext(s_context);
        Compositor::Initialize();
        while (m_running) {
            if (Workspace::IsProjectLoaded()) {
                double firstTime = GPU::GetTime();

                auto& project = Workspace::GetProject();
                Compositor::EnsureResolutionConstraints();
                Compositor::s_bundles.Get().clear();
                project.Traverse({
                    {"RENDERING_PASS", true}
                });
                Compositor::PerformComposition();
                GPU::Flush();

                double secondTime = GPU::GetTime();
                double timeDifference = (secondTime - firstTime) * 1000;
                double idealTime = (1.0 / (double) project.framerate) * 1000;
                if (idealTime > timeDifference) {
                    double idealTimeDifference = idealTime - timeDifference;
                    ExperimentalSleepFor(idealTimeDifference / 1000.0);
                }
                double finalTime = GPU::GetTime();
                m_allowRendering = false;
                {
                    RASTER_SYNCHRONIZED(DoubleBufferingIndex::s_mutex);
                    DoubleBufferingIndex::s_index = (DoubleBufferingIndex::s_index + 1) % 2;
                }
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
