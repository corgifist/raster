#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "typedefs.h"

namespace Raster {
    struct Libraries {
        static std::unordered_map<std::string, internalDylib> s_registry;

        static void LoadLibrary(std::string t_path, std::string t_key) {
            if (s_registry.find(t_key) != s_registry.end()) return; // do not reload libraries
            s_registry[t_key] = internalDylib(t_path, t_key);
        }

        template<typename T>
        static T* GetFunction(std::string t_key, std::string t_name) {
            if (s_registry.find(t_key) == s_registry.end())
                LoadLibrary(".", t_key);
            internalDylib* d = &s_registry[t_key];
            return d->get_function<T>(t_name.c_str());
        }

        template<typename T>
        static T& GetVariable(std::string t_key, std::string t_name) {
            if (s_registry.find(t_key) == s_registry.end())
                LoadLibrary(".", t_key);
            internalDylib* d = &s_registry[t_key];
            return d->get_variable<T>(t_name.c_str());
        }
    };
};