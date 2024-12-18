#include "dockspace.h"
#include "build_number.h"
#include "common/localization.h"
#include "common/plugins.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "gpu/gpu.h"
#include "common/ui_helpers.h"
#include "raster.h"

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
    static std::string s_projectPath;
    static bool s_preferencesPopupOpened = true;

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

            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_CIRCLE_INFO, Localization::GetString("ABOUT").c_str()).c_str())) {
                RenderAboutWindow();
                ImGui::EndMenu();
            }

            std::string rightAlignedText = FormatString("%s %i FPS (%0.2f ms)", ICON_FA_GEARS, (int) ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
            ImVec2 rightAlignedTextSize = ImGui::CalcTextSize(rightAlignedText.c_str());
            ImGui::SetCursorPosX(ImGui::GetWindowSize().x - rightAlignedTextSize.x - ImGui::GetStyle().WindowPadding.x);
            ImGui::Text("%s", rightAlignedText.c_str());
            ImGui::EndMainMenuBar();
        }

        if (openProjectInfoEditor) {
            ImGui::OpenPopup("##projectInfoEditor");
            s_project = Project();
        }

        if (openPreferencesModal) {
            ImGui::OpenPopup(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("PREFERENCES").c_str()).c_str());
        }

        RenderNewProjectPopup();
        RenderPreferencesModal();

        ImGui::End();
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
        RASTER_PACKAGED "preferences"  
    };

    void DockspaceUI::RenderPreferencesModal() {
        static std::string s_selectedPluginPacakge = RASTER_PACKAGED "preferences";
        static ImVec2 s_pluginButtonSize(150, 0);
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