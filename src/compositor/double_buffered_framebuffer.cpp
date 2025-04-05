#include "compositor/double_buffered_framebuffer.h"
#include "compositor/compositor.h"
#include "compositor/async_rendering.h"
#include "common/double_buffering_index.h"

namespace Raster {
    DoubleBufferedFramebuffer::DoubleBufferedFramebuffer() {
        this->width = this->height = 0;
        this->m_index = 0;
    }

    DoubleBufferedFramebuffer::DoubleBufferedFramebuffer(int width, int height, TexturePrecision precision) {
        this->m_back = Compositor::GenerateCompatibleFramebuffer({width, height}, precision);
        this->m_front = Compositor::GenerateCompatibleFramebuffer({width, height}, precision);
        this->width = width;
        this->height = height;
        this->m_index = 0;
    }

    Framebuffer& DoubleBufferedFramebuffer::Get() {
        return !(DoubleBufferingIndex::s_index.Get() % 2) ? m_back : m_front;
    }

    Framebuffer& DoubleBufferedFramebuffer::GetWithOffset(int offset) {
        return !((DoubleBufferingIndex::s_index.Get() + offset) % 2) ? m_back : m_front;
    }

    Framebuffer& DoubleBufferedFramebuffer::GetFrontFramebufferWithoutSwapping() {
        return (DoubleBufferingIndex::s_index.Get() % 2) ? m_back : m_front;
    }

    void DoubleBufferedFramebuffer::SwapBuffers() {
        this->m_index = (this->m_index + 1) % 2;
    }

    Framebuffer& DoubleBufferedFramebuffer::GetFrontFramebuffer() {
        auto& result = GetFrontFramebufferWithoutSwapping();
        SwapBuffers();
        return result;
    }
    
    void DoubleBufferedFramebuffer::Destroy() {
        GPU::DestroyFramebufferWithAttachments(m_back);
        GPU::DestroyFramebufferWithAttachments(m_front);
    }
};