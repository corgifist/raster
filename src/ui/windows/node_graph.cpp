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

            Nodes::Begin("SimpleEditor");
                int uniqueId = 1;

                Nodes::NodeId nodeA_Id = uniqueId++;
                Nodes::PinId  nodeA_InputPinId = uniqueId++;
                Nodes::PinId  nodeA_OutputPinId = uniqueId++;
                Nodes::BeginNode(nodeA_Id);
                    ImGui::Text("Node A");
                    Nodes::BeginPin(nodeA_InputPinId, Nodes::PinKind::Input);
                        ImGui::Text("-> In");
                    Nodes::EndPin();
                    ImGui::SameLine();
                    Nodes::BeginPin(nodeA_OutputPinId, Nodes::PinKind::Output);
                        ImGui::Text("Out ->");
                    Nodes::EndPin();
                Nodes::EndNode();
            Nodes::End();
            Nodes::SetCurrentEditor(nullptr);
        ImGui::End();
    }
}