#include "compositor/async_rendering.h"
#include "common/audio_memory_management.h"
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
    Framebuffer AsyncRendering::s_readyFramebuffer;

    void AsyncRendering::Initialize() {
        s_context = GPU::ReserveContext();
        RASTER_LOG("booting up async renderer");
        m_running = true;
        m_renderingThread = std::thread(AsyncRendering::RenderingLoop);
    }

    void AsyncRendering::RenderingLoop() {
        GPU::SetCurrentContext(s_context);
        Compositor::Initialize();
        DoubleBufferingIndex::s_index = 0;
        static int s_renderingPassID = 1;
        while (m_running) {
            if (Workspace::IsProjectLoaded()) {
                auto& project = Workspace::GetProject();
                while ((!project.playing && !Rendering::MustRenderFrame()) && m_running) {
                    continue;
                }
                if (!m_running) break;
                if (!Rendering::MustRenderFrame() || !m_allowRendering) continue;
                Rendering::CancelRenderFrame();
                Compositor::EnsureResolutionConstraints();
                GPU::EnableClipping();
                GPU::SetClipRect(project.roi.upperLeft, project.roi.bottomRight);
                auto firstTimePoint = std::chrono::system_clock::now();
                double firstTime = GPU::GetTime();
                Compositor::s_bundles.Get().clear();
                AudioMemoryManagement::Reset();
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
                auto f = Compositor::PerformComposition();
                GPU::DisableClipping();
                GPU::Flush();
                if (Compositor::primaryFramebuffer) s_readyFramebuffer = Compositor::primaryFramebuffer.value().Get();

                double secondTime = GPU::GetTime();
                double timeDifference = (secondTime - firstTime) * 1000;
                s_renderTime = timeDifference;
                double idealTime = (1.0 / (double) project.framerate) * 1000;
                if (idealTime > timeDifference) {
                    int idealTimeDifference = idealTime - timeDifference;
                }
                double finalTime = GPU::GetTime();
                DoubleBufferingIndex::s_index = (DoubleBufferingIndex::s_index + 1) % 2;
                m_allowRendering = false;
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
        GPU::DestroyContext(s_context);
    }
};
