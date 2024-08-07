#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include <unordered_map>
#include <math.h>
#include <algorithm>
#include <random>
#include <any>
#include <filesystem>
#include <regex>
#include <optional>
#include <fstream>
#include <typeindex>
#include <set>
#include <cmath>
#include <codecvt>
#include <locale>

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "json.hpp"

#define RASTER_PACKAGED "packaged.raster."

#define print(expr) std::cout << expr << std::endl

#define DUMP_VAR(var) print(#var << " = " << (var))

namespace Raster {

    using Json = nlohmann::json;
    
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
        return std::regex_replace(subject, std::regex(search), replace);
    }

    static glm::vec2 ScreenToNDC(glm::vec2 point, glm::vec2 screen) {
        return {
            (point.x / screen.x) * 2 - 1,
            1 - (point.y / screen.y) * 2
        };
    }

    static glm::vec2 NDCToScreen(glm::vec2 point, glm::vec2 screen) {
        return {
            (point.x + 1) / 2 * screen.x,
            (1 - point.y) / 2 * screen.y
        };
    }

    template <typename T>
    std::basic_string<T> LowerCase(const std::basic_string<T>& s) {
        std::basic_string<T> s2 = s;
        std::transform(s2.begin(), s2.end(), s2.begin(),
            [](const T v){ return static_cast<T>(std::tolower(v)); });
        return s2;
    }

    template <typename T>
    std::basic_string<T> UpperCase(const std::basic_string<T>& s) {
        std::basic_string<T> s2 = s;
        std::transform(s2.begin(), s2.end(), s2.begin(),
            [](const T v){ return static_cast<T>(std::toupper(v)); });
        return s2;
    }

    template<typename R>
    bool IsFutureReady(std::future<R> const& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }
}
