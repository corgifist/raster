#include "node_graph.h"
#include "font/font.h"
#include "common/ui_shared.h"
#include "common/dispatchers.h"

namespace Raster {

    struct CopyAccumulatorBundle {
        AbstractNode node;
        ImVec2 relativeNodeOffset;
    };

    struct DeferredAttributeExposure {
        AbstractNode node;
        std::string attribute;
    };


    static ImVec2 s_headerSize, s_originalCursor;
    static float s_maxInputPinX, s_maxOutputPinX;
    static float s_maxRuntimeInputPinX;

    static float s_pinTextScale = 0.7f;

    static std::optional<std::any> s_outerTooltip = "";

    static Composition* s_currentComposition = nullptr;
    static AbstractNode s_currentNode = nullptr;

    static std::vector<CopyAccumulatorBundle> s_copyAccumulator;

    static ImVec2 s_mousePos;

    static bool s_mustNavigateToSelection = false;

    struct NodePositioningTask {
        int nodeID;
        ImVec2 position;
    };

    static std::vector<NodePositioningTask> s_positioningTasks;

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
                for (auto& node : s_currentComposition->nodes) {
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
            ImGui::SameLine();
            s_maxRuntimeInputPinX = ImGui::GetCursorPosX() - s_originalCursor.x;
            ImGui::NewLine();
        ImGui::SetWindowFontScale(1.0f);
    }

    void NodeGraphUI::RenderOutputPin(GenericPin& pin, bool flow) {
        ImVec2 linkedAttributeSize = ImGui::CalcTextSize(pin.linkedAttribute.c_str());

        std::any cachedValue = std::nullopt;
        if (Workspace::s_pinCache.find(pin.pinID) != Workspace::s_pinCache.end()) {
            cachedValue = Workspace::s_pinCache[pin.pinID];
        }

        float maximumOffset = s_maxRuntimeInputPinX + s_maxOutputPinX;
        maximumOffset = std::max(maximumOffset, s_headerSize.x);

        float iconCursorX = maximumOffset - 20 + s_maxInputPinX;
        ImGui::SetCursorPosX(s_originalCursor.x + iconCursorX);

        Nodes::BeginPin(pin.pinID, Nodes::PinKind::Output);
            float reservedCursor = ImGui::GetCursorPosY();

            Nodes::PinPivotAlignment({1.44f, 0.51f});

            if (!flow) ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 1);
            bool isConnected = false;
            for (auto& node : s_currentComposition->nodes) {
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

    static std::unordered_map<int, int> s_idReplacements;

    void NodeGraphUI::ProcessCopyAction() {
        s_copyAccumulator.clear();
        s_idReplacements.clear();

        ImVec2 minNodePosition = ImVec2(FLT_MAX, FLT_MAX);
        for (auto& selectedNode : Workspace::GetProject().selectedNodes) {
            auto nodePos = Nodes::GetNodePosition(selectedNode);
            if (nodePos.x < minNodePosition.x) {
                minNodePosition.x = nodePos.x;
            }
            if (nodePos.y < minNodePosition.y) {
                minNodePosition.y = nodePos.y;
            }
        }
        auto& idReplacements = s_idReplacements;
        for (auto& selectedNode : Workspace::GetProject().selectedNodes) {
            auto nodeCandidate = Workspace::GetNodeByNodeID(selectedNode);
            if (nodeCandidate.has_value()) {
                CopyAccumulatorBundle bundle;
                auto& sourceNode = nodeCandidate.value();
                auto copiedNodeCandidate = Workspace::InstantiateSerializedNode(sourceNode->Serialize());
                if (nodeCandidate.has_value()) {
                    auto& node = copiedNodeCandidate.value();

                    int originalID = node->nodeID;
                    node->nodeID = Randomizer::GetRandomInteger();
                    idReplacements[originalID] = node->nodeID;

                    if (node->flowInputPin.has_value()) {
                        UpdateCopyPin(node->flowInputPin.value(), idReplacements);
                    }
                    if (node->flowOutputPin.has_value()) {
                        UpdateCopyPin(node->flowOutputPin.value(), idReplacements);
                    }
                    for (auto& inputPin : node->inputPins) {
                        UpdateCopyPin(inputPin, idReplacements);
                    }
                    for (auto& outputPin : node->outputPins) {
                        UpdateCopyPin(outputPin, idReplacements);
                    }

                    bundle.node = node;
                    bundle.relativeNodeOffset = Nodes::GetNodePosition(nodeCandidate.value()->nodeID) - minNodePosition;
                    s_copyAccumulator.push_back(bundle);
                }
            }
        }

        for (auto& bundle : s_copyAccumulator) {
            auto& node = bundle.node;
            if (node->flowInputPin.has_value()) {
                ReplaceCopyPin(node->flowInputPin.value(), idReplacements);
            }
            if (node->flowOutputPin.has_value()) {
                ReplaceCopyPin(node->flowOutputPin.value(), idReplacements);
            }
            for (auto& inputPin : node->inputPins) {
                ReplaceCopyPin(inputPin, idReplacements);
            }
            for (auto& outputPin : node->outputPins) {
                ReplaceCopyPin(outputPin, idReplacements);
            }
        }
    }

    void NodeGraphUI::UpdateCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements) {
        int originalID = pin.pinID;
        pin.pinID = Randomizer::GetRandomInteger();
        pin.linkID = Randomizer::GetRandomInteger();
        idReplacements[originalID] = pin.pinID;
    }

