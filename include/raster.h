#pragma once

#define WIN32_LEAN_AND_MEAN

#include <chrono>
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
#include <variant>
#include <cstdarg>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "json.hpp"

#include "source_location.h"

#include "nfd/nfd.hpp"
#include "unordered_dense.h"

#define RASTER_PACKAGED "packaged.raster."

// TODO: replace this macro with RASTER_PRINT
//       cause currently print() can cause problems with third-party libraries

#define print(expr) std::cout << expr << std::endl

#define RASTER_FUNCTION_MACRO BOOST_CURRENT_LOCATION

#define RASTER_STRINGIFY(x) #x

#define RASTER_SOURCE_LOCATION RASTER_FUNCTION_MACRO

#define RASTER_LOG(x) print(RASTER_SOURCE_LOCATION << ": " << x)

#define DUMP_VAR(var) RASTER_LOG(#var << " = " << (var))

#define RASTER_PREFERRED_POPUP_WIDTH  180
#define RASTER_PREFERRED_POPUP_HEIGHT 200

#if defined(UNIX) || defined(__linux__)
    #define RASTER_DL_EXPORT
#elif defined(_WIN32)
    #define RASTER_DL_EXPORT __declspec(dllexport)
#endif

#if defined(UNIX) || defined(__linux__)
#include <dirent.h>
#elif defined(_WIN32)
#include "win_dirent.h"
#endif

#include <sys/stat.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define SystemOpenURL(url) system(FormatString("%s %s", "start", (url).c_str()).c_str())
    #define RASTER_PLATFORM_WINDOWS
#elif __APPLE__
    #define SystemOpenURL(url)  system(FormatString("%s \"%s\" &&", "open", (url).c_str()).c_str())
    #define RASTER_PLATFORM_APPLE
#elif __linux__
    #define SystemOpenURL(url)  system(FormatString("%s \"%s\" &&", "xdg-open", (url).c_str()).c_str())
    #define RASTER_PLATFORM_LINUX
#else
    #error "Cannot determine valid SystemOpenURL implementation for your platform :("
#endif

#if defined(__GNUC__)
    #if defined(__clang__)
        #define RASTER_COMPILER_NAME "clang"
    #else
        #define RASTER_COMPILER_NAME "gcc/g++"
    #endif
    #define RASTER_COMPILER_VERSION __VERSION__
#elif defined(_MSC_VER)
    #define RASTER_COMPILER_NAME "msvc"
    #define RASTER_COMPILER_VERSION _MSC_FULL_VER
#else
    #error Cannot determine valid COMPILER_FMT / COMPILER_VERSION implementation for your compiler :(
#endif

#if defined(UNIX) || defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#include <winbase.h>
#endif

// use this macro to get info about the compiler
#define RASTER_COMPILER_VERSION_STRING RASTER_COMPILER_NAME ": " RASTER_COMPILER_VERSION

#define ATTRIBUTE_TYPE(T) \
    std::type_index(typeid(T))

#define RASTER_COLOR32(R,G,B,A)    (((uint32_t)(A)<<24) | ((uint32_t)(B)<<16) | ((uint32_t)(G)<<8) | ((uint32_t)(R)<<0))

#define RASTER_TYPE_NAME(T) {std::type_index(typeid(T)), #T}

// wrapper for std::lock_guard
#define RASTER_SYNCHRONIZED(MUTEX) \
    std::lock_guard<std::mutex> __sync((MUTEX)); \

#define RASTER_GET_CONTEXT_VALUE(t_data, t_key, t_type) \
    ((t_data.find(t_key) != t_data.end()) ? std::any_cast<t_type>(t_data[t_key]) : t_type())

#define RASTER_SPAWN_ABSTRACT(ABSTRACT, T) \
    ((ABSTRACT) std::make_shared<T>())

namespace Raster {

    using Json = nlohmann::json;
    using namespace ankerl;
    
