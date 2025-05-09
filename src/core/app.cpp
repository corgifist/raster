#include "app.h"
#include "common/configuration.h"
#include "common/project_color_precision.h"
#include "common/user_interface.h"
#include "dockspace.h"
#include "gpu/gpu.h"
#include "gpu/async_upload.h"
#include "font/font.h"
#include "common/common.h"
#include "build_number.h"
#include "common/ui_shared.h"
#include "compositor/compositor.h"
#include "common/node_category.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_freetype.h"
#include "../avcpp/av.h"
#include "../avcpp/ffmpeg.h"
#include "../avcpp/avutils.h"
#include "audio/audio.h"
#include "compositor/async_rendering.h"
#include "common/audio_info.h"
#include "common/ui_helpers.h"
#include "common/plugins.h"
#include "raster.h"
#include "common/layouts.h"
#include "common/rendering.h"
#include "common/waveform_manager.h"
#include "common/dispatchers.h"
#include "common/audio_memory_management.h"
#include "common/examples.h"
#include "common/color_management.h"
#include "../ImGui/ImGuizmo.h"

using namespace av;

namespace Raster {
    static std::thread s_writerThread;
    static bool s_writerThreadRunning = true;

    void App::Start() {
        print(RASTER_COMPILER_VERSION_STRING);
        RASTER_LOG("available RAM in bytes: " << GetRamAmount());
        RASTER_LOG("available RAM in megabytes: " << GetRamAmount() / 1024 / 1024);
        RASTER_LOG("available RAM in gigabytes: " << GetRamAmount() / 1024 / 1024 / 1024);
        static NFD::Guard s_guard;
        av::init();
        av::set_logging_level(AV_LOG_ERROR);
        RASTER_LOG("ffmpeg version: " << av::getversion());

        s_writerThread = std::thread(App::WriterThread);

        DUMP_VAR(Audio::s_backendInfo.name);
        DUMP_VAR(Audio::s_backendInfo.version);
        GPU::Initialize();
        ImGui::SetCurrentContext((ImGuiContext*) GPU::GetImGuiContext());
        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        GPU::InitializeImGui();
        ColorManagement::Initialize();
        AsyncUpload::Initialize();
        AsyncRendering::Initialize();
        GPU::SetRenderingFunction(App::RenderLoop);
        GPU::StartRenderingThread();
        Terminate();
    }

