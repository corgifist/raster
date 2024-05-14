#include "node_graph.h"


namespace Raster {
    void NodeGraphUI::Render() {
        ImGui::Begin("Nodes Test");
            static Nodes::EditorContext* ctx = nullptr;
            if (!ctx) {
                Nodes::Config cfg;
                cfg.EnableSmoothZoom = true;
                ctx = Nodes::CreateEditor(&cfg);
            }

            Nodes::SetCurrentEditor(ctx);

            auto& style = Nodes::GetStyle();
            style.Colors[Nodes::StyleColor_Bg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            style.Colors[Nodes::StyleColor_Grid] = ImVec4(0.09f, 0.09f, 0.09f, 1.0f);
            style.NodeRounding = 0;
            style.NodeBorderWidth = 2.0f;

            Nodes::Begin("SimpleEditor");
                int uniqueId = 1;

                for (auto& node : Workspace::s_nodes) {
                    Nodes::BeginNode(node->nodeID);
                        ImGui::Text("%s", node->Header().c_str());
                        ImGui::SetWindowFontScale(0.8f);
                        auto footer = node->Footer();
                        if (footer.has_value()) {
                            ImGui::Text("%s", footer.value_or("").c_str());
                        }
                        ImGui::SetWindowFontScale(1.0f);
                    Nodes::EndNode();
                }
            Nodes::End();
            Nodes::SetCurrentEditor(nullptr);
        ImGui::End();
    }
}