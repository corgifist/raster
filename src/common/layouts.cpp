#include "common/layouts.h"
#include "common/workspace.h"
#include "../ImGui/imgui.h"

namespace Raster {
    static std::optional<std::string> s_layout = std::nullopt;

    std::optional<std::string> Layouts::GetRequestedLayout() {
        auto layout = s_layout;
        s_layout = std::nullopt;
        return layout;
    }

    void Layouts::RequestLayout(std::string t_path) {
        s_layout = t_path;
    }

    void Layouts::LoadLayout(int id) {
        auto& configuration = Workspace::s_configuration;
        int previousSelectedLayout = configuration.selectedLayout;
        configuration.selectedLayout = id;
        std::string layoutPath = GetHomePath() + "/.raster/layouts/" + std::to_string(id);
        if (!std::filesystem::exists(layoutPath)) {
            std::filesystem::create_directories(layoutPath);
        }
        if (!std::filesystem::exists(layoutPath + "/layout.ini")) {
            std::string previousLayoutPath = GetHomePath() + "/.raster/layouts/" + std::to_string(previousSelectedLayout);
            std::string newLayoutContent = "";
            if (std::filesystem::exists(previousLayoutPath + "/layout.ini")) {
                newLayoutContent = ReadFile(previousLayoutPath + "/layout.ini");
            }
            WriteFile(layoutPath + "/layout.ini", newLayoutContent);
        }
        Layouts::RequestLayout((layoutPath + "/layout.ini").c_str());
        static std::string s_persistentIniFilename;
        s_persistentIniFilename = layoutPath + "/layout.ini";
        ImGui::GetIO().IniFilename = s_persistentIniFilename.c_str();
    }

    static std::vector<int> s_targetDeleteWindows;

    void Layouts::DestroyWindow(int t_windowID) {
        s_targetDeleteWindows.push_back(t_windowID);
    }

    void Layouts::Update() {
        auto& configuration = Workspace::s_configuration;
        for (auto& id : s_targetDeleteWindows) {
            for (auto& layout : configuration.layouts) {
                int targetEraseIndex = -1;
                int i = 0;
                for (auto& window : layout.windows) {
                    i++;
                    if (window->id == id) {
                        targetEraseIndex = i - 1;
                        break;
                    }
                }
                if (targetEraseIndex >= 0) {
                    layout.windows.erase(layout.windows.begin() + targetEraseIndex);
                }
            }
        }

        s_targetDeleteWindows.clear();
    }
};