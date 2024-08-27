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

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "json.hpp"

#include <nfd.hpp>

#define RASTER_PACKAGED "packaged.raster."

#define print(expr) std::cout << expr << std::endl

#define DUMP_VAR(var) print(#var << " = " << (var))

#if defined(UNIX) && !defined(WIN32)
    #define RASTER_DL_EXPORT
#else
    #define RASTER_DL_EXPORT __declspec(dllexport)
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define SystemOpenURL(url) system(FormatString("%s %s", "start", (url).c_str()).c_str())
#elif __APPLE__
    #define SystemOpenURL(url)  system(FormatString("%s %s &&", "open", (url).c_str()).c_str())
#elif __linux__
    #define SystemOpenURL(url)  system(FormatString("%s %s &&", "xdg-open", (url).c_str()).c_str())
#else
    #error "Cannot determine valid SystemOpenURL implementation for your platform :("
#endif

#if defined(__GNUC__)
    #if defined(__clang__)
        #define COMPILER_FMT "Clang: %s"
    #else
        #define COMPILER_FMT "GNUC: %s"
    #endif
    #define COMPILER_VERSION __VERSION__
#elif defined(_MSC_VER)
    #define COMPILER_FMT "MSVC: %d"
    #define COMPILER_VERSION _MSC_FULL_VER
#else
    #error Cannot determine valid COMPILER_FMT / COMPILER_VERSION implementation for your compiler :(
#endif

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
    static std::basic_string<T> LowerCase(const std::basic_string<T>& s) {
        std::basic_string<T> s2 = s;
        std::transform(s2.begin(), s2.end(), s2.begin(),
            [](const T v){ return static_cast<T>(std::tolower(v)); });
        return s2;
    }

    template <typename T>
    static std::basic_string<T> UpperCase(const std::basic_string<T>& s) {
        std::basic_string<T> s2 = s;
        std::transform(s2.begin(), s2.end(), s2.begin(),
            [](const T v){ return static_cast<T>(std::toupper(v)); });
        return s2;
    }

    template<typename R>
    static bool IsFutureReady(std::future<R> const& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }
    
    template<class T>
    static T GetBaseName(T const & path, T const & delims = "/\\") {
        return path.substr(path.find_last_of(delims) + 1);
    }

    static float Precision( float f, int places ) {
        float n = std::pow(10.0f, places ) ;
        return std::round(f * n) / n ;
    }

    static float GetPercentageInBounds(float v, float min, float max) {
        return (v - min) / (max - min);
    }

    static std::string GetExtension(std::string t_path) {
        return t_path.substr(t_path.find_last_of("."));
    }

}
