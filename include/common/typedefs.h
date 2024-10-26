#pragma once

#include "raster.h"

namespace Raster {
    struct NodeBase;
    struct Composition;

    using AbstractPinMap = std::unordered_map<int, std::any>;
    using ContextData = std::unordered_map<std::string, std::any>;

    using PropertyDispatcherFunction = std::function<void(NodeBase*, std::string, std::any&, bool, std::vector<std::any>)>;
    using PropertyDispatchersCollection = std::unordered_map<std::type_index, PropertyDispatcherFunction>;

    using StringDispatcherFunction = std::function<void(std::any&)>;
    using StringDispatchersCollection = std::unordered_map<std::type_index, StringDispatcherFunction>;

    using PreviewDispatcherFunction = StringDispatcherFunction;
    using PreviewDispatchersCollection = StringDispatchersCollection;

    using OverlayDispatcherFunction = std::function<bool(std::any&, Composition*, int, float, glm::vec2)>;
    using OverlayDispatchersCollection = std::unordered_map<std::type_index, OverlayDispatcherFunction>;

    using ConversionDispatcherFunction = std::function<std::optional<std::any>(std::any&)>;

    struct ConversionDispatcherPair {
        std::type_index from;
        std::type_index to;
        ConversionDispatcherFunction function;

        ConversionDispatcherPair(std::type_index t_from, std::type_index t_to, ConversionDispatcherFunction t_function) :
            from(t_from), to(t_to), function(t_function) {}
    };

    using ConversionDispatchersCollection = std::vector<ConversionDispatcherPair>;
};