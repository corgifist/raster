#include "dockspace.h"
#include "build_number.h"
#include "common/localization.h"
#include "common/plugins.h"
#include "common/randomizer.h"
#include "common/user_interface.h"
#include "common/workspace.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "gpu/gpu.h"
#include "common/ui_helpers.h"
#include "nfd/nfd.h"
#include "nfd/nfd.hpp"
#include "raster.h"
#include <filesystem>
#include "../ImGui/imgui_stdlib.h"
#include "common/layouts.h"
#include "common/waveform_manager.h"

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

    void DockspaceUI::RenderAboutWindow() {
        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_CIRCLE_INFO, Localization::GetString("ABOUT").c_str()).c_str());
        ImGui::Text("%s %s: %s (%i)", ICON_FA_SCREWDRIVER, Localization::GetString("BUILD_NUMBER").c_str(), NumberToHexadecimal(BUILD_NUMBER).c_str(), BUILD_NUMBER);
        ImGui::Text("%s %s: %s", ICON_FA_GEARS, Localization::GetString("COMPILER_VERSION").c_str(), RASTER_COMPILER_VERSION_STRING);
        ImGui::Text("%s %s: %s", ICON_FA_IMAGE, Localization::GetString("GRAPHICS_API").c_str(), GPU::info.version.c_str());
    }

    static Project s_project;
    static std::string s_projectPath = "";
    static bool s_preferencesPopupOpened = true;
    static bool s_layoutsPopupOpened = true;

    Json DockspaceUI::AbstractSerialize() {
        return {};
    }

    void DockspaceUI::AbstractLoad(Json t_data) {
        
    }

    void DockspaceUI::AbstractRender() {
        auto viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowBgAlpha(0.0f);

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

        ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - ImGui::CalcTextSize(Localization::GetString("GET_STARTED_TEXT").c_str()) / 2.0f);
        if (ImGui::Button(FormatString("%s %s", ICON_FA_WINDOW_RESTORE, Localization::GetString("GET_STARTED_TEXT").c_str()).c_str())) {
            ImGui::OpenPopup("##openWindow");
        }

        ImGuiID dockspace_id = ImGui::GetID("DockSpace");
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode, nullptr);

        if (ImGui::BeginPopup("##openWindow")) {
            static std::string s_windowSearchBar = "";
            ImGui::InputTextWithHint("##windowSearch", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_windowSearchBar);
            if (ImGui::BeginChild("##windowsList", ImVec2(ImGui::GetContentRegionAvail().x, RASTER_PREFERRED_POPUP_HEIGHT))) {
                bool hasCandidates = false;
                for (auto& implementation : UserInterfaces::s_implementations) {
                    if (!s_windowSearchBar.empty() && LowerCase(implementation.description.prettyName).find(LowerCase(s_windowSearchBar)) == std::string::npos) continue;
                    hasCandidates = true;
                    if (ImGui::MenuItem(FormatString("%s %s", implementation.description.icon.c_str(), implementation.description.prettyName.c_str()).c_str())) {
                        for (auto& layout : Workspace::s_configuration.layouts) {
                            if (layout.id == Workspace::s_configuration.selectedLayout) {
                                auto userInterfaceCandidate = UserInterfaces::InstantiateUserInterface(implementation.description.packageName);
                                if (userInterfaceCandidate) {
                                    layout.windows.push_back(*userInterfaceCandidate);
                                    break;
                                }
                            }
                        }
                    }
                }
                if (!hasCandidates) {
                    UIHelpers::RenderNothingToShowText();
                }
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }

        bool openProjectInfoEditor = false;
        bool openPreferencesModal = false;
        bool openLayoutEditorModal = false;
        auto& configuration = Workspace::s_configuration;
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_FOLDER, Localization::GetString("PROJECT").c_str()).c_str())) {
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("NEW_PROJECT").c_str()).c_str(), "Ctrl+N")) {
                    s_projectPath = FormatString("%s/%s%i.raster", GetHomePath().c_str(), Localization::GetString("NEW_PROJECT").c_str(), Randomizer::GetRandomInteger());
                    openProjectInfoEditor = true;
                }
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_FOLDER_OPEN, Localization::GetString("OPEN_PROJECT").c_str()).c_str(), "Ctrl+O")) {
                    NFD::UniquePath path;
                    nfdwindowhandle_t window;
                    GPU::GetNFDWindowHandle(&window);
                    static nfdfilteritem_t s_filters[] = {
                        {"Raster Project", "raster"}
                    };
                    nfdresult_t result = NFD::OpenDialog(path, s_filters, 1, GetHomePath().c_str(), window);
                    if (result == NFD_OKAY) {
                        Workspace::OpenProject(path.get());
                    }
                }
                auto openRecentProjectText = configuration.lastProjectPath.empty() ?
                                            FormatString("%s %s", ICON_FA_FOLDER_CLOSED, Localization::GetString("OPEN_RECENT_PROJECT").c_str()) :
                                            FormatString("%s %s: '%s'", ICON_FA_FOLDER_OPEN, Localization::GetString("OPEN_RECENT_PROJECT").c_str(), configuration.lastProjectName.c_str());

                if (ImGui::MenuItem(FormatString("%s", openRecentProjectText.c_str()).c_str(), "Ctrl+Shift+O", false, !configuration.lastProjectPath.empty())) {
                    Workspace::OpenProject(configuration.lastProjectPath);
                }
                if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_FOLDER_OPEN, Localization::GetString("RECENT_PROJECTS").c_str()).c_str())) {
                    ImGui::SeparatorText(FormatString("%s %s", ICON_FA_FOLDER_OPEN, Localization::GetString("RECENT_PROJECTS").c_str()).c_str());
                    for (int i = configuration.recentProjects.size(); i --> 0;) {
                        if (ImGui::MenuItem(FormatString("%s %s (%s)", ICON_FA_FOLDER_OPEN, configuration.recentProjects[i][0].c_str(), configuration.recentProjects[i][1].c_str()).c_str())) {
                            Workspace::OpenProject(configuration.recentProjects[i][1]);
                        }
                    }
                    if (configuration.recentProjects.empty()) {
                        UIHelpers::RenderNothingToShowText();
                    }
                    ImGui::EndMenu();
                }
                std::string saveProject = Workspace::IsProjectLoaded() ? 
                        FormatString("%s %s '%s'", ICON_FA_FLOPPY_DISK, Localization::GetString("SAVE").c_str(), Workspace::GetProject().name.c_str()) :
                        FormatString("%s %s", ICON_FA_FLOPPY_DISK, Localization::GetString("SAVE_PROJECT").c_str());
                if (ImGui::MenuItem(saveProject.c_str(), "Ctrl+S", nullptr, Workspace::IsProjectLoaded())) {
                    Workspace::SaveProject();
                }
                auto saveProjectAsText = Workspace::IsProjectLoaded() ? 
                                            FormatString(Localization::GetString("SAVE_PROJECT_AS_FORMAT").c_str(), Workspace::GetProject().name.c_str()) :
                                            Localization::GetString("SAVE_PROJECT_AS");

                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_FLOPPY_DISK, saveProjectAsText.c_str()).c_str(), "Ctrl+Shift+S", false, Workspace::IsProjectLoaded())) {
                    auto& project = Workspace::GetProject();
                    auto reservedPackPath = project.packedProjectPath;
                    NFD::UniquePath path;
                    nfdwindowhandle_t window;
                    GPU::GetNFDWindowHandle(&window);
                    static nfdfilteritem_t s_projectFilters[] = {
                        {"Raster Project", "raster"}
                    };
                    nfdresult_t result = NFD::SaveDialog(path, s_projectFilters, 1, GetHomePath().c_str());
                    if (result == NFD_OKAY) {
                        project.packedProjectPath = path.get();
                        if (!StringEndsWith(project.packedProjectPath, ".raster")) {
                            project.packedProjectPath += ".raster";
                        }
                        Workspace::SaveProject();
                        project.packedProjectPath = reservedPackPath;
                    }
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

            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_WINDOW_RESTORE, Localization::GetString("WINDOW").c_str()).c_str())) {
                for (auto& implementation : UserInterfaces::s_implementations) {
                    if (ImGui::MenuItem(FormatString(ICON_FA_PLUS " %s %s %s", implementation.description.icon.c_str(), Localization::GetString("NEW").c_str(), implementation.description.prettyName.c_str()).c_str())) {
                        for (auto& layout : Workspace::s_configuration.layouts) {
                            if (layout.id == Workspace::s_configuration.selectedLayout) {
                                auto userInterfaceCandidate = UserInterfaces::InstantiateUserInterface(implementation.description.packageName);
                                if (userInterfaceCandidate) {
                                    layout.windows.push_back(*userInterfaceCandidate);
                                    break;
                                }
                            }
                        }
                    }
                }
                Plugins::RenderWindowPopup();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("TOOLS").c_str()).c_str())) {
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_WAVE_SQUARE, Localization::GetString("RECOMPUTE_ALL_AUDIO_WAVEFORMS").c_str()).c_str(), nullptr, false, Workspace::IsProjectLoaded())) {
                    auto& project = Workspace::GetProject();
                    for (auto& composition : project.compositions) {
                        WaveformManager::RequestWaveformRefresh(composition.id);
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_ALIGN_CENTER, Localization::GetString("LAYOUTS").c_str()).c_str())) {
                auto& configuration = Workspace::s_configuration;
                auto& selectedLayout = configuration.selectedLayout;
                for (auto& layout : configuration.layouts) {
                    auto& id = layout.id;
                    if (ImGui::MenuItem(FormatString("%s%s %s", selectedLayout == id ? ICON_FA_CHECK " " : "", ICON_FA_ALIGN_CENTER, layout.name.c_str()).c_str())) {
                        Layouts::LoadLayout(id);
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
        ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
        if (ImGui::BeginPopupModal(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT_LAYOUTS").c_str()).c_str(), &s_layoutsPopupOpened)) {
            auto& configuration = Workspace::s_configuration;
            static std::string s_searchFilter = "";
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x);
                ImGui::InputTextWithHint("##searchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_searchFilter);
            ImGui::PopItemWidth();
            ImGui::Separator();
            int index = 0;
            int targetRemoveIndex = -1;
            std::pair<int, int> targetSwapPair = {-1, -1};
            for (auto& layout : configuration.layouts) {
                std::string layoutName = layout.name;
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
                    Layouts::LoadLayout(layout.id);
                }
                ImGui::SameLine();
                ImGui::SetItemTooltip("%s %s", ICON_FA_SCREWDRIVER, Localization::GetString("LOAD_LAYOUT").c_str());
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x);
                ImGui::InputTextWithHint("##layoutName", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("LAYOUT_NAME").c_str()).c_str(), &layoutName);
                ImGui::PopItemWidth();
                layout.name = layoutName;
                ImGui::PopID();
                index++;
            }
            if (targetSwapPair.first >= 0 && targetSwapPair.second >= 0) {
                std::swap(configuration.layouts[targetSwapPair.first], configuration.layouts[targetSwapPair.second]);
            }    
            if (targetRemoveIndex > 0) {
                configuration.layouts.erase(configuration.layouts.begin() + targetRemoveIndex);
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
                    for (auto& selectedLayout : configuration.layouts) {
                        if (selectedLayout.id == configuration.selectedLayout) {
                            auto copiedLayout = Layout(selectedLayout.Serialize());
                            copiedLayout.id = Randomizer::GetRandomInteger();
                            copiedLayout.name = s_newLayoutName;
                            auto internalRasterFolder = GetHomePath() + "/.raster/";
                            if (!std::filesystem::exists(internalRasterFolder + "layouts/" + std::to_string(copiedLayout.id) + "/")) {
                                std::filesystem::create_directory(internalRasterFolder + "layouts/" + std::to_string(copiedLayout.id) + "/");
                            }
                            WriteFile(internalRasterFolder + "layouts/" + std::to_string(copiedLayout.id) + "/layout.ini", ReadFile(internalRasterFolder + "layouts/" + std::to_string(configuration.selectedLayout) + "/layout.ini"));
                            configuration.layouts.push_back(copiedLayout);
                            break;
                        }
                    }

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
                if (ImGui::Button(ICON_FA_FOLDER_OPEN)) {
                    NFD::UniquePath path;
                    nfdwindowhandle_t window;
                    GPU::GetNFDWindowHandle(&window);
                    static nfdfilteritem_t s_projectFilters[] = {
                        {"Raster Project", "raster"}
                    };
                    nfdresult_t result = NFD::SaveDialog(path, s_projectFilters, 1, GetHomePath().c_str());
                    if (result == NFD_OKAY) {
                        s_projectPath = path.get();
                        if (!StringEndsWith(s_projectPath, ".raster")) {
                            s_projectPath += ".raster";
                        }
                    }
                }
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x);
                ImGui::InputTextWithHint("##projectPath", FormatString("%s %s", ICON_FA_FOLDER_OPEN, Localization::GetString("PROJECT_PATH").c_str()).c_str(), &s_projectPath);
                ImGui::PopItemWidth();
                UIHelpers::RenderProjectEditor(s_project);
                if (s_projectPath.empty()) ImGui::BeginDisabled();
                if (ImGui::Button(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    Workspace::CreateEmptyProject(s_project, s_projectPath);
                    Workspace::OpenProject(s_projectPath);
                    ImGui::CloseCurrentPopup();
                }
                if (s_projectPath.empty()) ImGui::EndDisabled();
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