    static const std::string FormatString(std::string zcFormat, ...) {
        // initialize use of the variable argument array
        va_list vaArgs;
        va_start(vaArgs, zcFormat);

        // reliably acquire the size
        // from a copy of the variable argument array
        // and a functionally reliable call to mock the formatting
        va_list vaArgsCopy;
        va_copy(vaArgsCopy, vaArgs);
        const int iLen = std::vsnprintf(NULL, 0, zcFormat.c_str(), vaArgsCopy);
        va_end(vaArgsCopy);

        // return a formatted string without risking memory mismanagement
        // and without assuming any compiler or platform specific behavior
        std::vector<char> zc(iLen + 1);
        std::vsnprintf(zc.data(), zc.size(), zcFormat.c_str(), vaArgs);
        va_end(vaArgs);
        return std::string(zc.data(), iLen); 
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

	static float DecibelToLinear(float p_db) {
		return std::exp(p_db * (float)0.11512925464970228420089957273422);
	}

	static float LinearToDecibel(float p_linear) {
		return std::log(p_linear) * (float)8.6858896380650365530225783783321;
	}

    typedef std::chrono::high_resolution_clock clock;
    template <typename T>
    using duration = std::chrono::duration<T>;

    static void ExperimentalSleepFor(double dt)
    {
#define RASTER_USE_SPIN_LOCK
#ifdef RASTER_USE_SPIN_LOCK
        static constexpr duration<double> MinSleepDuration(0);
        clock::time_point start = clock::now();
        while (duration<double>(clock::now() - start).count() < dt) {
            std::this_thread::sleep_for(MinSleepDuration);
        }
#elif defined(RASTER_USE_COMBINED_SLEEP)
        using namespace std;
        using namespace std::chrono;

        auto seconds = dt;
        static double estimate = 5e-3;
        static double mean = 5e-3;
        static double m2 = 0;
        static int64_t count = 1;

        while (seconds > estimate) {
            auto start = high_resolution_clock::now();
            this_thread::sleep_for(milliseconds(1));
            auto end = high_resolution_clock::now();

            double observed = (end - start).count() / 1e9;
            seconds -= observed;

            ++count;
            double delta = observed - mean;
            mean += delta / count;
            m2   += delta * (observed - mean);
            double stddev = sqrt(m2 / (count - 1));
            estimate = mean + stddev;
        }

        // spin lock
        auto start = high_resolution_clock::now();
        while ((high_resolution_clock::now() - start).count() / 1e9 < seconds);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds((int) (dt * 1000)));
#endif
    }

    template< typename T >
    std::string NumberToHexadecimal( T i ) {
        return FormatString("%X", i);
    }

    static std::string GetHomePath() {
        #if defined(UNIX) || defined(__linux__)
            return getenv("HOME");
        #elif defined(_WIN32)
            return std::string(getenv("HOMEDRIVE")) + std::string(getenv("HOMEPATH"));
        #else
            return "~";
        #endif
    }

    static std::string RemoveExtension(const std::string& path) {
        if (path == "." || path == "..")
            return path;

        size_t pos = path.find_last_of("\\/.");
        if (pos != std::string::npos && path[pos] == '.')
            return path.substr(0, pos);

        return path;
    }

#if defined(UNIX) || defined(__linux__)
    #include <unistd.h>

    static unsigned long long GetRamAmount() {
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        return pages * page_size;
    }
#elif defined(_WIN32)

    static unsigned long long GetRamAmount()
    {
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        return status.ullTotalPhys;
    }
#else
    static unsigned long long GetRamAmount() {
        return 0;
    }
#endif


    inline bool StringEndsWith(std::string const & value, std::string const & ending)
    {
        if (ending.size() > value.size()) return false;
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

#ifdef RASTER_PLATFORM_WINDOWS
    static void SetEnv(std::string name, std::string data) {
        SetEnvironmentVariable(name.c_str(), data.c_str());
    } 
#elif defined(RASTER_PLATFORM_LINUX)
    static void SetEnv(std::string name, std::string data) {
        setenv(name.c_str(), data.c_str(), 0);
    } 
#endif

}