    void App::WriterThread() {
        while (s_writerThreadRunning) {
            Plugins::WriteConfigs();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void App::InitializeInternals() {
        Plugins::Initialize();
        Plugins::SetupUI();
        Plugins::EarlyInitialize();

        DefaultNodeCategories::Initialize();
        Dispatchers::Initialize();
        Workspace::Initialize();
        Plugins::WorkspaceInitialize();

        AudioMemoryManagement::Initialize(1024 * 1024 * 1);
        WaveformManager::Initialize();

        auto& io = ImGui::GetIO();
        auto& configuration = Workspace::s_configuration;
        int currentLayoutID = configuration.selectedLayout;
        if (currentLayoutID > 0) {
            static std::string targetLayoutPath = GetHomePath() + "/.raster/layouts/" + std::to_string(currentLayoutID) + "/layout.ini";
            if (std::filesystem::exists(targetLayoutPath)) {
                io.IniFilename = targetLayoutPath.c_str();
                Layouts::RequestLayout(io.IniFilename);
                RASTER_LOG("loading layout " << io.IniFilename);
            }
        } else {
            RASTER_LOG("no layout was applied");
        }

        Plugins::LateInitialize();
        Examples::Initialize();

        ImFontConfig fontCfg = {};

        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};

        fontCfg.RasterizerDensity = 1.5f;
        Font::s_normalFont = io.Fonts->AddFontFromMemoryCompressedTTF(
            Font::s_fontBytes.data(), Font::s_fontSize,
            16.0f, &fontCfg, io.Fonts->GetGlyphRangesCyrillic()
        );

        fontCfg.MergeMode = true;
        fontCfg.GlyphMinAdvanceX = 16.0f * 0.85f;
        io.Fonts->AddFontFromMemoryCompressedTTF(
            Font::s_fontAwesomeBytes.data(), Font::s_fontAwesomeSize,
            16.0f * 0.85f, &fontCfg, icons_ranges
        );

        fontCfg.RasterizerDensity = 5;

        fontCfg.MergeMode = false;
        fontCfg.GlyphMinAdvanceX = 0;
        Font::s_denseFont = io.Fonts->AddFontFromMemoryCompressedTTF(
                    Font::s_fontBytes.data(), Font::s_fontSize,
                    16.0f, &fontCfg, io.Fonts->GetGlyphRangesCyrillic());

        fontCfg.MergeMode = true;
        fontCfg.GlyphMinAdvanceX = 16.0f * 0.85f;
        io.Fonts->AddFontFromMemoryCompressedTTF(
            Font::s_fontAwesomeBytes.data(), Font::s_fontAwesomeSize,
            16.0f * 0.85f, &fontCfg, icons_ranges
        ); 

        auto& style = ImGui::GetStyle();
        // style.CurveTessellationTol = 0.01f;
        style.SeparatorTextAlign = ImVec2(0.5, 0.5);
        style.ScrollbarSize = 10;
	
        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.7f;
        style.WindowPadding = ImVec2(12, 12);
        style.WindowRounding = 2;
        style.WindowBorderSize = 1.0f;
        style.WindowMinSize = ImVec2(20.0f, 20.0f);
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Right;
        style.ChildRounding = 0;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 2.0f;
        style.PopupBorderSize = 1.0f;
        style.FramePadding = ImVec2(8.0f, 3);
        style.FrameRounding = 2;
        style.FrameBorderSize = 1.0f;
        style.ItemSpacing = ImVec2(4, 6);
        style.ItemInnerSpacing = ImVec2(6, 6);
        style.CellPadding = ImVec2(12.10000038146973f, 2);
        style.IndentSpacing = 21.0f;
        style.ColumnsMinSpacing = 4.900000095367432f;
        style.ScrollbarSize = 16;
        style.ScrollbarRounding = 16;
        style.GrabMinSize = 8;
        style.GrabRounding = 0;
        style.TabRounding = 0.0f;
        style.TabBorderSize = 0.0f;
        style.TabBarBorderSize = 0.0f;
        // style.TabMinWidthForCloseButton = 0.0f;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0, 0.0f);
        style.DockingSeparatorSize = 4;
        
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                   = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.07f, 0.07f, 0.07f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
        colors[ImGuiCol_Border]                 = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.08f, 0.09f, 0.10f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.91f, 0.91f, 0.91f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0, 0, 0, 1.00f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.98f, 0.98f, 0.98f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.71f, 0.71f, 0.71f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.04f, 0.98f, 0.98f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.88f, 0.80f, 0.56f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.66f, 0.66f, 0.66f, 0.50f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.55f, 0.55f, 0.55f, 0.50f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.55f, 0.55f, 0.55f, 0.50f);

        ImGui::GetIO().ConfigDebugHighlightIdConflicts = false;
    }

