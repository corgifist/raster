#include "dockspace.h"
#include "build_number.h"
#include "common/localization.h"
#include "common/plugins.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "gpu/gpu.h"
#include "common/ui_helpers.h"
#include "raster.h"
#include <filesystem>
#include "../../ImGui/imgui_stdlib.h"
#include "common/layouts.h"

#define LAYOUT_DRAG_DROP_PAYLOAD "LAYOUT_DRAG_DROP_PAYLOAD"

namespace Raster {

    int ImFormatString(char* buf, size_t buf_size, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
    #ifdef IMGUI_USE_STB_SPRINTF
        int w = stbsp_vsnprintf(buf, (int)buf_size, fmt, args);
    #else
        int w = vsnprintf(buf, buf_size, fmt, args);
    #endif
        va_end(args);
        if (buf == NULL)
            return w;
        if (w == -1 || w >= (int)buf_size)
            w = (int)buf_size - 1;
        buf[w] = 0;
        return w;
    }

    static Json CreateLayout(std::string t_name) {
        return {
            {"ID", Randomizer::GetRandomInteger()},
            {"Name", t_name}
        };
    }

    static void LoadLayout(int id) {
        auto& preferences = Workspace::s_configuration.GetPluginData(RASTER_PACKAGED "preferences");
        int previousSelectedLayout = preferences["SelectedLayout"];
        preferences["SelectedLayout"] = id;
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

    void DockspaceUI::RenderAboutWindow() {
        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_CIRCLE_INFO, Localization::GetString("ABOUT").c_str()).c_str());
        ImGui::Text("%s %s: %s (%i)", ICON_FA_SCREWDRIVER, Localization::GetString("BUILD_NUMBER").c_str(), NumberToHexadecimal(BUILD_NUMBER).c_str(), BUILD_NUMBER);
        ImGui::Text("%s %s: %s", ICON_FA_GEARS, Localization::GetString("COMPILER_VERSION").c_str(), RASTER_COMPILER_VERSION_STRING);
        ImGui::Text("%s %s: %s", ICON_FA_IMAGE, Localization::GetString("GRAPHICS_API").c_str(), GPU::info.version.c_str());
    }

    static Project s_project;
    static std::string s_projectPath;
    static bool s_preferencesPopupOpened = true;
    static bool s_layoutsPopupOpened = true;

    void DockspaceUI::Render() {
        auto viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_window_flags = 0;
        host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
        host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        
        // host_window_flags |= ImGuiWindowFlags_NoBackground;

        char label[32];
        ImFormatString(label, IM_ARRAYSIZE(label), "DockSpaceViewport_%08X", viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(label, NULL, host_window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("DockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0, nullptr);

        bool openProjectInfoEditor = false;
        bool openPreferencesModal = false;
        bool openLayoutEditorModal = false;

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_FOLDER, Localization::GetString("PROJECT").c_str()).c_str())) {
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("NEW_PROJECT").c_str()).c_str(), "Ctrl+N")) {
                    NFD::UniquePath path;
                    nfdwindowhandle_t window;
                    GPU::GetNFDWindowHandle(&window);
                    nfdresult_t result = NFD::PickFolder(path, nullptr, window);
                    if (result == NFD_OKAY) {
                        s_projectPath = path.get();
                        openProjectInfoEditor = true;
                    }
                }
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_FOLDER_OPEN, Localization::GetString("OPEN_PROJECT").c_str()).c_str(), "Ctrl+O")) {
                    NFD::UniquePath path;
                    nfdwindowhandle_t window;
                    GPU::GetNFDWindowHandle(&window);
                    nfdresult_t result = NFD::PickFolder(path, nullptr, window);
                    if (result == NFD_OKAY) {
                        if (std::filesystem::exists(std::string(path.get()) + "/project.json")) {
                                Workspace::s_project = Project(ReadJson(std::string(path.get()) + "/project.json"));
                                Workspace::s_project.value().path = std::string(path.get()) + "/";
                        }
                    }
                }
                std::string saveProject = Workspace::IsProjectLoaded() ? 
                        FormatString("%s %s '%s'", ICON_FA_FLOPPY_DISK, Localization::GetString("SAVE").c_str(), Workspace::GetProject().name.c_str()) :
                        FormatString("%s %s", ICON_FA_FLOPPY_DISK, Localization::GetString("SAVE_PROJECT").c_str());
                if (ImGui::MenuItem(saveProject.c_str(), "Ctrl+S", nullptr, Workspace::IsProjectLoaded())) {
                    auto& project = Workspace::GetProject();
                    WriteFile(project.path + "/project.json", project.Serialize().dump());
                }
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("PREFERENCES").c_str()).c_str())) {
                    openPreferencesModal = true;
                    s_preferencesPopupOpened = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("EXIT_RASTER").c_str()).c_str())) {
                    std::exit(0);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_ALIGN_CENTER, Localization::GetString("LAYOUTS").c_str()).c_str())) {
                auto& preferences = Workspace::s_configuration.GetPluginData(RASTER_PACKAGED "preferences");
                for (auto& layout : preferences["Layouts"]) {
                    int id = layout["ID"];
                    if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_ALIGN_CENTER, layout["Name"].get<std::string>().c_str()).c_str())) {
                        LoadLayout(id);
                    }
                }
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT_LAYOUTS").c_str()).c_str())) {
                    openLayoutEditorModal = true;
                    s_layoutsPopupOpened = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_CIRCLE_INFO, Localization::GetString("ABOUT").c_str()).c_str())) {
                RenderAboutWindow();
                ImGui::EndMenu();
            }

            std::string rightAlignedText = FormatString("%s %i FPS (%0.2f ms)", ICON_FA_GEARS, (int) ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
            ImVec2 rightAlignedTextSize = ImGui::CalcTextSize(rightAlignedText.c_str());
            float fpsCursorX =ImGui::GetWindowSize().x - rightAlignedTextSize.x - ImGui::GetStyle().WindowPadding.x;
            ImGui::SetCursorPosX(fpsCursorX);
            ImGui::Text("%s", rightAlignedText.c_str());

            std::string bpcText = FormatString("%s %s %s", ICON_FA_DROPLET, Workspace::IsProjectLoaded() ? std::to_string(static_cast<int>(Workspace::GetProject().colorPrecision)).c_str() : "-", Localization::GetString("bpc").c_str());
            ImVec2 bpcTextSize = ImGui::CalcTextSize(bpcText.c_str());
            ImGui::SetCursorPosX(fpsCursorX - ImGui::GetStyle().FramePadding.x - bpcTextSize.x);
            if (ImGui::BeginMenu(bpcText.c_str())) {
                if (!Workspace::IsProjectLoaded()) {
                    UIHelpers::RenderNothingToShowText();
                } else {
                    auto& project = Workspace::GetProject();
                    ImGui::SeparatorText(FormatString("%s %s", ICON_FA_DROPLET, Localization::GetString("BPC").c_str()).c_str());
                    if (ImGui::MenuItem(FormatString("%s %i %s (%s)", ICON_FA_DROPLET, 8, Localization::GetString("BITS").c_str(), Localization::GetString("BYTE").c_str()).c_str())) {
                        project.colorPrecision = ProjectColorPrecision::Usual;
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem(FormatString("%s %i %s (%s)", ICON_FA_DROPLET, 16, Localization::GetString("BITS").c_str(), Localization::GetString("HALF_FLOAT").c_str()).c_str())) {
                        project.colorPrecision = ProjectColorPrecision::Half;
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::MenuItem(FormatString("%s %i %s (%s)", ICON_FA_DROPLET, 32, Localization::GetString("BITS").c_str(), Localization::GetString("FULL_FLOAT").c_str()).c_str())) {
                        project.colorPrecision = ProjectColorPrecision::Full;
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (openProjectInfoEditor) {
            ImGui::OpenPopup("##projectInfoEditor");
            s_project = Project();
        }

        if (openPreferencesModal) {
            ImGui::OpenPopup(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("PREFERENCES").c_str()).c_str());
        }

        if (openLayoutEditorModal) {
            ImGui::OpenPopup(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT_LAYOUTS").c_str()).c_str());
        }

        RenderNewProjectPopup();
        RenderPreferencesModal();
        RenderLayoutsModal();

        ImGui::End();
    }

    void DockspaceUI::RenderLayoutsModal() {
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
        if (ImGui::BeginPopupModal(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT_LAYOUTS").c_str()).c_str(), &s_layoutsPopupOpened)) {
            auto& preferences = Workspace::s_configuration.GetPluginData(RASTER_PACKAGED "preferences");
            static std::string s_searchFilter = "";
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x);
                ImGui::InputTextWithHint("##searchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_searchFilter);
            ImGui::PopItemWidth();
            ImGui::Separator();
            int index = 0;
            int targetRemoveIndex = -1;
            std::pair<int, int> targetSwapPair = {-1, -1};
            for (auto& layout : preferences["Layouts"]) {
                std::string layoutName = layout["Name"];
                if (!s_searchFilter.empty() && LowerCase(layoutName).find(LowerCase(s_searchFilter)) == std::string::npos) {
                    index++;
                    continue;
                }
                ImGui::PushID(index);
                if (ImGui::Button(ICON_FA_LIST)) {}
                ImGui::SameLine();
                ImGui::SetItemTooltip("%s %s", ICON_FA_LIST, Localization::GetString("REORDER_LAYOUTS").c_str());
                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload(LAYOUT_DRAG_DROP_PAYLOAD, &index, sizeof(index));
                    ImGui::Text("%s %s", ICON_FA_ALIGN_CENTER, layoutName.c_str());
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(LAYOUT_DRAG_DROP_PAYLOAD)) {
                        int from = *((int*) payload->Data);
                        int to = index;
                        targetSwapPair = {from, to};
                    }
                    ImGui::EndDragDropTarget();
                }
                if (ImGui::Button(ICON_FA_TRASH_CAN)) {
                    targetRemoveIndex = index;
                }
                ImGui::SetItemTooltip("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("REMOVE_LAYOUT").c_str());
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_SCREWDRIVER)) {
                    LoadLayout(layout["ID"]);
                }
                ImGui::SameLine();
                ImGui::SetItemTooltip("%s %s", ICON_FA_SCREWDRIVER, Localization::GetString("LOAD_LAYOUT").c_str());
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x);
                ImGui::InputTextWithHint("##layoutName", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("LAYOUT_NAME").c_str()).c_str(), &layoutName);
                ImGui::PopItemWidth();
                layout["Name"] = layoutName;
                ImGui::PopID();
                index++;
            }
            if (targetSwapPair.first >= 0 && targetSwapPair.second >= 0) {
                std::swap(preferences["Layouts"][targetSwapPair.first], preferences["Layouts"][targetSwapPair.second]);
            }    
            if (targetRemoveIndex > 0) {
                preferences["Layouts"].erase(targetRemoveIndex);
            }
            static std::string s_newLayoutName = "New Layout";
            if (UIHelpers::CenteredButton(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("ADD_NEW_LAYOUT").c_str()).c_str())) {
                ImGui::OpenPopup("##newLayout");
                s_newLayoutName = "New Layout";
            }
            if (ImGui::BeginPopup("##newLayout")) {
                ImGui::InputTextWithHint("##newLayoutName", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("LAYOUT_NAME").c_str()).c_str(), &s_newLayoutName);
                ImGui::SameLine();
                if (ImGui::Button(Localization::GetString("OK").c_str()) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                    preferences["Layouts"].push_back(CreateLayout(s_newLayoutName));
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (index == 0) {
                UIHelpers::RenderNothingToShowText();
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();
    }

    void DockspaceUI::RenderNewProjectPopup() {
        static ImVec2 s_windowSize(0, 0);
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Size / 2.0f - s_windowSize / 2.0f);
        if (ImGui::BeginPopupModal("##projectInfoEditor", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
            if (ImGui::BeginChild("##projectSettingsContainer", ImVec2(RASTER_PREFERRED_POPUP_WIDTH * 2.5f, 0), ImGuiChildFlags_AutoResizeY)) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("PROJECT_INFO").c_str()).c_str());
                UIHelpers::RenderProjectEditor(s_project);
                if (ImGui::Button(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    s_project.path = s_projectPath;
                    Workspace::s_project = s_project;
                }
                if (ImGui::Button(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("CANCEL").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndChild();
            s_windowSize = ImGui::GetWindowSize();
            ImGui::EndPopup();
        }
    }

    static std::vector<std::string> s_pinnedPlugins = {
        RASTER_PACKAGED "preferences",
        RASTER_PACKAGED "rendering"
    };

    void DockspaceUI::RenderPreferencesModal() {
        static std::string s_selectedPluginPacakge = RASTER_PACKAGED "preferences";
        static ImVec2 s_pluginButtonSize(200, 0);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
        if (ImGui::BeginPopupModal(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("PREFERENCES").c_str()).c_str(), &s_preferencesPopupOpened)) {
                ImGui::BeginGroup();
                    for (auto& pinnedPlugin : s_pinnedPlugins) {
                        auto pluginCandidate = Plugins::GetPluginByPackageName(pinnedPlugin);
                        if (pluginCandidate) {
                            auto& plugin = *pluginCandidate;
                            if (ImGui::Button(FormatString("%s %s", plugin->Icon().c_str(), plugin->Name().c_str()).c_str(), s_pluginButtonSize)) {
                                s_selectedPluginPacakge = plugin->PackageName();
                            }
                        }
                    }
                    for (auto& plugin : Plugins::s_plugins) {
                        if (std::find(s_pinnedPlugins.begin(), s_pinnedPlugins.end(), plugin->PackageName()) != s_pinnedPlugins.end()) continue;
                        if (ImGui::Button(FormatString("%s %s", plugin->Icon().c_str(), plugin->Name().c_str()).c_str(), s_pluginButtonSize)) {
                            s_selectedPluginPacakge = plugin->PackageName();
                        }
                    }
                ImGui::EndGroup();
                ImGui::SameLine(0, 20);
                if (ImGui::BeginChild("##pluginProperties", ImGui::GetContentRegionAvail())) {
                    auto selectedPluginCandidate = Plugins::GetPluginByPackageName(s_selectedPluginPacakge);
                    if (selectedPluginCandidate) {
                        auto& plugin = *selectedPluginCandidate;
                        ImGui::PushFont(Font::s_denseFont);
                        ImGui::SetWindowFontScale(1.7f);
                            ImGui::Text("%s %s", plugin->Icon().c_str(), plugin->Name().c_str());
                        ImGui::SetWindowFontScale(1.0f);
                        ImGui::PopFont();
                        ImGui::Text("%s %s", ICON_FA_MESSAGE, plugin->Description().c_str());
                        ImGui::Separator();
                        plugin->RenderProperties();

                        std::string footerText = FormatString("%s %s: %s", ICON_FA_BOX_OPEN, Localization::GetString("PACKAGE_NAME").c_str(), plugin->PackageName().c_str());
                        ImVec2 footerSize = ImGui::CalcTextSize(footerText.c_str());
                        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - footerSize.x / 2.0f);
                        ImGui::Text("%s", footerText.c_str());
                    }
                }
                ImGui::EndChild();
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();
    }
};