    void NodeGraphUI::ReplaceCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements) {
        if (idReplacements.find(pin.connectedPinID) != idReplacements.end()) {
            pin.connectedPinID = idReplacements[pin.connectedPinID];
        }
    }

    void NodeGraphUI::ProcessPasteAction() {
        for (auto& accumulatedNode : s_copyAccumulator) {
            s_currentComposition->nodes.push_back(accumulatedNode.node);
            Nodes::BeginNode(accumulatedNode.node->nodeID);
                Nodes::SetNodePosition(accumulatedNode.node->nodeID, s_mousePos + accumulatedNode.relativeNodeOffset);
            Nodes::EndNode();
        }

        if (s_copyAccumulator.size() != 0) {
            bool first = true;
            for (auto& accumulatedNode : s_copyAccumulator) {
                Nodes::SelectNode(accumulatedNode.node->nodeID, !first);
                first = false;
            }
        }
        s_copyAccumulator.clear();
    }

    void NodeGraphUI::Render() {
        s_outerTooltip = std::nullopt;

        static bool exposeAllPins = false;
        int uniqueID = 0;
        std::unordered_map<int, DeferredAttributeExposure> exposures;

        ImGui::Begin(FormatString("%s %s", ICON_FA_CIRCLE_NODES, Localization::GetString("NODE_GRAPH").c_str()).c_str());
        ImGui::PushFont(Font::s_denseFont);
            static std::unordered_map<int, Nodes::EditorContext*> compositionContexts;
            Nodes::EditorContext* ctx = nullptr;
            
            auto& project = Workspace::GetProject();
            auto compositionsCandidate = Workspace::GetSelectedCompositions();
            if (!compositionsCandidate.has_value()) s_currentComposition = nullptr;
            if (compositionsCandidate.has_value()) {
                auto compositions = compositionsCandidate.value();
                if (compositions.empty()) s_currentComposition = nullptr;
                s_currentComposition = compositions[0];
            }

            if (s_currentComposition && compositionContexts.find(s_currentComposition->id) != compositionContexts.end()) {
                ctx = compositionContexts[s_currentComposition->id];
            }

            if (!ctx && s_currentComposition) {
                if (!std::filesystem::exists("editors/")) {
                    std::filesystem::create_directory("editors");
                }
                std::string compositionPath = FormatString("editors/%i.json", s_currentComposition->id);
                Nodes::Config cfg;
                cfg.EnableSmoothZoom = true;
                cfg.SettingsFile = compositionPath.c_str();
                ctx = Nodes::CreateEditor(&cfg);
                compositionContexts[s_currentComposition->id] = ctx;
            }


            static float dimA = 0.0f;
            static float dimB = 0.3f;
            static float dimPercentage = -1.0f;
            static float previousDimPercentage;
            static bool dimming = false;

            Nodes::SetCurrentEditor(ctx);

            if (ctx) Nodes::EnableShortcuts(true);


            if (ctx) {
                auto& style = Nodes::GetStyle();
                style.Colors[Nodes::StyleColor_Bg] = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                style.Colors[Nodes::StyleColor_Grid] = ImVec4(0.09f, 0.09f, 0.09f, 1.0f);
                style.Colors[Nodes::StyleColor_NodeSelRect] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
                style.Colors[Nodes::StyleColor_NodeSelRectBorder] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
                style.Colors[Nodes::StyleColor_HighlightLinkBorder] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
                style.NodeRounding = 0;
                style.NodeBorderWidth = 2.0f;
                style.SnapLinkToPinDir = 1;
                style.PinArrowSize = 10.0f;
                style.PinArrowWidth = 10.0f;
            }

            if (ImGui::BeginTabBar("##compositionsBar", ImGuiTabBarFlags_Reorderable)) {
                for (auto& compositionID : project.selectedCompositions) {
                    auto compositionCandidate = Workspace::GetCompositionByID(compositionID);
                    if (compositionCandidate.has_value()) {
                        auto& composition = compositionCandidate.value();
                        if (ImGui::BeginTabItem(FormatString("%s %s", ICON_FA_LAYER_GROUP, composition->name.c_str()).c_str())) {
                            s_currentComposition = composition;
                            ImGui::EndTabItem();
                        }
                    }
                }
                ImGui::EndTabBar();
            }

            static Nodes::NodeId contextNodeID;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::BeginChild("##nodeGraphContainer", ImGui::GetContentRegionAvail());
            ImGui::PopStyleVar();
            if (ctx) {
                bool openSearchPopup = false;
                static bool drawLastLine = false;
                static Nodes::PinId connectedLastLinePin;
                bool previousMustNavigateToSelection = s_mustNavigateToSelection;
                Nodes::Begin("NodeGraph");

                    for (auto& target : Workspace::s_targetSelectNodes) {
                        Nodes::SelectNode(target, true);
                    }
                    Workspace::s_targetSelectNodes.clear();
                    s_mousePos = ImGui::GetIO().MousePos - ImGui::GetCursorScreenPos();

                    for (auto& task : s_positioningTasks) {
                        Nodes::BeginNode(task.nodeID);
                            Nodes::SetNodePosition(task.nodeID, task.position);
                        Nodes::EndNode();
                    }
                    s_positioningTasks.clear();

                    if (s_currentComposition) {
                        for (auto& node : s_currentComposition->nodes) {
                            s_currentNode = node;
                            float maxInputXCandidate = 0;
                            std::vector<std::string> attributesList;
                            auto set = node->GetAttributesList();
                            attributesList = std::vector<std::string>(set.begin(), set.end());
                            for (auto& pin : attributesList) {
                                ImGui::SetWindowFontScale(s_pinTextScale);
                                    auto attributeSize = ImGui::CalcTextSize(pin.c_str());
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
                                s_maxRuntimeInputPinX = 0;
                                s_originalCursor = ImGui::GetCursorScreenPos();
                                ImVec2 originalCursor = ImGui::GetCursorScreenPos();
                                std::string header = node->Icon() + (node->Icon().empty() ? "" : " ") + node->Header();
                                s_headerSize = ImGui::CalcTextSize(header.c_str());
                                if (s_maxInputPinX + s_maxOutputPinX > s_headerSize.x) {
                                    s_headerSize.x = s_maxInputPinX + s_maxOutputPinX;
                                }
                                if (node->Footer().has_value()) {
                                    auto footer = node->Footer().value();
                                    ImGui::SetWindowFontScale(0.8f);
                                    ImVec2 footerSize = ImGui::CalcTextSize(footer.c_str());
                                    ImGui::SetWindowFontScale(1.0f);
                                    if (footerSize.x > s_headerSize.x) {
                                        s_headerSize.x = footerSize.x;
                                    }
                                }
                                ImGui::Text("%s", header.c_str());
                                if (node->DetailsAvailable()) {
                                    ImGui::SetWindowFontScale(s_pinTextScale);
                                    std::optional<float> detailsCursorX = std::nullopt;
                                    static std::unordered_map<int, bool> s_customTreeNodeMap;
                                    static std::unordered_map<int, bool> s_customTreeNodeMapHovered;
                                    if (s_customTreeNodeMap.find(node->nodeID) == s_customTreeNodeMap.end()) {
                                        s_customTreeNodeMap[node->nodeID] = false;
                                        s_customTreeNodeMapHovered[node->nodeID] = false;
                                    }
                                    bool& customTreeNodeOpened = s_customTreeNodeMap[node->nodeID];
                                    bool& customTreeNodeHovered = s_customTreeNodeMapHovered[node->nodeID];
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(customTreeNodeHovered ? 0.7f : 1.0f));
                                        ImGui::Text("%s %s %s", customTreeNodeOpened ? ICON_FA_ANGLE_DOWN : ICON_FA_ANGLE_RIGHT, ICON_FA_CIRCLE_INFO, Localization::GetString("DETAILS").c_str());
                                    ImGui::PopStyleColor();
                                    if (ImGui::IsItemClicked()) {
                                        customTreeNodeOpened = !customTreeNodeOpened;
                                    }
                                    customTreeNodeHovered = ImGui::IsItemHovered();
                                    if (customTreeNodeOpened) {
                                        ImGui::Indent();
                                            node->RenderDetails();
                                            ImGui::SameLine();
                                            detailsCursorX = ImGui::GetCursorPosX();
                                            ImGui::NewLine();
                                        ImGui::Unindent();
                                    }
                                    if (detailsCursorX.has_value() && detailsCursorX.value() - Nodes::GetNodePosition(node->nodeID).x > s_headerSize.x) {
                                        s_headerSize.x = detailsCursorX.value() - Nodes::GetNodePosition(node->nodeID).x;
                                    }
                                    ImGui::SetWindowFontScale(1.0f);
                                }
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
                                if (!exposeAllPins && !drawLastLine) {
                                    for (auto& pin : node->inputPins) {
                                        RenderInputPin(pin);
                                    }
                                } else if (exposeAllPins || drawLastLine) {
                                    std::vector<std::string> excludedAttributes;
                                    for (auto& pin : node->inputPins) {
                                        RenderInputPin(pin);
                                        excludedAttributes.push_back(pin.linkedAttribute);
                                    }
                                    for (auto& attribute : node->GetAttributesList()) {
                                        if (std::find(excludedAttributes.begin(), excludedAttributes.end(), attribute) != excludedAttributes.end()) continue;
                                        GenericPin placeholderPin;
                                        placeholderPin.type = PinType::Input;
                                        placeholderPin.linkedAttribute = attribute;
                                        placeholderPin.pinID = uniqueID++;
                                        placeholderPin.linkID = uniqueID++;
                                            
                                        DeferredAttributeExposure exposure;
                                        exposure.attribute = attribute;
                                        exposure.node = node;
                                        exposures[placeholderPin.pinID] = exposure;

                                        RenderInputPin(placeholderPin);
                                    }
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

                            if (node->bypassed || !node->enabled || node->executionsPerFrame == 0) {
                                auto& style = Nodes::GetStyle();
                                ImDrawList* drawList = ImGui::GetWindowDrawList();
                                ImVec2 padding = ImVec2(style.NodePadding.x, style.NodePadding.y);
                                ImVec2 nodeSize = Nodes::GetNodeSize(node->nodeID);
                                drawList->AddRectFilled(s_originalCursor - padding, s_originalCursor + Nodes::GetNodeSize(node->nodeID) - padding, ImColor(0, 0, 0, 60));
                            }
                        }

                        for (auto& node : s_currentComposition->nodes) {
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
                                exposeAllPins = true;
                                int rawStartPinID = (int) startPinID.Get();
                                int rawEndPinID = (int) endPinID.Get();
                                auto startNode = Workspace::GetNodeByPinID(rawStartPinID);
                                auto endNode = Workspace::GetNodeByPinID(rawEndPinID);
                                
                                auto startPinContainer = Workspace::GetPinByPinID(rawStartPinID);
                                auto endPinContainer = Workspace::GetPinByPinID(rawEndPinID);
                                bool exposureTriggered = exposures.find(rawEndPinID) != exposures.end();
                                if ((startNode.has_value() && endNode.has_value() && startPinContainer.has_value() && endPinContainer.has_value()) || exposureTriggered) {
                                    if (exposureTriggered) {
                                        if (Nodes::AcceptNewItem(ImColor(128, 255, 128), 4.0f)) {
                                                if (exposures.find(rawEndPinID) != exposures.end()) {
                                                    auto& exposure = exposures[rawEndPinID];
                                                    auto freshEndNode = exposure.node;
                                                    freshEndNode->AddInputPin(exposure.attribute);
                                                    auto addedPinCandidate = freshEndNode->GetAttributePin(exposure.attribute);
                                                    if (addedPinCandidate.has_value()) {
                                                        auto& addedPin = addedPinCandidate.value();
                                                        addedPin.connectedPinID = rawStartPinID;
                                                        Workspace::UpdatePinByID(addedPin, addedPin.pinID);
                                                    }
                                                }
                                            }
                                        } else {
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
                                                        if (exposures.find(endPin.pinID) != exposures.end()) {
                                                            auto& exposure = exposures[endPin.pinID];
                                                            auto freshEndNode = Workspace::GetNodeByPinID(endPin.pinID).value();
                                                            freshEndNode->AddInputPin(exposures[endPin.connectedPinID].attribute);
                                                        }
                                                    }
                                                }
                                            }

                                            Workspace::UpdatePinByID(startPin, startPin.pinID);
                                            Workspace::UpdatePinByID(endPin, endPin.pinID);
                                    }
                                }
                            }
                            exposeAllPins = true;
                        } else exposeAllPins = false;

                        if (Nodes::QueryNewNode(&connectedLastLinePin)) {
                            if (Nodes::AcceptNewItem()) {
                                openSearchPopup = true;
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
                                    for (auto& node : s_currentComposition->nodes) {
                                        if (node->nodeID == rawNodeID) {
                                            targetNodeDelete = nodeIndex;
                                            break; 
                                        }
                                        nodeIndex++;
                                    }
                                    s_currentComposition->nodes.erase(s_currentComposition->nodes.begin() + targetNodeDelete);
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
                        if (Nodes::ShowNodeContextMenu(&contextNodeID)) {
                            ImGui::OpenPopup("##nodeContextMenu");
                        }
                        Nodes::Resume();

                    }

                if (drawLastLine) Nodes::DrawLastLine();
                Nodes::Suspend();

                if (ImGui::BeginPopup("##nodeContextMenu")) {
                    ImGui::PushFont(Font::s_normalFont);
                    auto nodeCandidate = Workspace::GetNodeByNodeID((int) contextNodeID.Get());
                    if (nodeCandidate.has_value()) {
                        auto& node = nodeCandidate.value();
                        ImGui::SeparatorText(FormatString("%s %s", node->Icon().c_str(), node->Header().c_str()).c_str());
                        ImGui::Text("%s %s: %i", ICON_FA_GEARS, Localization::GetString("EXECUTIONS_PER_FRAME").c_str(), node->executionsPerFrame);
                        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("ATTRIBUTES").c_str()).c_str())) {
                            int id = 0;
                            for (auto& attribute : node->GetAttributesList()) {
                                bool isAttributeExposed = false;
                                int attributeIndex = 0;
                                std::optional<std::any> attributeValue;
                                for (auto& inputPin : node->inputPins) {
                                    if (inputPin.linkedAttribute == attribute) {
                                        isAttributeExposed = true;
                                        if (Workspace::s_pinCache.find(inputPin.connectedPinID) != Workspace::s_pinCache.end()) {
                                            attributeValue = Workspace::s_pinCache[inputPin.connectedPinID];
                                        }
                                        break;
                                    }
                                    attributeIndex++;
                                }

                                ImGui::BulletText("%s", attribute.c_str());
                                if (ImGui::IsItemHovered() && attributeValue.has_value()) {
                                    s_outerTooltip = attributeValue.value();
                                }
                                ImGui::SameLine();
                                ImGui::PushID(id++);
                                if (ImGui::SmallButton(FormatString("%s %s", isAttributeExposed ? ICON_FA_LINK_SLASH : ICON_FA_LINK, Localization::GetString(isAttributeExposed ? "HIDE" : "EXPOSE").c_str()).c_str())) {
                                    if (!isAttributeExposed) {
                                        node->AddInputPin(attribute);
                                    } else {
                                        node->inputPins.erase(node->inputPins.begin() + attributeIndex);
                                    }
                                }
                                ImGui::PopID();
                            }
                            ImGui::EndMenu();
                        }
                        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("DESCRIPTION").c_str()).c_str())) {
                            ImGui::InputText("##nodeDescription", &node->overridenHeader);
                            ImGui::SetItemTooltip("%s %s", ICON_FA_PENCIL, Localization::GetString("DESCRIPTION").c_str());
                            ImGui::EndMenu();
                        }
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("DELETE_NODE").c_str()).c_str())) {
                            Nodes::DeleteNode(node->nodeID);
                        }
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_COPY, Localization::GetString("COPY").c_str()).c_str())) {
                            ProcessCopyAction();
                        }
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PASTE, Localization::GetString("PASTE").c_str()).c_str())) {
                            ProcessPasteAction();
                        }
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_CLONE, Localization::GetString("DUPLICATE").c_str()).c_str())) {
                            ProcessCopyAction();
                            ProcessPasteAction();
                        }
                    } else {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopFont();
                    ImGui::EndPopup();
                }


                previousDimPercentage = dimPercentage;
                if (dimPercentage >= 0 && dimming) {
                    dimPercentage += ImGui::GetIO().DeltaTime * 2.5;
                    if (dimPercentage >= dimB) {
                        dimming = false;
                    }
                    dimPercentage = std::clamp(dimPercentage, 0.0f, dimB);
                }
                bool popupVisible = true;
                static std::optional<ImVec2> nodeSearchMousePos;
                if (ImGui::BeginPopup("##createNewNode")) {
                    ImGui::PushFont(Font::s_normalFont);
                    if (!nodeSearchMousePos.has_value()) nodeSearchMousePos = s_mousePos;
                    ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LAYER_GROUP, s_currentComposition->name.c_str()).c_str());
                    if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("CREATE_NEW_NODE").c_str()).c_str(), "Tab")) {
                        openSearchPopup = true;
                    }
                    if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_LINK, Localization::GetString("CREATE_NEW_ATTRIBUTE").c_str()).c_str())) {
                        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("ADD_ATTRIBUTE").c_str()).c_str());
                        for (auto& entry : Attributes::s_attributes) {
                            if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_PLUS, entry.prettyName.c_str(), Localization::GetString("ATTRIBUTE").c_str()).c_str())) {
                                auto attributeCandidate = Attributes::InstantiateAttribute(entry.packageName);
                                if (attributeCandidate.has_value()) {
                                    s_currentComposition->attributes.push_back(attributeCandidate.value());
                                    project.selectedAttributes = {attributeCandidate.value()->id};
                                    auto attributeNode = Workspace::InstantiateNode(RASTER_PACKAGED "get_attribute_value").value();
                                    attributeNode->SetAttributeValue("AttributeID", s_currentComposition->attributes.back()->id);
                                    s_currentComposition->nodes.push_back(attributeNode);
                                    Nodes::BeginNode(attributeNode->nodeID);
                                    Nodes::EndNode();
                                    Nodes::SelectNode(attributeNode->nodeID);
                                    s_mustNavigateToSelection = true;
                                }
                            }
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_ARROW_POINTER, Localization::GetString("NAVIGATE_TO_SELECTED").c_str()).c_str())) {
                        Nodes::NavigateToSelection();
                    }
                    if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_EXPAND, Localization::GetString("NAVIGATE_TO_FIT").c_str()).c_str())) {
                        Nodes::NavigateToContent();
                    }
                    ImGui::PopFont();
                    ImGui::EndPopup();
                }
                bool ignoreLastLine = false;
                if (ImGui::Shortcut(ImGuiKey_Tab)) {
                    openSearchPopup = true;
                    ignoreLastLine = true;
                }
                if (openSearchPopup) {
                    ImGui::OpenPopup("##createNewNodeSearch");
                    drawLastLine = true;
                }
                static bool nodeSearchFocused = false;
                if (ImGui::BeginPopup("##createNewNodeSearch")) {
                    if (ignoreLastLine) drawLastLine = false;
                    if (!nodeSearchMousePos.has_value()) nodeSearchMousePos = s_mousePos;
                    static ImVec2 popupSize = ImVec2(300, 250);
                    ImGui::PushFont(Font::s_normalFont);
                    static std::string searchFilter = "";
                    if (!nodeSearchFocused) {
                        searchFilter = "";
                        ImGui::SetKeyboardFocusHere(0);
                        nodeSearchFocused = true;
                    }
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::InputTextWithHint("##searchFilter",FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &searchFilter);
                    ImGui::PopItemWidth();
                    ImGui::SameLine(0, 0);
                    float searchBarWidth = ImGui::GetCursorPosX();
                    ImGui::NewLine();
                    ImGui::Separator();
                    static NodeCategory targetCategory = 0;
                    ImGui::BeginChild("##tabChild", ImVec2(160, 0), ImGuiChildFlags_AutoResizeY);
                        int allCategoryTotalNodes = 0;
                        for (auto& node : Workspace::s_nodeImplementations) {
                            if (!searchFilter.empty() && LowerCase(node.description.prettyName).find(LowerCase(searchFilter)) == std::string::npos) continue;
                            allCategoryTotalNodes++;
                        }
                        ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
                        if (targetCategory == 0 && allCategoryTotalNodes > 0) {
                            buttonColor = buttonColor * 1.3f;
                        }
                        ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
                        if (allCategoryTotalNodes == 0) ImGui::BeginDisabled();
                        if (ImGui::ButtonEx(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("ALL_NODES").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                            targetCategory = 0;
                        }
                        if (allCategoryTotalNodes == 0) ImGui::EndDisabled();
                        ImGui::PopStyleColor();
                        for (auto& category : NodeCategoryUtils::s_categoriesOrder) {
                            int categoryTotalNodes = 0;
                            for (auto& node : Workspace::s_nodeImplementations) {
                                if (node.description.category != category) continue;
                                if (!searchFilter.empty() && LowerCase(node.description.prettyName).find(LowerCase(searchFilter)) == std::string::npos) continue;
                                categoryTotalNodes++;
                            }
                            ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
                            if (targetCategory == category && categoryTotalNodes != 0 && targetCategory != 0) {
                                buttonColor = buttonColor * 1.3f;
                            }
                            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
                            if (categoryTotalNodes == 0) ImGui::BeginDisabled();
                            if (ImGui::ButtonEx(FormatString("%s (%i)", NodeCategoryUtils::ToString(category).c_str(), categoryTotalNodes).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                                targetCategory = category;
                            }
                            if (categoryTotalNodes == 0) ImGui::EndDisabled();
                            ImGui::PopStyleColor();
                        }
                        
                    ImGui::EndChild();
                    ImGui::SameLine();
                    ImGui::BeginChild("##nodeCandidates", ImVec2(200, 200));
                    bool hasCandidates = false;
                    std::string categoryText = NodeCategoryUtils::ToString(targetCategory);
                    if (targetCategory == 0) {
                        categoryText = FormatString("%s %s", ICON_FA_LIST, Localization::GetString("ALL_NODES").c_str());
                    }
                    ImGui::SeparatorText(categoryText.c_str());
                    for (auto& node : Workspace::s_nodeImplementations) {
                        if (node.description.category != targetCategory && targetCategory > 0) continue;
                        if (!searchFilter.empty() && LowerCase(node.description.prettyName).find(LowerCase(searchFilter)) == std::string::npos) continue;
                        bool overrideNodeImplementationSelected = false;
                        if (!hasCandidates && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                            overrideNodeImplementationSelected = true;
                        }
                        hasCandidates = true;
                        bool nodeImplementationSelected = ImGui::MenuItem(FormatString("%s %s",
                                NodeCategoryUtils::ToIcon(node.description.category).c_str(), 
                                node.description.prettyName.c_str()).c_str());
                        if (ImGui::IsItemHovered()) {
                            if (ImGui::BeginTooltip()) {
                                ImGui::Text("%s %s: %s", ICON_FA_BOX_OPEN, Localization::GetString("PACKAGE_NAME").c_str(), node.description.packageName.c_str());
                                ImGui::Text("%s %s: %s", ICON_FA_STAR, Localization::GetString("PRETTY_NODE_NAME").c_str(), node.description.prettyName.c_str());
                                ImGui::Text("%s %s: %s", ICON_FA_LIST, Localization::GetString("CATEGORY").c_str(), NodeCategoryUtils::ToString(node.description.category).c_str());
                                ImGui::EndTooltip();
                            }
                        }
                        if (nodeImplementationSelected || overrideNodeImplementationSelected) {
                            auto targetNode = Workspace::AddNode(node.libraryName);
                            if (targetNode.has_value()) {
                                auto node = targetNode.value();
                                Nodes::BeginNode(node->nodeID);
                                    Nodes::SetNodePosition(node->nodeID, nodeSearchMousePos.value());
                                Nodes::EndNode();
                                Nodes::SelectNode(node->nodeID, false);
                                s_mustNavigateToSelection = true;
                                nodeSearchMousePos = std::nullopt;
                                if (drawLastLine) {
                                    auto lastLinePinCandidate = Workspace::GetPinByPinID((int) connectedLastLinePin.Get());
                                    if (lastLinePinCandidate.has_value()) {
                                        auto& lastLinePin = lastLinePinCandidate.value();
                                        auto lastLineNode = Workspace::GetNodeByPinID(lastLinePin.pinID).value();
                                        auto nodeAttributes = node->GetAttributesList();
                                        if (lastLinePin.type == PinType::Output) {
                                            if (!nodeAttributes.empty()) {
                                                node->AddInputPin(*nodeAttributes.begin());
                                                auto nodeInputPin = node->GetAttributePin(*nodeAttributes.begin()).value();
                                                nodeInputPin.connectedPinID = lastLinePin.pinID;
                                                Workspace::UpdatePinByID(nodeInputPin, nodeInputPin.pinID);
                                                Workspace::UpdatePinByID(lastLinePin, lastLinePin.pinID);
                                            }
                                        } else {
                                            if (!lastLineNode->inputPins.empty() && !node->outputPins.empty()) {
                                                auto& nodeInputPin = node->inputPins[0];
                                                lastLineNode->inputPins[0].connectedPinID = node->outputPins[0].pinID;
                                            }
                                        }
                                    }
                                }
                            }
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    if (!hasCandidates) {
                        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 2.0f - ImGui::CalcTextSize(Localization::GetString("NOTHING_TO_SHOW").c_str()).x / 2.0f);
                        ImGui::Text(Localization::GetString("NOTHING_TO_SHOW").c_str());
                    }
                    ImGui::EndChild();
                    ImGui::PopFont();
                    popupSize = ImGui::GetWindowSize();
                    ImGui::EndPopup();
                    if (dimPercentage < 0) {
                        dimPercentage = 0.0f;
                        dimming = true;
                    }
                } else if (!ImGui::GetIO().MouseDown[ImGuiMouseButton_Right]) {
                    dimming = false;
                    popupVisible = false;
                    drawLastLine = false;
                    nodeSearchFocused = false;
                } else {
                    drawLastLine = false;
                    nodeSearchFocused = false;
                }
                if (!popupVisible && !ImGui::GetIO().MouseDown[ImGuiMouseButton_Right]) {
                    dimPercentage -= ImGui::GetIO().DeltaTime * 2.5;
                    if (dimPercentage < 0) {
                        dimPercentage = -1;
                        dimming = true;
                    }
                }
                Nodes::Resume();
                project.selectedNodes.clear();

                std::vector<Nodes::NodeId> temporarySelectedNodes(Nodes::GetSelectedObjectCount());
                Nodes::GetSelectedNodes(temporarySelectedNodes.data(), Nodes::GetSelectedObjectCount());

                for (auto& nodeID : temporarySelectedNodes) {
                    project.selectedNodes.push_back((int) nodeID.Get());
                }

                if (ImGui::Shortcut(ImGuiKey_ModCtrl | ImGuiKey_C)) {
                    ProcessCopyAction();
                }

                if (ImGui::Shortcut(ImGuiKey_ModCtrl | ImGuiKey_V)) {
                    ProcessPasteAction();
                }
                
                if (ImGui::Shortcut(ImGuiKey_ModCtrl | ImGuiKey_D)) {
                    ProcessCopyAction();
                    ProcessPasteAction();
                }

                if (s_mustNavigateToSelection && previousMustNavigateToSelection) {
                    Nodes::NavigateToSelection();
                    s_mustNavigateToSelection = false;
                }

                Nodes::End();
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                if (dimPercentage > 0.05f) {
                    drawList->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, dimPercentage)));
                }
            }
            if (s_currentComposition) {
                ImGui::SetCursorPos(ImGui::GetCursorStartPos() + ImVec2(5, 5));
                ImGui::PushFont(Font::s_denseFont);
                    ImGui::SetWindowFontScale(1.5f);
                        ImGui::Text("%s %s", ICON_FA_LAYER_GROUP, s_currentComposition->name.c_str());
                    ImGui::SetWindowFontScale(1.0f);
                ImGui::PopFont();
                static bool isDescriptionHovered = false;
                static bool isEditingDescription = false;
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, isDescriptionHovered && !isEditingDescription ? 0.8f : 1.0f);
                    if (isEditingDescription) {
                        isDescriptionHovered = false;
                    } 
                    ImGui::SetCursorPosX(ImGui::GetCursorStartPos().x + 5);
                    ImGui::Text("%s", s_currentComposition->description.c_str());
                    if (isDescriptionHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        ImGui::OpenPopup("##compositionDescriptionEditor");
                        isEditingDescription = true;
                    }
                    isDescriptionHovered = ImGui::IsItemHovered();
                ImGui::PopStyleVar();
                if (ImGui::BeginPopup("##compositionDescriptionEditor")) {
                    ImGui::InputTextMultiline("##description", &s_currentComposition->description);
                    ImGui::SameLine();
                    if (ImGui::Button(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str())) {
                        isEditingDescription = false;
                        isDescriptionHovered = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::EndChild();
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ATTRIBUTE_TIMELINE_PAYLOAD)) {
                    AttributeDragDropPayload attributePayload = *(AttributeDragDropPayload*) payload->Data;
                    if (s_currentComposition) {
                        auto attributeNodeCandidate = Workspace::InstantiateNode(RASTER_PACKAGED "get_attribute_value");
                        if (attributeNodeCandidate.has_value()) {
                            auto& attributeNode = attributeNodeCandidate.value();
                            attributeNode->SetAttributeValue("AttributeID", attributePayload.attributeID);
                            s_currentComposition->nodes.push_back(attributeNode);
                            s_positioningTasks.push_back(NodePositioningTask{
                                .nodeID = attributeNode->nodeID,
                                .position = s_mousePos
                            });
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }
            Nodes::SetCurrentEditor(nullptr);
            ImGui::PopFont();

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            if (dimPercentage > 0.05f) {
                drawList->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, dimPercentage)));
            }


        if (s_outerTooltip.has_value()) {
            if (ImGui::BeginTooltip()) {
                auto value = s_outerTooltip.value();
                ImGui::Text("%s %s: %s", ICON_FA_CIRCLE_INFO, Localization::GetString("VALUE_TYPE").c_str(), Workspace::GetTypeName(value).c_str());
                Dispatchers::DispatchString(value);
                ImGui::EndTooltip();
            }
        }

        ImGui::End();
    }
}