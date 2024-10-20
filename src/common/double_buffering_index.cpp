#include "common/double_buffering_index.h"

namespace Raster {
    int DoubleBufferingIndex::s_index = 0;
    std::mutex DoubleBufferingIndex::s_mutex;
};