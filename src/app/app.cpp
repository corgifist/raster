#include "app/app.h"
#include "gpu/gpu.h"
#include "font/font.h"
#include "../ImGui/imgui_node_editor.h"
#include "common/common.h"
#include "traverser/traverser.h"

namespace Nodes = ax::NodeEditor;

namespace Raster {

    std::vector<AbstractUI> App::s_windows{};

    void App::Initialize() {
        GPU::Initialize();
        ImGui::SetCurrentContext((ImGuiContext*) GPU::GetImGuiContext());

        Workspace::s_configuration = Configuration(ReadJson("misc/config.json"));

        try {
            Localization::Load(ReadJson(FormatString("misc/localizations/%s.json", Workspace::s_configuration.localizationCode.c_str())));
        } catch (std::exception ex) {
            Localization::Load(ReadJson("misc/localizations/en.json"));
        }

        Workspace::Initialize();
        Workspace::s_nodes.push_back(Workspace::InstantiateNode("raster_debug_print").value());
        Workspace::s_nodes.push_back(Workspace::InstantiateNode("raster_debug_print").value());

        ImGuiIO& io = ImGui::GetIO();

        ImFontConfig fontCfg = {};
        fontCfg.PixelSnapH = true;

        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};

        io.Fonts->AddFontFromMemoryCompressedTTF(
            Font::s_fontBytes.data(), Font::s_fontSize,
            16.0f
        );

        fontCfg.MergeMode = true;
        io.Fonts->AddFontFromMemoryCompressedTTF(
            Font::s_fontAwesomeBytes.data(), Font::s_fontAwesomeSize,
            16.0f * 0.85f, &fontCfg, icons_ranges
        );

        fontCfg.RasterizerDensity = 5;
        fontCfg.MergeMode = false;
        Font::s_denseFont = io.Fonts->AddFontFromMemoryCompressedTTF(
                    Font::s_fontBytes.data(), Font::s_fontSize,
                    16.0f, &fontCfg, io.Fonts->GetGlyphRangesCyrillic());

        // fontCfg.GlyphMinAdvanceX = 16.0f * 2.0f / 3.0f;
        fontCfg.MergeMode = true;
        io.Fonts->AddFontFromMemoryCompressedTTF(
            Font::s_fontAwesomeBytes.data(), Font::s_fontAwesomeSize,
            16.0f * 0.85f, &fontCfg, icons_ranges
        );

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;


        auto& style = ImGui::GetStyle();
        style.CurveTessellationTol = 0.01f;
        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
        colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] =
            ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] =
            ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.70f, 0.70f, 0.70f, 0.31f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.50f, 0.52f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.34f, 0.34f, 0.34f, 0.86f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.09f, 0.09f, 0.09f, 0.97f);
        colors[ImGuiCol_TabUnfocusedActive] =
            ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.62f, 0.62f, 0.62f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.60f, 0.15f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] =
            ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] =
            ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        colors[ImGuiCol_TabActive] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
        style.WindowRounding = 4;
        style.FrameRounding = 4;
        style.ChildRounding = 4;
        style.PopupRounding = 4;
        style.TabRounding = 4;
        style.ScrollbarRounding = 4;
        style.GrabRounding = 4;

        s_windows.push_back(UIFactory::SpawnNodeGraphUI());
        s_windows.push_back(UIFactory::SpawnNodePropertiesUI());

        GPU::GenerateTexture(128, 128);
    }

    void App::RenderLoop() {
        while (!GPU::MustTerminate()) {
            GPU::BeginFrame();
                ImGui::DockSpaceOverViewport();

                for (const auto& window : s_windows) {
                    window->Render();
                }

                
                ImGui::ShowDemoWindow();

                Traverser::TraverseAll();
            GPU::EndFrame();
        }
    }

    void App::Terminate() {
        GPU::Terminate();
    }
}