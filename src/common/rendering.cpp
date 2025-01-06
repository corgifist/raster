#include "common/rendering.h"
#include "common/synchronized_value.h"

namespace Raster {
    static SynchronizedValue<bool> s_mustBeRendered = false;

    bool Rendering::MustRenderFrame() {
        return s_mustBeRendered.Get();
    }

    void Rendering::ForceRenderFrame() {
        s_mustBeRendered.Lock();
        s_mustBeRendered.GetReference() = true;
        s_mustBeRendered.Unlock();
    }

    void Rendering::CancelRenderFrame() {
        s_mustBeRendered.Lock();
        s_mustBeRendered.GetReference() = false;
        s_mustBeRendered.Unlock();
    }
};