#include "compositor/async_rendering.h"
#include "common/rendering.h"
#include <chrono>
#include <ratio>
#include <thread>

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
        static int s_renderingPassID = 1;
        while (m_running) {
            if (Workspace::IsProjectLoaded()) {
                auto& project = Workspace::GetProject();
                while ((!project.playing && !Rendering::MustRenderFrame()) && m_running) {
                    std::this_thread::yield();
                    continue;
                }
                if (!m_running) break;
                Rendering::CancelRenderFrame();
                Compositor::EnsureResolutionConstraints();
                auto firstTimePoint = std::chrono::system_clock::now();
                double firstTime = GPU::GetTime();
                Compositor::s_bundles.Get().clear();
                project.Traverse({
                    {"RENDERING_PASS", true},
                    {"INCREMENT_EPF", true},
                    {"RESET_WORKSPACE_STATE", true},
                    {"ALLOW_MEDIA_DECODING", true},
                    {"ONLY_RENDERING_NODES", true},
                    {"RENDERING_PASS_ID", s_renderingPassID}
                });
                project.Traverse({
                    {"RENDERING_PASS", true},
                    {"INCREMENT_EPF", true},
                    {"RESET_WORKSPACE_STATE", false},
                    {"ALLOW_MEDIA_DECODING", false},
                    {"ONLY_AUDIO_NODES", true},
                    {"RENDERING_PASS_ID", s_renderingPassID}
                });
                s_renderingPassID++;
                Compositor::PerformComposition();
                DoubleBufferingIndex::s_index.Set((DoubleBufferingIndex::s_index.Get() + 1) % 2);
                GPU::Flush();

                double secondTime = GPU::GetTime();
                double timeDifference = (secondTime - firstTime) * 1000;
                s_renderTime = timeDifference;
                double idealTime = (1.0 / (double) project.framerate) * 1000;
                if (idealTime > timeDifference) {
                    int idealTimeDifference = idealTime - timeDifference;
                    ExperimentalSleepFor(idealTimeDifference / 1000.0); 
                }
                double finalTime = GPU::GetTime();
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
