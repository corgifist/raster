#include "node_graph.h"

namespace Raster {

    static ImVec2 s_headerSize, s_originalCursor;
    static float s_maxInputPinX, s_maxOutputPinX;

    static float s_pinTextScale = 0.7f;

    void NodeGraphUI::RenderInputPin(GenericPin& pin, bool flow) {
        ImVec2 linkedAttributeSize = ImGui::CalcTextSize(pin.linkedAttribute.c_str());
        Nodes::BeginPin(pin.pinID, Nodes::PinKind::Input);
        Nodes::PinPivotAlignment({-0.14f, 0.5f});
            if (!flow) ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 1);
            Widgets::Icon(ImVec2(20, linkedAttributeSize.y), flow ? Widgets::IconType::Flow : Widgets::IconType::Circle, pin.connectedPinID > 0);
            ImGui::SameLine(0, 2.0f); 
            ImGui::SetWindowFontScale(s_pinTextScale);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.5f);
                ImGui::Text(pin.linkedAttribute.c_str());
            ImGui::SetWindowFontScale(1.0f);
        Nodes::EndPin();
    }

    void NodeGraphUI::RenderOutputPin(GenericPin& pin, bool flow) {
        ImVec2 linkedAttributeSize = ImGui::CalcTextSize(pin.linkedAttribute.c_str());
        ImGui::SetCursorPosX(s_originalCursor.x + (s_maxInputPinX + s_headerSize.x / 4.0f + (flow ? s_maxOutputPinX : 0)));
        Nodes::BeginPin(pin.pinID, Nodes::PinKind::Output);
            Nodes::PinPivotAlignment({-0.14f, 0.5f});

            ImGui::SetWindowFontScale(s_pinTextScale);
                float reservedCursor = ImGui::GetCursorPosY();
                ImGui::SetCursorPosY(reservedCursor + 2.5f);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3);
                ImGui::Text(pin.linkedAttribute.c_str());
            ImGui::SetWindowFontScale(1.0f);

            ImGui::SameLine(0.0f, 2.0f);
            ImGui::SetCursorPosY(reservedCursor);
            if (!flow) ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 1);
            Widgets::Icon(ImVec2(20, linkedAttributeSize.y), flow ? Widgets::IconType::Flow : Widgets::IconType::Circle, pin.connectedPinID > 0);
        Nodes::EndPin();
    }

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
            style.Colors[Nodes::StyleColor_NodeSelRect] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
            style.Colors[Nodes::StyleColor_NodeSelRectBorder] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
            style.NodeRounding = 0;
            style.NodeBorderWidth = 2.0f;


            Nodes::Begin("SimpleEditor");
                for (auto& node : Workspace::s_nodes) {
                    float maxInputXCandidate = 0;
                    for (auto& pin : node->inputPins) {
                        ImGui::SetWindowFontScale(s_pinTextScale);
                            auto attributeSize = ImGui::CalcTextSize(pin.linkedAttribute.c_str());
                        ImGui::SetWindowFontScale(1.0f);
                        if (attributeSize.x > maxInputXCandidate) {
                            maxInputXCandidate = attributeSize.x;
                        }
                    }
                    s_maxInputPinX = maxInputXCandidate;

                    float maxOutputXCandidate = 0;
                    for (auto& pin : node->outputPins) {
                        ImGui::SetWindowFontScale(s_pinTextScale);
                            auto attributeSize = ImGui::CalcTextSize(pin.linkedAttribute.c_str());
                        ImGui::SetWindowFontScale(1.0f);
                        if (attributeSize.x > maxOutputXCandidate) {
                            maxOutputXCandidate = attributeSize.x;
                        }
                    }
                    s_maxOutputPinX = maxOutputXCandidate;

                    Nodes::BeginNode(node->nodeID);
                        s_originalCursor = ImGui::GetCursorScreenPos();
                        ImVec2 originalCursor = ImGui::GetCursorScreenPos();
                        s_headerSize = ImGui::CalcTextSize(node->Header().c_str());
                        if (node->Footer().has_value()) {
                            auto footer = node->Footer().value();
                            ImGui::SetWindowFontScale(0.8f);
                            ImVec2 footerSize = ImGui::CalcTextSize(footer.c_str());
                            ImGui::SetWindowFontScale(1.0f);
                            if (footerSize.x > s_headerSize.x) {
                                s_headerSize = footerSize;
                            }
                        }
                        ImGui::Text("%s", node->Header().c_str());
                        if (node->flowInputPin.has_value()) {
                            RenderInputPin(node->flowInputPin.value(), true);
                        }
                        if (node->flowOutputPin.has_value()) {
                            ImGui::SameLine();
                            RenderOutputPin(node->flowOutputPin.value(), true);
                        }
                        float headerY = ImGui::GetCursorPosY();

                        ImGui::SetCursorPosX(originalCursor.x);

                        // Pins Rendering
                        for (auto& pin : node->inputPins) {
                            RenderInputPin(pin);
                        }


                        ImGui::SetCursorPosY(headerY);
                        for (auto& pin : node->outputPins) {
                            RenderOutputPin(pin);
                        }

                        // Footer Rendering

                        ImGui::SetWindowFontScale(0.8f);
                        auto footer = node->Footer();
                        if (footer.has_value()) {
                            ImGui::Spacing();
                            ImGui::Text(footer.value().c_str());
                        }
                        ImGui::SetWindowFontScale(1.0f);
                    Nodes::EndNode();
                }
            Nodes::End();
            Nodes::SetCurrentEditor(nullptr);
        ImGui::End();
    }
}