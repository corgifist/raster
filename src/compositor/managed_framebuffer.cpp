#include "compositor/managed_framebuffer.h"

namespace Raster {
    ManagedFramebuffer::ManagedFramebuffer() {
        this->m_internalFramebuffer = DoubleBufferedFramebuffer();
    }

    ManagedFramebuffer::~ManagedFramebuffer() {
        if (m_internalFramebuffer.Get().handle) {
            DestroyInternalFramebuffer();
        }
    }

    Framebuffer& ManagedFramebuffer::Get(std::optional<Framebuffer> t_framebuffer) {
        auto requiredResolution = Compositor::GetRequiredResolution();
        if (!m_internalFramebuffer.Get().handle) {
            InstantiateInternalFramebuffer(requiredResolution.x, requiredResolution.y);
        }
        EnsureResolutionConstraints();
        auto& framebuffer = m_internalFramebuffer.GetFrontFramebuffer();
        GPU::BindFramebuffer(framebuffer);
        GPU::ClearFramebuffer(0, 0, 0, 0);
        if (t_framebuffer.has_value() && t_framebuffer.value().handle) {
            auto& framebuffer = t_framebuffer.value();
            int index = 0;
            for (auto& attachment : framebuffer.attachments) {
                GPU::BlitFramebuffer(framebuffer, attachment, index);
                index++;
            }
        }
        return framebuffer;
    }

    Framebuffer& ManagedFramebuffer::GetReadyFramebuffer() {
        return m_internalFramebuffer.Get();
    }

    void ManagedFramebuffer::Destroy() {
        if (m_internalFramebuffer.Get().handle) {
            DestroyInternalFramebuffer();
        }
    }

    void ManagedFramebuffer::EnsureResolutionConstraints() {
        auto requiredResolution = Compositor::GetRequiredResolution();
        if (m_internalFramebuffer.Get().handle && (m_internalFramebuffer.Get().width != requiredResolution.x || m_internalFramebuffer.Get().height != requiredResolution.y)) {
            DestroyInternalFramebuffer();
            InstantiateInternalFramebuffer(requiredResolution.x, requiredResolution.y);
        }
    }

    void ManagedFramebuffer::DestroyInternalFramebuffer() {
        m_internalFramebuffer.Destroy();
        m_internalFramebuffer = DoubleBufferedFramebuffer();
    }

    void ManagedFramebuffer::InstantiateInternalFramebuffer(uint32_t width, uint32_t height) {
        this->m_internalFramebuffer = Compositor::GenerateCompatibleDoubleBufferedFramebuffer({width, height});
    }
};