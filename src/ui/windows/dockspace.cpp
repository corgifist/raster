#include "dockspace.h"

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
        static std::string s_projectPath;

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_FOLDER, Localization::GetString("PROJECT").c_str()).c_str())) {
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("NEW_PROJECT").c_str()).c_str(), "Ctrl+N")) {
                    NFD::UniquePath path;
                    nfdresult_t result = NFD::PickFolder(path);
                    if (result == NFD_OKAY) {
                        s_projectPath = path.get();
                        openProjectInfoEditor = true;
                    }
                }
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_FOLDER_OPEN, Localization::GetString("OPEN_PROJECT").c_str()).c_str(), "Ctrl+O")) {
                    NFD::UniquePath path;
                    nfdresult_t result = NFD::PickFolder(path);
                    if (result == NFD_OKAY) {
                        if (std::filesystem::exists(std::string(path.get()) + "/project.json")) {
                                Workspace::s_project = Project(ReadJson(std::string(path.get()) + "/project.json"));
                                Workspace::s_project.value().path = std::string(path.get()) + "/";
                        }
                    }
                }
                std::string saveProject = Workspace::IsProjectLoaded() ? 
                        FormatString("%s %s '%s'", ICON_FA_FLOPPY_DISK, Localization::GetString("SAVE").c_str(), Workspace::GetProject().name.c_str()) :
                        Localization::GetString("SAVE_PROJECT");
                if (ImGui::MenuItem(saveProject.c_str(), "Ctrl+S", nullptr, Workspace::IsProjectLoaded())) {
                    auto& project = Workspace::GetProject();
                    WriteFile(project.path + "/project.json", project.Serialize().dump());
                }
                ImGui::Separator();
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("EXIT_RASTER").c_str()).c_str())) {
                    std::exit(0);
                }
                ImGui::EndMenu();
            }

            std::string rightAlignedText = FormatString("%s %i FPS (%0.2f ms)", ICON_FA_GEARS, (int) ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
            ImVec2 rightAlignedTextSize = ImGui::CalcTextSize(rightAlignedText.c_str());
            ImGui::SetCursorPosX(ImGui::GetWindowSize().x - rightAlignedTextSize.x - ImGui::GetStyle().WindowPadding.x);
            ImGui::Text("%s", rightAlignedText.c_str());
            ImGui::EndMainMenuBar();
        }

        static std::string s_projectName, s_projectDescription;
        static glm::vec2 s_projectResolution;
        static glm::vec4 s_backgroundColor;

        if (openProjectInfoEditor) {
            ImGui::OpenPopup("##projectInfoEditor");
            s_projectName = "New Project";
            s_projectDescription = "Empty Project";
            s_projectResolution = glm::vec2(1280, 720);
            s_backgroundColor = glm::vec4(0, 0, 0, 1);
        }

        if (ImGui::BeginPopupModal("##projectInfoEditor", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {

            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("PROJECT_INFO").c_str()).c_str());

            static std::vector<float> s_cursorCandidates;

            float largestCursorX = 0;
            for (auto& cursor : s_cursorCandidates) {
                if (cursor > largestCursorX) largestCursorX = cursor;
            }

            s_cursorCandidates.clear();

            ImGui::Text("%s %s", ICON_FA_PENCIL, Localization::GetString("PROJECT_NAME").c_str());
            ImGui::SameLine();
            s_cursorCandidates.push_back(ImGui::GetCursorPosX());
            if (largestCursorX != 0) ImGui::SetCursorPosX(largestCursorX);
            ImGui::InputText("##projectName", &s_projectName);


            ImGui::Text("%s %s", ICON_FA_PENCIL, Localization::GetString("PROJECT_DESCRIPTION").c_str());
            ImGui::SameLine();
            s_cursorCandidates.push_back(ImGui::GetCursorPosX());
            if (largestCursorX != 0) ImGui::SetCursorPosX(largestCursorX);
            ImGui::InputTextMultiline("##projectDescription", &s_projectDescription);

            ImGui::Text("%s %s", ICON_FA_EXPAND, Localization::GetString("PROJECT_RESOLUTION").c_str());
            ImGui::SameLine();
            s_cursorCandidates.push_back(ImGui::GetCursorPosX());
            if (largestCursorX != 0) ImGui::SetCursorPosX(largestCursorX);
            ImGui::DragFloat2("##projectResolution", glm::value_ptr(s_projectResolution), 1, 0, 0, "%0.0f");

            ImGui::Text("%s %s", ICON_FA_DROPLET, Localization::GetString("PROJECT_BACKGROUND_COLOR").c_str());
            ImGui::SameLine();
            s_cursorCandidates.push_back(ImGui::GetCursorPosX());
            if (largestCursorX != 0) ImGui::SetCursorPosX(largestCursorX);
            ImGui::ColorEdit4("##bgColor", glm::value_ptr(s_backgroundColor));

            if (ImGui::Button(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                Project result;

                result.name = s_projectName;
                result.description = s_projectDescription;
                result.preferredResolution = s_projectResolution;
                result.path = s_projectPath;
                result.compositions.push_back(Composition());
                result.selectedCompositions = {result.compositions.back().id};
                result.backgroundColor = s_backgroundColor;

                Workspace::s_project = result;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("CANCEL").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetWindowPos(ImGui::GetMainViewport()->Size / 2.0f - ImGui::GetWindowSize() / 2.0f);
            ImGui::EndPopup();
        }



        ImGui::End();
    }
};