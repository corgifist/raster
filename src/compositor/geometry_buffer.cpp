#include "compositor/geometry_framebuffer.h"
#include "compositor/compositor.h"
#include "gpu/gpu.h"

namespace Raster {
    GeometryFramebuffer::GeometryFramebuffer() {
        this->m_internalFramebuffer = DoubleBufferedFramebuffer();
    }

    GeometryFramebuffer::~GeometryFramebuffer() {
        if (m_internalFramebuffer.Get().handle) {
            DestroyInternalFramebuffer();
        }
    }

    Framebuffer& GeometryFramebuffer::Get(std::optional<Framebuffer> t_framebuffer) {
        EnsureResolutionConstraints(t_framebuffer);
        return m_internalFramebuffer.Get();
    }

    void GeometryFramebuffer::Destroy() {
        if (m_internalFramebuffer.Get().handle) {
            DestroyInternalFramebuffer();
        }
    }

    void GeometryFramebuffer::EnsureResolutionConstraints(std::optional<Framebuffer> t_framebuffer) {
        auto requiredResolution = Compositor::GetRequiredResolution();
        if (!t_framebuffer.has_value() || !t_framebuffer.value().handle) {
            InstantiateInternalFramebuffer(requiredResolution.x, requiredResolution.y);
        } else if (t_framebuffer.has_value() && t_framebuffer.value().handle && !t_framebuffer->attachments.empty()) {
            auto& passedFramebuffer = t_framebuffer.value();
            InstantiateInternalFramebuffer(passedFramebuffer.width, passedFramebuffer.height);
        }
    }

    void GeometryFramebuffer::DestroyInternalFramebuffer() {
        if (!m_internalFramebuffer.Get().handle) return;
        m_internalFramebuffer.Destroy();
        m_internalFramebuffer = DoubleBufferedFramebuffer();
    }

    void GeometryFramebuffer::InstantiateInternalFramebuffer(uint32_t width, uint32_t height) {
        if (m_internalFramebuffer.width == width && m_internalFramebuffer.height == height) return;
        DestroyInternalFramebuffer();
        this->m_internalFramebuffer = DoubleBufferedFramebuffer(
            GPU::GenerateFramebuffer(width, height, {
                GPU::GenerateTexture(width, height, 4, TexturePrecision::Half)
            }), 
            GPU::GenerateFramebuffer(width, height, {
                GPU::GenerateTexture(width, height, 4, TexturePrecision::Half)
            })
        );
    }
};