#pragma once

#include "raster.h"
#include "double_buffering_index.h"

namespace Raster {
    template <typename T>
    struct DoubleBufferedValue {
    private:
        T m_back, m_front;

    public:
        T& GetWithOffset(int t_offset = 0) {
            return (DoubleBufferingIndex::s_index.Get() + t_offset) % 2 ? m_back : m_front;
        }

        T& Get() {
            return DoubleBufferingIndex::s_index.Get() ? m_front : m_back;
        }
        T& GetFrontValue() {
            return DoubleBufferingIndex::s_index.Get() ? m_back : m_front;
        }

        void SetBackValue(T t_back) {
            auto& value = Get();
            value = t_back;
        }

        void SetFrontValue(T t_front) {
            auto& value = GetFrontValue();
            value = t_front;
        }

        void Set(T t_back, T t_front) {
            SetBackValue(t_back);
            SetFrontValue(t_front);
        }

        void Set(T t_value) {
            Set(t_value, t_value);
        }

        DoubleBufferedValue() {}
    };
};