    void App::RenderLoop() {
        static bool firstFrame = true;
        if (firstFrame) {
            InitializeInternals();
            firstFrame = false;
        }
        UIShared::s_timelineAnykeyframeDragged = false;

        std::string constructedTitle = std::string("Raster - Build Number ") + NumberToHexadecimal(BUILD_NUMBER) + " (" + std::to_string(BUILD_NUMBER) + ")";
        if (Workspace::s_project.has_value()) {
            auto& project = Workspace::s_project.value();
            constructedTitle += " - " + project.name;
        }
        GPU::SetWindowTitle(constructedTitle);

        auto requestedLayout = Layouts::GetRequestedLayout();
        if (requestedLayout) {
            ImGui::LoadIniSettingsFromDisk(requestedLayout->c_str());
        }

        auto dragDropPaths = GPU::GetDragDropPaths();
        if (Workspace::IsProjectLoaded()) {
            auto& project = Workspace::GetProject();
            for (auto& path : dragDropPaths) {
                auto assetCandidate = Workspace::ImportAsset(path);
                if (assetCandidate) {
                    project.assets.push_back(*assetCandidate);
                }
            }
        }

        GPU::BeginFrame();
            ImGuizmo::BeginFrame();
            if (Workspace::s_project.has_value() && !UIHelpers::AnyItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Space)) {
                Workspace::GetProject().playing = !Workspace::GetProject().playing;
            }
            if (Workspace::IsProjectLoaded()) {
                auto& project = Workspace::GetProject();

                TexturePrecision targetPrecision = TexturePrecision::Usual;
                if (project.colorPrecision == ProjectColorPrecision::Half) targetPrecision = TexturePrecision::Half;
                if (project.colorPrecision == ProjectColorPrecision::Full) targetPrecision = TexturePrecision::Full;
                Compositor::s_colorPrecision = targetPrecision;

                Audio::s_currentOptions = project.audioOptions;
                if (!UIHelpers::AnyItemFocused()) {
                    if (Audio::UpdateAudioInstance()) {
                        project.OnTimelineSeek();
                    }
                }

                for (auto& composition : project.compositions) {
                    if (!IsInBounds(project.currentFrame, composition.GetBeginFrame() - 1, composition.GetEndFrame() + 1) && !composition.identityState) {
                        composition.identityState = true;
                        project.SetFakeTime(composition.GetBeginFrame());
                        composition.OnTimelineSeek();
                        project.ResetFakeTime();
                    }
                    if (IsInBounds(project.currentFrame, composition.GetBeginFrame() - 1, composition.GetEndFrame() + 1)) {
                        composition.identityState = true;
                    }
                    if (composition.lockedCompositionID < 0) continue;
                    if (composition.lockedCompositionID == composition.id) {
                        composition.lockedCompositionID = -1;
                        continue;
                    }
                    auto lockedCompositionCandidate = Workspace::GetCompositionByID(composition.lockedCompositionID);
                    if (!lockedCompositionCandidate) continue;
                    auto& lockedComposition = *lockedCompositionCandidate;
                    if (composition.beginFrame != lockedComposition->beginFrame || composition.endFrame != lockedComposition->endFrame) {
                        composition.OnTimelineSeek();
                    }
                    composition.beginFrame = lockedComposition->beginFrame;
                    composition.endFrame = lockedComposition->endFrame;
                }
            }
            GPU::BindFramebuffer(std::nullopt);
            if (Workspace::s_project.has_value()) {
                auto& project = Workspace::s_project.value();
                project.currentFrame = std::max(project.currentFrame, 0.0f);
                project.currentFrame = std::min(project.currentFrame, project.GetProjectLength());
            }

            static AbstractUserInterface s_dockspaceUI = RASTER_SPAWN_ABSTRACT(AbstractUserInterface, DockspaceUI);
            s_dockspaceUI->Render();
            for (auto& layout : Workspace::s_configuration.layouts) {
                if (layout.id == Workspace::s_configuration.selectedLayout) {
                    for (auto& window : layout.windows) {
                        // DUMP_VAR(window->packageName);
                        window->Render();
                    }
                    break;
                }
            }

            Layouts::Update();

            ImGui::ShowDemoWindow();
            GPU::BindFramebuffer(std::nullopt);
        GPU::EndFrame();
        GPU::Flush();
        AsyncRendering::AllowRendering();

        if (Workspace::IsProjectLoaded()) {
            auto& project = Workspace::GetProject();
            AudioInfo::s_globalAudioOffset = std::fmod(project.currentFrame / project.framerate * AudioInfo::s_sampleRate, AudioInfo::s_periodSize);
            if (project.playing) {
                Rendering::ForceRenderFrame();
                auto projectLength = project.GetProjectLength();
                if (project.currentFrame >= projectLength) {
                    if (project.looping) {
                        project.currentFrame = 0;
                        project.OnTimelineSeek();
                    } else project.playing = false;
                } else {
                    if (project.currentFrame < projectLength) {
                        project.currentFrame += (project.framerate * ImGui::GetIO().DeltaTime);
                    }
                }
            }
        }
    }

    void App::Terminate() {
        AsyncRendering::Terminate();
        AsyncUpload::Terminate();
        if (Workspace::s_project.has_value()) {
            Workspace::GetProject().compositions.clear();
        }
        GPU::Terminate();
        Audio::Terminate();
        AudioMemoryManagement::Terminate();
        WaveformManager::Terminate();
        s_writerThreadRunning = false;
        s_writerThread.join();
    }
}