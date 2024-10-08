#ifndef AV_FILTERPAD_H
#define AV_FILTERPAD_H

#include <iterator>
#include <memory>
#include <type_traits>

#include "../ffmpeg.h"
#include "../avutils.h"

namespace av {

class FilterPadList : public FFWrapperPtr<const AVFilterPad>
{
private:
    friend class Filter;
    FilterPadList(const AVFilterPad *begin, size_t count);

public:
    using FFWrapperPtr<const AVFilterPad>::FFWrapperPtr;

    size_t count() const noexcept;
    std::string name(size_t index) const noexcept;
    const char* nameCStr(size_t index) const noexcept;
    AVMediaType type(size_t index) const noexcept;

private:
    size_t m_count = 0;
};


} // namespace av

#endif // AV_FILTERPAD_H
