#include <cstdio>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <random>
#include "zip/zip.h"

// what is a starter?
// - before running Raster, it needs to be unpacked from so-called PAK files
// - PAK files are essentially ZIP archives with some metadata included in them
// - the purpose of PAK files is to simplify distribution of custom plugins/nodes/attributes and etc.
// - starter creates hidden temporary folder to which all PAK files will be extracted

#if defined(UNIX) || defined(__linux__)
    #define EXECUTABLE_EXTENSION ""
#elif #defined(_WIN32)
    #define EXECUTABLE_EXTENSION "exe"
#endif

#define print(expr) std::cout << expr << std::endl

static std::random_device s_random_device;
static std::mt19937 s_random;
static std::uniform_int_distribution<std::mt19937::result_type> s_distribution;

int GetRandomInteger() {
    int value = std::abs((int) s_distribution(s_random));
    return value;
}

template<typename ... Args>
static std::string FormatString( const std::string& format, Args ... args ) {
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

int main(int argc, char** argv) {
    print("starting raster from " << std::filesystem::current_path());
    int randomID = GetRandomInteger();
    std::string temporaryFolderPath = FormatString("./.raster%i/", randomID);
    if (std::filesystem::exists(temporaryFolderPath)) {
        std::filesystem::remove_all(temporaryFolderPath);
    }

    if (!std::filesystem::exists(temporaryFolderPath)) {
        std::filesystem::create_directory(temporaryFolderPath);
    }

    std::string paksFolder = "./paks/";
    if (!std::filesystem::exists(paksFolder + "core.pak")) {
        print("paks/core.pak file is not found!");
        print("paks/core.pak contains core libraries and the binary of Raster itself");
        print("without paks/core.pak starter has nothing to execute");
        return 1;
    }
    auto paksIterator = std::filesystem::recursive_directory_iterator(paksFolder);
    for (auto& entry : paksIterator) {
        print("extracting " << entry.path().string());
        zip_extract(entry.path().c_str(), temporaryFolderPath.c_str(), nullptr, nullptr);
    }

    // getchar();

    auto originalCwd = std::filesystem::current_path();
    try {
        std::string targetExecutable = temporaryFolderPath + "raster_core" + EXECUTABLE_EXTENSION;
        if (std::filesystem::exists(targetExecutable)) {
            std::filesystem::current_path(temporaryFolderPath);
            std::flush(std::cout);
            #if defined(UNIX) || defined(__linux__)
                system("./raster_core");
            #elif #defined(_WIN32)
                system("raster_core");
            #endif
        } else {
            print("raster_core is not found! nothing to run");
        }
    } catch (...) {
        print("raster has crashed! exiting");
    }

    print("removing temporary files...");
    std::filesystem::current_path(originalCwd);
    std::filesystem::remove_all(temporaryFolderPath);

    return 0;
}