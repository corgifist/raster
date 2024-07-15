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

    static std::string ReadFile(const std::string& filename) {
        std::string buffer;
        std::ifstream in(filename.c_str(), std::ios_base::binary | std::ios_base::ate);
        in.exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit);
        buffer.resize(in.tellg());
        in.seekg(0, std::ios_base::beg);
        in.read(&buffer[0], buffer.size());
        return buffer;
    }

    template <typename T>
    static bool IsInBounds(const T& value, const T& low, const T& high) {
        return !(value < low) && (value < high);
    }

    static void WriteFile(std::string path, std::string content) {
        std::ofstream file(path, std::ios_base::binary);
        file.exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit);
        file << content;
    }

    static std::string ReplaceString(std::string subject, const std::string& search,
                            const std::string& replace) {
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return subject;
    }
}