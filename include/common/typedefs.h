#pragma once

#include "raster.h"

namespace Raster {
    struct NodeBase;

    using Json = nlohmann::json;
    using AbstractPinMap = std::unordered_map<int, std::any>;

    using PropertyDispatcherFunction = std::function<void(NodeBase*, std::string, std::any&, bool)>;
    using PropertyDispatchersCollection = std::unordered_map<std::type_index, PropertyDispatcherFunction>;

    using StringDispatcherFunction = std::function<void(std::any&)>;
    using StringDispatchersCollection = std::unordered_map<std::type_index, StringDispatcherFunction>;

    using PreviewDispatcherFunction = StringDispatcherFunction;
    using PreviewDispatchersCollection = StringDispatchersCollection;
};