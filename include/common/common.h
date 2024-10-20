#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "typedefs.h"
#include "font/IconsFontAwesome5.h"

#include "composition.h"
#include "configuration.h"
#include "libraries.h"
#include "localization.h"
#include "node_base.h"
#include "project.h"
#include "workspace.h"
#include "randomizer.h"
#include "attribute.h"
#include "attributes.h"

#define INSTANTIATE_ATTRIBUTE_TEMPLATE(T) \
    template std::optional<T> NodeBase::GetAttribute<T>(std::string); 
