#include "common/managed_framebuffer.h"

namespace Raster {
    ManagedFramebuffer::ManagedFramebuffer() {
        this->m_internalFramebuffer = Framebuffer();
    }

    ManagedFramebuffer::~ManagedFramebuffer() {
        if (m_internalFramebuffer.handle) {
            DestroyInternalFramebuffer();
        }
    }

    Framebuffer& ManagedFramebuffer::Get(std::optional<Framebuffer> t_framebuffer) {
        auto requiredResolution = Compositor::GetRequiredResolution();
        if (!m_internalFramebuffer.handle) {
            InstantiateInternalFramebuffer(requiredResolution.x, requiredResolution.y);
        }
        EnsureResolutionConstraints();
        GPU::BindFramebuffer(m_internalFramebuffer);
        GPU::ClearFramebuffer(0, 0, 0, 0);
        if (t_framebuffer.has_value() && t_framebuffer.value().handle) {
            auto& framebuffer = t_framebuffer.value();
            int index = 0;
            for (auto& attachment : framebuffer.attachments) {
                GPU::BlitFramebuffer(m_internalFramebuffer, attachment, index);
                index++;
            }
        }
        return m_internalFramebuffer;
    }

    void ManagedFramebuffer::Destroy() {
        if (m_internalFramebuffer.handle) {
            DestroyInternalFramebuffer();
        }
    }

    void ManagedFramebuffer::EnsureResolutionConstraints() {
        auto requiredResolution = Compositor::GetRequiredResolution();
        if (m_internalFramebuffer.handle && (m_internalFramebuffer.width != requiredResolution.x || m_internalFramebuffer.height != requiredResolution.y)) {
            DestroyInternalFramebuffer();
            InstantiateInternalFramebuffer(requiredResolution.x, requiredResolution.y);
        }
    }

    void ManagedFramebuffer::DestroyInternalFramebuffer() {
        for (auto& attachment : m_internalFramebuffer.attachments) {
            GPU::DestroyTexture(attachment);
        }
        GPU::DestroyFramebuffer(m_internalFramebuffer);
        this->m_internalFramebuffer = Framebuffer();
    }

    void ManagedFramebuffer::InstantiateInternalFramebuffer(uint32_t width, uint32_t height) {
        this->m_internalFramebuffer = Compositor::GenerateCompatibleFramebuffer({width, height});
    }
};