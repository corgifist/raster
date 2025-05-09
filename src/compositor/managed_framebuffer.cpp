#include "compositor/managed_framebuffer.h"
#include "compositor/compositor.h"
#include "gpu/gpu.h"

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
        EnsureResolutionConstraints(t_framebuffer);
        auto& internalFramebuffer =  m_internalFramebuffer.Get();
        GPU::BindFramebuffer(internalFramebuffer);
        GPU::ClearFramebuffer(0, 0, 0, 0);
        if (t_framebuffer.has_value() && t_framebuffer.value().handle && t_framebuffer.value().attachments.size() == internalFramebuffer.attachments.size() && t_framebuffer->attachments[0].precision == internalFramebuffer.attachments[0].precision) {
            auto& framebuffer = t_framebuffer.value();
            int index = 0;
            for (auto& attachment : framebuffer.attachments) {
                GPU::BlitTexture(internalFramebuffer.attachments[index++], attachment);
            }
        }
        return internalFramebuffer;
    }

    Framebuffer& ManagedFramebuffer::GetWithoutBlitting(std::optional<Framebuffer> t_framebuffer) {
        EnsureResolutionConstraints(t_framebuffer);
        return m_internalFramebuffer.Get();
    }

    Framebuffer& ManagedFramebuffer::GetReadyFramebuffer() {
        return m_internalFramebuffer.GetFrontFramebuffer();
    }

    void ManagedFramebuffer::Destroy() {
        if (m_internalFramebuffer.Get().handle) {
            DestroyInternalFramebuffer();
        }
    }

    void ManagedFramebuffer::EnsureResolutionConstraints(std::optional<Framebuffer> t_framebuffer) {
        auto requiredResolution = Compositor::GetRequiredResolution();
        if (!t_framebuffer.has_value() || !t_framebuffer.value().handle) {
            InstantiateInternalFramebuffer(requiredResolution.x, requiredResolution.y, Compositor::s_colorPrecision);
        } else if (t_framebuffer.has_value() && t_framebuffer.value().handle && !t_framebuffer->attachments.empty()) {
            auto& passedFramebuffer = t_framebuffer.value();
            InstantiateInternalFramebuffer(passedFramebuffer.width, passedFramebuffer.height, t_framebuffer->attachments.at(0).precision);
        }
    }

    void ManagedFramebuffer::DestroyInternalFramebuffer() {
        if (!m_internalFramebuffer.Get().handle) return;
        m_internalFramebuffer.Destroy();
        m_internalFramebuffer = DoubleBufferedFramebuffer();
    }

    void ManagedFramebuffer::InstantiateInternalFramebuffer(uint32_t width, uint32_t height, TexturePrecision precision) {
        if (m_internalFramebuffer.width == width && m_internalFramebuffer.height == height && (!m_internalFramebuffer.Get().attachments.empty() && m_internalFramebuffer.Get().attachments[0].precision == precision)) return;
        DestroyInternalFramebuffer();
        this->m_internalFramebuffer = Compositor::GenerateCompatibleDoubleBufferedFramebuffer({width, height}, precision);
    }
};