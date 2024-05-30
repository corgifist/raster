#include "node_graph.h"
#include "font/font.h"

namespace Raster {

    static ImVec2 s_headerSize, s_originalCursor;
    static float s_maxInputPinX, s_maxOutputPinX;

    static float s_pinTextScale = 0.7f;

    static std::optional<std::any> s_outerTooltip = "";

    std::optional<ImVec4> NodeGraphUI::GetColorByDynamicValue(std::any& value) {
        for (auto& color : Workspace::s_typeColors) {
            if (color.first == std::type_index(value.type())) {
                return ImGui::ColorConvertU32ToFloat4(color.second);
            }
        }
        return std::nullopt;
    }

    void NodeGraphUI::ShowLabel(std::string t_label, ImU32 t_color) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
        auto size = ImGui::CalcTextSize(t_label.c_str());

        auto padding = ImGui::GetStyle().FramePadding;
        auto spacing = ImGui::GetStyle().ItemSpacing;

        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

        auto rectMin = ImGui::GetCursorScreenPos() - padding;
        auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

        auto drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(rectMin, rectMax, t_color, size.y * 0.15f);
        ImGui::TextUnformatted(t_label.c_str());
    }

    void NodeGraphUI::RenderInputPin(GenericPin& pin, bool flow) {
        ImVec2 linkedAttributeSize = ImGui::CalcTextSize(pin.linkedAttribute.c_str());
        std::any cachedValue = std::nullopt;
        if (Workspace::s_pinCache.find(pin.connectedPinID) != Workspace::s_pinCache.end()) {
            cachedValue = Workspace::s_pinCache[pin.connectedPinID];
        }
        Nodes::BeginPin(pin.pinID, Nodes::PinKind::Input);
            Nodes::PinPivotAlignment({-0.45f, 0.5});


            if (!flow) ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 1);
            bool isConnected = false;
            if (pin.flow) {
                for (auto& node : Workspace::s_nodes) {
                    if (node->flowInputPin.has_value()) {
                        if (node->flowInputPin.value().connectedPinID == pin.pinID) isConnected = true;
                    }
                    
                    if (node->flowOutputPin.has_value()) {
                        if (node->flowOutputPin.value().connectedPinID == pin.pinID) isConnected = true;
                    }

                    if (isConnected) break;
                }
            }
            ImVec4 pinColor = ImVec4(1, 1, 1, 1);
            auto colorCandidate = GetColorByDynamicValue(cachedValue);
            if (colorCandidate.has_value()) {
                pinColor = colorCandidate.value();
            }
            Widgets::Icon(ImVec2(20, linkedAttributeSize.y), flow ? Widgets::IconType::Flow : Widgets::IconType::Circle, isConnected || pin.connectedPinID > 0, pinColor);
        Nodes::EndPin();

        if ((int) Nodes::GetHoveredPin().Get() == pin.pinID && cachedValue.type() != typeid(std::nullopt)) {
            s_outerTooltip = cachedValue;
        }
        ImGui::SameLine(0, 0.0f);
        ImGui::SetWindowFontScale(s_pinTextScale);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.5f);
            ImGui::Text(pin.linkedAttribute.c_str());
        ImGui::SetWindowFontScale(1.0f);
    }

    void NodeGraphUI::RenderOutputPin(GenericPin& pin, bool flow) {
        ImVec2 linkedAttributeSize = ImGui::CalcTextSize(pin.linkedAttribute.c_str());

        std::any cachedValue = std::nullopt;
        if (Workspace::s_pinCache.find(pin.pinID) != Workspace::s_pinCache.end()) {
            cachedValue = Workspace::s_pinCache[pin.pinID];
        }

        float maximumOffset = s_maxInputPinX + s_maxOutputPinX + 5;
        maximumOffset = std::max(maximumOffset, s_headerSize.x);

        ImGui::SetCursorPosX(s_originalCursor.x + maximumOffset - 20);

        Nodes::BeginPin(pin.pinID, Nodes::PinKind::Output);
            float reservedCursor = ImGui::GetCursorPosY();

            Nodes::PinPivotAlignment({1.44f, 0.51f});

            float iconCursorX = maximumOffset - 20 + s_maxInputPinX;
            ImGui::SetCursorPosX(s_originalCursor.x + iconCursorX);
            if (!flow) ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 1);
            bool isConnected = false;
            for (auto& node : Workspace::s_nodes) {
                for (auto& inputPin : node->inputPins) {
                    if (inputPin.connectedPinID == pin.pinID) isConnected = true;
                }
                for (auto& outputPin : node->outputPins) {
                    if (outputPin.connectedPinID == pin.pinID) isConnected = true;
                }

                if (node->flowInputPin.has_value()) {
                    if (node->flowInputPin.value().pinID == pin.connectedPinID) isConnected = true;
                }
                
                if (node->flowOutputPin.has_value()) {
                    if (node->flowOutputPin.value().pinID == pin.connectedPinID) isConnected = true;
                }

                if (isConnected) break;
            }
            ImVec4 pinColor = ImVec4(1, 1, 1, 1);
            auto colorCandidate = GetColorByDynamicValue(cachedValue);
            if (colorCandidate.has_value()) {
                pinColor = colorCandidate.value();
            }
            Widgets::Icon(ImVec2(20, linkedAttributeSize.y), flow ? Widgets::IconType::Flow : Widgets::IconType::Circle, isConnected, pinColor);
        Nodes::EndPin();

        if ((int) Nodes::GetHoveredPin().Get() == pin.pinID && cachedValue.type() != typeid(std::nullopt)) {
            s_outerTooltip = cachedValue;
        }

        ImGui::SetWindowFontScale(s_pinTextScale);
            ImGui::SetCursorPosY(reservedCursor + 2.5f);
            ImGui::SetCursorPosX((s_originalCursor.x + iconCursorX) - linkedAttributeSize.x * s_pinTextScale);
            ImGui::Text(pin.linkedAttribute.c_str());
        ImGui::SetWindowFontScale(1.0f);
    }

    void NodeGraphUI::Render() {
        s_outerTooltip = std::nullopt;
        Workspace::s_selectedNodes.clear();
        ImGui::Begin(FormatString("%s %s", ICON_FA_CIRCLE_NODES, Localization::GetString("NODE_GRAPH").c_str()).c_str());
        ImGui::PushFont(Font::s_denseFont);
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
            style.Colors[Nodes::StyleColor_HighlightLinkBorder] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
            style.NodeRounding = 0;
            style.NodeBorderWidth = 2.0f;
            style.SnapLinkToPinDir = 1;
            style.PinArrowSize = 10.0f;
            style.PinArrowWidth = 10.0f;


            Nodes::Begin("SimpleEditor");
                for (auto& target : Workspace::s_targetSelectNodes) {
                    Nodes::SelectNode(target, true);
                }
                Workspace::s_targetSelectNodes.clear();
                ImVec2 mousePos = ImGui::GetIO().MousePos - ImGui::GetCursorScreenPos();
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
                        std::string header = node->Icon() + (node->Icon().empty() ? "" : " ") + node->Header();
                        s_headerSize = ImGui::CalcTextSize(header.c_str());
                        if (node->Footer().has_value()) {
                            auto footer = node->Footer().value();
                            ImGui::SetWindowFontScale(0.8f);
                            ImVec2 footerSize = ImGui::CalcTextSize(footer.c_str());
                            ImGui::SetWindowFontScale(1.0f);
                            if (footerSize.x > s_headerSize.x) {
                                s_headerSize = footerSize;
                            }
                        }
                        ImGui::Text("%s", header.c_str());
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

                for (auto& node : Workspace::s_nodes) {
                    if (node->flowInputPin.has_value() && node->flowInputPin.value().connectedPinID > 0) {
                        auto sourcePin = node->flowInputPin.value();
                        Nodes::Link(sourcePin.linkID, sourcePin.pinID, sourcePin.connectedPinID, ImVec4(1, 1, 1, 1), 2.0f);
                    }
                    if (node->flowOutputPin.has_value() && node->flowOutputPin.value().connectedPinID > 0) {
                        auto sourcePin = node->flowOutputPin.value();
                        Nodes::Link(sourcePin.linkID, sourcePin.pinID, sourcePin.connectedPinID, ImVec4(1, 1, 1, 1), 2.0f);
                    }

                    for (auto& pin : node->inputPins) {
                        if (pin.connectedPinID > 0) {
                            std::any cachedValue = std::nullopt;
                            if (Workspace::s_pinCache.find(pin.connectedPinID) != Workspace::s_pinCache.end()) {
                                cachedValue = Workspace::s_pinCache[pin.connectedPinID];
                            }
                            ImVec4 linkColor = ImVec4(1, 1, 1, 1);
                            auto colorCandidate = GetColorByDynamicValue(cachedValue);
                            if (colorCandidate.has_value() && cachedValue.type() != typeid(std::nullopt)) {
                                linkColor = colorCandidate.value();
                            }
                            Nodes::Link(pin.linkID, pin.connectedPinID, pin.pinID, linkColor, 2.0f);
                        }
                    }
                    for (auto& pin : node->outputPins) {
                        if (pin.connectedPinID > 0) {
                            Nodes::Link(pin.linkID, pin.connectedPinID, pin.pinID, ImVec4(1, 1, 1, 1), 2.0f);
                        }
                    }
                }

                if (Nodes::BeginCreate()) {
                    Nodes::PinId startPinID, endPinID;
                    if (Nodes::QueryNewLink(&startPinID, &endPinID)) {
                        int rawStartPinID = (int) startPinID.Get();
                        int rawEndPinID = (int) endPinID.Get();
                        auto startNode = Workspace::GetNodeByPinID(rawStartPinID);
                        auto endNode = Workspace::GetNodeByPinID(rawEndPinID);
                        
                        auto startPinContainer = Workspace::GetPinByPinID(rawStartPinID);
                        auto endPinContainer = Workspace::GetPinByPinID(rawEndPinID);
                        if (startNode.has_value() && endNode.has_value() && startPinContainer.has_value() && endPinContainer.has_value()) {
                            auto startPin = startPinContainer.value();
                            auto endPin = endPinContainer.value();

                            if (startPin.type == PinType::Input) {
                                std::swap(startPin, endPin);
                                std::swap(startPinContainer, endPinContainer);
                                std::swap(startPinID, endPinID);
                                std::swap(rawStartPinID, rawEndPinID);
                            }

                            if (endPin.pinID == startPin.pinID) {
                                Nodes::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                            } else if (endPin.type == startPin.type) {
                                ShowLabel(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("INVALID_LINK").c_str()), ImColor(45, 32, 32, 180));
                                Nodes::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                            } else {
                                if (Nodes::AcceptNewItem(ImColor(128, 255, 128), 4.0f)) {
                                    if (startPin.flow) {
                                        startPin.connectedPinID = endPin.pinID;
                                    } else {
                                        endPin.connectedPinID = startPin.pinID;
                                    }
                                }
                            }

                            Workspace::UpdatePinByID(startPin, startPin.pinID);
                            Workspace::UpdatePinByID(endPin, endPin.pinID);
                        }
                    }
                }
                Nodes::EndCreate();

                if (Nodes::BeginDelete()) {
                    Nodes::NodeId nodeID = 0;
                    while (Nodes::QueryDeletedNode(&nodeID)) {
                        int rawNodeID = (int) nodeID.Get();
                        if (Nodes::AcceptDeletedItem()) {
                            int targetNodeDelete = -1;
                            int nodeIndex = 0;
                            for (auto& node : Workspace::s_nodes) {
                                if (node->nodeID == rawNodeID) {
                                    targetNodeDelete = nodeIndex;
                                    break; 
                                }
                                nodeIndex++;
                            }
                            Workspace::s_nodes.erase(Workspace::s_nodes.begin() + targetNodeDelete);
                        }
                    }

                    Nodes::LinkId linkID = 0;
                    while (Nodes::QueryDeletedLink(&linkID)) {
                        int rawLinkID = (int) linkID.Get();
                        auto pin = Workspace::GetPinByLinkID(rawLinkID);
                        if (pin.has_value()) {
                            auto correctedPin = pin.value();
                            correctedPin.connectedPinID = -1;
                            Workspace::UpdatePinByID(correctedPin, correctedPin.pinID);
                        }
                    }
                }
                Nodes::EndDelete();

            Nodes::Suspend();
            if (Nodes::ShowBackgroundContextMenu()) {
                ImGui::OpenPopup("##createNewNode");
            }
            Nodes::Resume();

            Nodes::Suspend();
            static float dimA = 0.0f;
            static float dimB = 0.3f;
            static float dimPercentage = -1.0f;
            static float previousDimPercentage;
            static bool dimming = false;
            previousDimPercentage = dimPercentage;
            if (dimPercentage >= 0 && dimming) {
                dimPercentage += ImGui::GetIO().DeltaTime * 2.5;
                if (dimPercentage >= dimB) {
                    dimming = false;
                }
                dimPercentage = std::clamp(dimPercentage, 0.0f, dimB);
            }
            bool popupVisible = true;
            if (ImGui::BeginPopup("##createNewNode", ImGuiWindowFlags_AlwaysAutoResize)) {
                static std::string searchFilter = "";
                ImGui::InputText("##searchFilter", &searchFilter);
                ImGui::SameLine(0, 0);
                float searchBarWidth = ImGui::GetCursorPosX();
                ImGui::NewLine();
                ImGui::SetItemTooltip(FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str());
                static float tabBarHeight = 0;
                NodeCategory targetCategory = NodeCategory::Other;
                ImGui::BeginChild("##tabChild", ImVec2(searchBarWidth, tabBarHeight));
                    if (ImGui::BeginTabBar("##searchCategories")) {
                        for (auto& category : Workspace::s_categories) {
                            if (ImGui::BeginTabItem(NodeCategoryUtils::ToString(category).c_str())) {
                                targetCategory = category;
                                ImGui::EndTabItem();
                            }
                        }
                        ImGui::EndTabBar();
                    }
                    tabBarHeight = ImGui::GetCursorPosY();
                ImGui::EndChild();
                ImGui::BeginChild("##nodeCandidates", ImVec2(searchBarWidth, 200));
                bool hasCandidates = false;
                for (auto& node : Workspace::s_nodeImplementations) {
                    if (node.description.category != targetCategory) continue;
                    if (!searchFilter.empty() && node.description.prettyName.find(searchFilter) == std::string::npos) continue;
                    hasCandidates = true;
                    if (ImGui::MenuItem(FormatString("%s %s",
                            NodeCategoryUtils::ToIcon(node.description.category).c_str(), 
                            node.description.prettyName.c_str()).c_str())) {
                        auto targetNode = Workspace::AddNode(node.libraryName);
                        if (targetNode.has_value()) {
                            auto node = targetNode.value();
                            Nodes::BeginNode(node->nodeID);
                                Nodes::SetNodePosition(node->nodeID, mousePos);
                            Nodes::EndNode();
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!hasCandidates) {
                    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 2.0f - ImGui::CalcTextSize(Localization::GetString("NOTHING_TO_SHOW").c_str()).x / 2.0f);
                    ImGui::Text(Localization::GetString("NOTHING_TO_SHOW").c_str());
                }
                ImGui::EndChild();
                ImGui::EndPopup();
                if (dimPercentage < 0) {
                    dimPercentage = 0.0f;
                    dimming = true;
                }
            } else if (!ImGui::GetIO().MouseDown[ImGuiMouseButton_Right]) {
                dimming = false;
                popupVisible = false;
            }
            if (!popupVisible && !ImGui::GetIO().MouseDown[ImGuiMouseButton_Right]) {
                dimPercentage -= ImGui::GetIO().DeltaTime * 2.5;
                if (dimPercentage < 0) {
                    dimPercentage = -1;
                    dimming = true;
                }
            }
            Nodes::Resume();

            std::vector<Nodes::NodeId> temporarySelectedNodes(Nodes::GetSelectedObjectCount());
            Nodes::GetSelectedNodes(temporarySelectedNodes.data(), Nodes::GetSelectedObjectCount());

            for (auto& nodeID : temporarySelectedNodes) {
                Workspace::s_selectedNodes.push_back((int) nodeID.Get());
            }

            Nodes::End();
            ImGui::PopFont();
            Nodes::SetCurrentEditor(nullptr);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            if (dimPercentage > 0.05f) {
                drawList->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, dimPercentage)));
            }

        if (s_outerTooltip.has_value()) {
            if (ImGui::BeginTooltip()) {
                auto value = s_outerTooltip.value();
                ImGui::Text("%s %s: %s", ICON_FA_CIRCLE_INFO, Localization::GetString("VALUE_TYPE").c_str(), value.type().name());
                NodeBase::DispatchValueAttribute(value);
                ImGui::EndTooltip();
            }
        }
        ImGui::End();
    }
}