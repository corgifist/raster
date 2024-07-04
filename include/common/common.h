#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "typedefs.h"
#include "font/IconsFontAwesome5.h"

#include "attribute_dispatchers.h"
#include "string_dispatchers.h"
#include "preview_dispatchers.h"
#include "composition.h"
#include "configuration.h"
#include "dispatchers.h"
#include "libraries.h"
#include "localization.h"
#include "node_base.h"
#include "project.h"
#include "workspace.h"
#include "randomizer.h"
#include "attribute.h"

#define INSTANTIATE_ATTRIBUTE_TEMPLATE(T) \
    template std::optional<T> NodeBase::GetAttribute<T>(std::string); 

#define ATTRIBUTE_TYPE(T) \
    std::type_index(typeid(T))

#define RASTER_COLOR32(R,G,B,A)    (((uint32_t)(A)<<24) | ((uint32_t)(B)<<16) | ((uint32_t)(G)<<8) | ((uint32_t)(R)<<0))


namespace Raster {

    struct NodeBase;


    template<typename ... Args>
    static std::string FormatString( const std::string& format, Args ... args ) {
        int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
        if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
        auto size = static_cast<size_t>( size_s );
        std::unique_ptr<char[]> buf( new char[ size ] );
        std::snprintf( buf.get(), size, format.c_str(), args ... );
        return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
    }

    static Json ReadJson(std::string path) {
        return Json::parse(std::fstream(path));
    }
}