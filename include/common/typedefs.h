#pragma once

#include "raster.h"

namespace Raster {
    struct NodeBase;
    struct Composition;

    using AbstractPinMap = std::unordered_map<int, std::any>;
    using ContextData = std::unordered_map<std::string, std::any>;

    using PropertyDispatcherFunction = std::function<void(NodeBase*, std::string, std::any&, bool)>;
    using PropertyDispatchersCollection = std::unordered_map<std::type_index, PropertyDispatcherFunction>;

    using StringDispatcherFunction = std::function<void(std::any&)>;
    using StringDispatchersCollection = std::unordered_map<std::type_index, StringDispatcherFunction>;

    using PreviewDispatcherFunction = StringDispatcherFunction;
    using PreviewDispatchersCollection = StringDispatchersCollection;

    using OverlayDispatcherFunction = std::function<bool(std::any&, Composition*, int, float, glm::vec2)>;
    using OverlayDispatchersCollection = std::unordered_map<std::type_index, OverlayDispatcherFunction>;
};