#include "node_graph.h"
#include "font/font.h"
#include "common/ui_shared.h"
#include "common/dispatchers.h"
#include "raster.h"
#include "timeline.h"
#include "asset_manager.h"

namespace Raster {

    struct CopyAccumulatorBundle {
        AbstractNode node;
        ImVec2 relativeNodeOffset;
    };

    struct DeferredAttributeExposure {
        AbstractNode node;
        std::string attribute;
    };

    struct DeferredNodeCreation {
        int nodeID;
        ImVec2 position;
        bool mustBeSelected;
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
    static std::vector<DeferredNodeCreation> s_deferredNodeCreations;

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

    static unordered_dense::map<int, bool> s_inputPinCache;

    static void InvalidateInputPinCache(int t_pinID) {
        if (s_inputPinCache.find(t_pinID) != s_inputPinCache.end()) {
            s_inputPinCache.erase(t_pinID);
        }
    }

    void NodeGraphUI::RenderInputPin(GenericPin& pin, bool flow) {
        ImVec2 linkedAttributeSize = ImGui::CalcTextSize(pin.linkedAttribute.c_str());
        std::any cachedValue = std::nullopt;
        {
            // RASTER_SYNCHRONIZED(Workspace::s_pinCacheMutex);
            if (Workspace::s_pinCache.GetFrontValue().find(pin.connectedPinID) != Workspace::s_pinCache.GetFrontValue().end()) {
                cachedValue = Workspace::s_pinCache.GetFrontValue()[pin.connectedPinID];
            }
        }
        Nodes::BeginPin(pin.pinID, Nodes::PinKind::Input);
            Nodes::PinPivotAlignment({-0.45f, 0.5});

            if (!flow) ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 1);
            bool isConnected = false;
            if (s_inputPinCache.find(pin.pinID) != s_inputPinCache.end()) {
                isConnected = s_inputPinCache[pin.pinID];
            } else {
                if (pin.flow) {
                    for (auto& pair : s_currentComposition->nodes) {
                        auto& node = pair.second;
                        if (node->flowInputPin.has_value()) {
                            if (node->flowInputPin.value().connectedPinID == pin.pinID) isConnected = true;
                        }
                        
                        if (node->flowOutputPin.has_value()) {
                            if (node->flowOutputPin.value().connectedPinID == pin.pinID) isConnected = true;
                        }

                        if (isConnected) break;
                    }
                    s_inputPinCache[pin.pinID] = isConnected;
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
            std::string linkedAttributeText = pin.linkedAttribute;
            auto nodeCandidate = Workspace::GetNodeByPinID(pin.pinID);
            if (nodeCandidate.has_value()) {
                auto& node = nodeCandidate.value();
                auto compositionCandidate = Workspace::GetCompositionByNodeID(node->nodeID);
                std::string linkedAttributeID = FormatString("<%i>.%s", node->nodeID, pin.linkedAttribute.c_str());
                if (compositionCandidate.has_value()) {
                    auto& composition = compositionCandidate.value();
                    for (auto& attribute : composition->attributes) {
                        if (attribute->internalAttributeName.find(linkedAttributeID) != std::string::npos) {
                            linkedAttributeText = ICON_FA_LINK + std::string(" ") + linkedAttributeText;
                        }
                    }
                }
            }
            ImGui::Text(linkedAttributeText.c_str());
            ImGui::SameLine();
            s_maxRuntimeInputPinX = ImGui::GetCursorPosX() - s_originalCursor.x;
            ImGui::NewLine();
        ImGui::SetWindowFontScale(1.0f);
    }

    static unordered_dense::map<int, bool> s_outputPinCache;

    static void InvalidateOutputPinCache(int t_pinID) {
        if (s_outputPinCache.find(t_pinID) != s_outputPinCache.end()) {
            s_outputPinCache.erase(t_pinID);
        }
    }

    static void InvalidatePinCache(int t_pinID) {
        InvalidateInputPinCache(t_pinID);
        InvalidateOutputPinCache(t_pinID);
    }

    void NodeGraphUI::RenderOutputPin(GenericPin& pin, bool flow) {
        ImVec2 linkedAttributeSize = ImGui::CalcTextSize(pin.linkedAttribute.c_str());

        std::any cachedValue = std::nullopt;
        {
            // RASTER_SYNCHRONIZED(Workspace::s_pinCacheMutex);
            if (Workspace::s_pinCache.GetFrontValue().find(pin.pinID) != Workspace::s_pinCache.GetFrontValue().end()) {
                cachedValue = Workspace::s_pinCache.GetFrontValue()[pin.pinID];
            }
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
            if (s_outputPinCache.find(pin.pinID) != s_outputPinCache.end()) {
                isConnected = s_outputPinCache[pin.pinID];
            } else {
                for (auto& pair : s_currentComposition->nodes) {
                    auto& node = pair.second;
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
                s_outputPinCache[pin.pinID] = isConnected;
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
        RASTER_SYNCHRONIZED(Workspace::s_nodesMutex);
        for (auto& accumulatedNode : s_copyAccumulator) {
            s_currentComposition->nodes[accumulatedNode.node->nodeID] = accumulatedNode.node;
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

        static std::optional<ImVec2> nodeSearchMousePos;
        static std::optional<Composition> s_compositionToAdd;

        if (ImGui::Begin(FormatString("%s %s", ICON_FA_CIRCLE_NODES, Localization::GetString("NODE_GRAPH").c_str()).c_str())) {
            if (!Workspace::IsProjectLoaded()) {
                ImGui::PushFont(Font::s_denseFont);
                ImGui::SetWindowFontScale(2.0f);
                    ImVec2 exclamationSize = ImGui::CalcTextSize(ICON_FA_TRIANGLE_EXCLAMATION);
                    ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - exclamationSize / 2.0f);
                    ImGui::Text(ICON_FA_TRIANGLE_EXCLAMATION);
                ImGui::SetWindowFontScale(1.0f);
                ImGui::PopFont();
                ImGui::End();
                return;
            }
            static std::unordered_map<int, Nodes::EditorContext*> compositionContexts;
            Nodes::EditorContext* ctx = nullptr;
            
            static int overrideCurrentComposition = -1;
            auto compositionsCandidate = Workspace::GetSelectedCompositions();
            if (!compositionsCandidate.has_value()) s_currentComposition = nullptr;
            if (compositionsCandidate.has_value()) {
                auto compositions = compositionsCandidate.value();
                if (compositions.empty()) s_currentComposition = nullptr;
                s_currentComposition = compositions[0];
                auto overridenCompositionCandidate = Workspace::GetCompositionByID(overrideCurrentComposition);
                if (overridenCompositionCandidate.has_value()) {
                    s_currentComposition = overridenCompositionCandidate.value();
                }
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

            std::optional<int> openDescriptionRenamePopup;

            Nodes::SetCurrentEditor(ctx);

            if (ctx) Nodes::EnableShortcuts(true);


            if (ctx) {
                auto& style = Nodes::GetStyle();
                style.Colors[Nodes::StyleColor_Bg] = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                style.Colors[Nodes::StyleColor_Grid] = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg) * 0.7f;
                style.Colors[Nodes::StyleColor_Grid].w = 1.0f;
                style.Colors[Nodes::StyleColor_NodeSelRect] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
                style.Colors[Nodes::StyleColor_NodeSelRectBorder] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
                style.Colors[Nodes::StyleColor_HighlightLinkBorder] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
                style.NodeRounding = 0;
                style.NodeBorderWidth = 2.0f;
                style.SnapLinkToPinDir = 1;
                style.PinArrowSize = 10.0f;
                style.PinArrowWidth = 10.0f;
            }


            if (ImGui::BeginTabBar("##compositionsBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {
                if (Workspace::IsProjectLoaded()) {
                    auto& project = Workspace::GetProject();
                    int focusNeeded = -1;
                    if (!project.customData.contains("CurrentlyTabbedCompositions")) {
                        project.customData["CurrentlyTabbedCompositions"] = Json::array();
                    }
                    static std::vector<int> s_currentlyTabbedCompositions = project.customData["CurrentlyTabbedCompositions"];

                    if (!project.customData.contains("PreviousSelectedCompositions")) {
                        project.customData["PreviousSelectedCompositions"] = Json::array();
                    }
                    static std::vector<int> s_previousSelectedCompositions = project.customData["PreviousSelectedCompositions"];
                    project.customData["PreviousSelectedCompositions"] = s_previousSelectedCompositions;
                    project.customData["CurrentlyTabbedCompositions"] = s_currentlyTabbedCompositions;
                    auto& selectedCompositions = project.selectedCompositions;
                    bool mustReviewTabbedCompositions = false;
                    if (s_previousSelectedCompositions.size() != selectedCompositions.size()) {
                        mustReviewTabbedCompositions = true;
                    }
                    for (auto& compositionID : selectedCompositions) {
                        if (std::find(s_previousSelectedCompositions.begin(), s_previousSelectedCompositions.end(), compositionID) == s_previousSelectedCompositions.end()) {
                            mustReviewTabbedCompositions = true;
                            break;
                        }
                    }
                    std::vector<int> forceFocusedCompositions;
                    if (mustReviewTabbedCompositions && selectedCompositions.size() > 1) {
                        for (auto& compositionID : selectedCompositions) {
                            if (std::find(s_currentlyTabbedCompositions.begin(), s_currentlyTabbedCompositions.end(), compositionID) == s_currentlyTabbedCompositions.end()) {
                                s_currentlyTabbedCompositions.push_back(compositionID);
                                forceFocusedCompositions.push_back(compositionID);
                            }
                        }
                    } else if (mustReviewTabbedCompositions && selectedCompositions.size() == 1) {
                        if (!selectedCompositions.empty()) {
                            if (std::find(s_currentlyTabbedCompositions.begin(), s_currentlyTabbedCompositions.end(), selectedCompositions[0]) == s_currentlyTabbedCompositions.end()) {
                                s_currentlyTabbedCompositions.push_back(selectedCompositions[0]);
                            }
                            forceFocusedCompositions.push_back(selectedCompositions[0]);
                        }
                    }
                    if (!forceFocusedCompositions.empty()) {
                        focusNeeded = forceFocusedCompositions[0];
                    }
                    s_previousSelectedCompositions = selectedCompositions;

                    int closedComposition = -1;
                    for (auto& compositionID : s_currentlyTabbedCompositions) {
                        auto compositionCandidate = Workspace::GetCompositionByID(compositionID);
                        if (compositionCandidate.has_value()) {
                            auto& composition = compositionCandidate.value();
                            bool open = true;
                            ImVec4 colorMark4 = ImGui::ColorConvertU32ToFloat4(composition->colorMark);
                            ImGui::PushStyleColor(ImGuiCol_Tab, colorMark4 * 0.6f);
                            ImGui::PushStyleColor(ImGuiCol_TabHovered, colorMark4 * 0.8f);
                            ImGui::PushStyleColor(ImGuiCol_TabActive, colorMark4);
                            
                            if (ImGui::BeginTabItem(FormatString("%s %s###%i", ICON_FA_LAYER_GROUP, composition->name.c_str(), composition->id).c_str(), &open, focusNeeded == compositionID ? ImGuiTabItemFlags_SetSelected : 0)) {
                                overrideCurrentComposition = composition->id;
                                ImGui::EndTabItem();
                            }
                            ImGui::PopStyleColor(3);
                            if (!open) {
                                closedComposition = compositionID;
                            }
                        }
                    }

                    bool openNewCompositionPopup = false;
                    bool openNavigateToCompositionPopup = false;
                    static std::string s_newCompositionName = "";
                    if (ImGui::TabItemButton(ICON_FA_PLUS, ImGuiTabItemFlags_Trailing)) {
                        openNewCompositionPopup = true;
                        s_newCompositionName = Localization::GetString("NEW_COMPOSITION");
                    }
                    if (ImGui::TabItemButton(ICON_FA_MAGNIFYING_GLASS, ImGuiTabItemFlags_Trailing)) {
                        openNavigateToCompositionPopup = true;
                    }

                    if (openNewCompositionPopup) {
                        ImGui::OpenPopup("##newCompositionPopup");
                    }

                    if (openNavigateToCompositionPopup) {
                        ImGui::OpenPopup("##navigateToCompositionPopup");
                    }

                    static bool s_compositionSearchFocused = false;
                    if (ImGui::BeginPopup("##navigateToCompositionPopup")) {
                        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LAYER_GROUP, Localization::GetString("NAVIGATE_TO_COMPOSITION").c_str()).c_str());
                        static std::string s_compositionSearch = "";
                        if (!s_compositionSearchFocused) {
                            ImGui::SetKeyboardFocusHere(0);
                            s_compositionSearchFocused = true;
                        }
                        ImGui::InputTextWithHint("##compositionSearch", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_compositionSearch);
                        if (ImGui::BeginChild("##compositionCandidates", ImVec2(ImGui::GetContentRegionAvail().x, RASTER_PREFERRED_POPUP_HEIGHT))) {
                            bool first = true;
                            for (auto& composition : project.compositions) {
                                if (!s_compositionSearch.empty() && LowerCase(composition.name).find(LowerCase(s_compositionSearch)) == std::string::npos) continue;
                                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_LAYER_GROUP, composition.name.c_str()).c_str()) || (first && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                                    project.selectedCompositions = {composition.id};
                                    ImGui::CloseCurrentPopup();
                                }
                                first = false;
                            }
                        }
                        ImGui::EndChild();
                        ImGui::EndPopup();
                    } else s_compositionSearchFocused = false;

                    static bool s_compositionNameFieldFocused = false;
                    if (ImGui::BeginPopup("##newCompositionPopup")) {
                        if (!s_compositionNameFieldFocused) {
                            ImGui::SetKeyboardFocusHere(0);
                            s_compositionNameFieldFocused = true;
                        }
                        ImGui::InputTextWithHint("##compositionName", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("NEW_COMPOSITION_NAME").c_str()).c_str(), &s_newCompositionName);
                        ImGui::SameLine();
                        if (ImGui::Button(Localization::GetString("OK").c_str()) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                            Composition newComposition;
                            newComposition.beginFrame = project.currentFrame;
                            newComposition.endFrame = project.currentFrame + project.framerate;
                            s_compositionToAdd = newComposition;
                            project.selectedCompositions = {newComposition.id};
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    } else s_compositionNameFieldFocused = false;

                    if (closedComposition > 0) {
                        auto tabIterator = std::find(s_currentlyTabbedCompositions.begin(), s_currentlyTabbedCompositions.end(), closedComposition);
                        if (tabIterator != s_currentlyTabbedCompositions.end()) {
                            s_currentlyTabbedCompositions.erase(tabIterator);
                        }
                    }
                }
                ImGui::EndTabBar();
            }

            static Nodes::NodeId contextNodeID;
            static bool drawLastLine = false;
            bool popupVisible = true;
            static bool nodeSearchFocused = false;
            static ImVec2 popupSize = ImVec2(300, 250);
            bool previousMustNavigateToSelection = s_mustNavigateToSelection;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            if (ImGui::BeginChild("##nodeGraphContainer", ImGui::GetContentRegionAvail())) {
                ImGui::PopStyleVar();
                if (ctx) {
                    bool openSearchPopup = false;
                    bool openNavigateToNodePopup = false;
                    static Nodes::PinId connectedLastLinePin;
                    Nodes::Begin("NodeGraph");
                        ImGui::PushFont(Font::s_denseFont);
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
                            for (auto& nodePair : s_currentComposition->nodes) {
                                auto& node = nodePair.second;
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
                                    for (auto& deferredNodeCreationInfo : s_deferredNodeCreations) {
                                        if (deferredNodeCreationInfo.nodeID == node->nodeID) {
                                            Nodes::SetNodePosition(node->nodeID, deferredNodeCreationInfo.position);
                                            if (deferredNodeCreationInfo.mustBeSelected) {
                                                Nodes::SelectNode(node->nodeID);
                                                s_mustNavigateToSelection = true;
                                            }
                                        }
                                    }
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
                                        auto immediateFooters = node->GetImmediateFooters();
                                        if (!immediateFooters.empty()) {
                                            footer += "\n";
                                        }
                                        for (auto& immediateFooter : immediateFooters) {
                                            footer += immediateFooter + "\n";
                                        }
                                        ImGui::SetWindowFontScale(0.8f);
                                        ImVec2 footerSize = ImGui::CalcTextSize(footer.c_str());
                                        ImGui::SetWindowFontScale(1.0f);
                                        if (footerSize.x > s_headerSize.x) {
                                            s_headerSize.x = footerSize.x;
                                        }
                                    }
                                    static std::unordered_map<int, bool> s_labelHoveredMap;
                                    static std::unordered_map<int, bool> s_labelClickedMap;

                                    if (s_labelHoveredMap.find(node->nodeID) == s_labelHoveredMap.end()) {
                                        s_labelHoveredMap[node->nodeID] = false;
                                        s_labelClickedMap[node->nodeID] = false;
                                    }

                                    bool& labelHovered = s_labelHoveredMap[node->nodeID];
                                    bool& labelClicked = s_labelClickedMap[node->nodeID];

                                    float textAlpha = 1.0f;

                                    if (labelHovered) textAlpha *= 0.9f;
                                    if (labelClicked) textAlpha *= 0.9f;

                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, textAlpha));
                                        ImGui::Text("%s", header.c_str());
                                    ImGui::PopStyleColor();

                                    if (labelHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                        openDescriptionRenamePopup = node->nodeID;
                                    }

                                    labelHovered = ImGui::IsItemHovered();
                                    labelClicked = ImGui::IsItemClicked();
                                    
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
                                        auto& actualFooter = footer.value();
                                        auto immediateFooters = node->GetImmediateFooters();
                                        if (!immediateFooters.empty()) {
                                            actualFooter += "\n";
                                        }
                                        for (auto& immediateFooter : immediateFooters) {
                                            actualFooter += immediateFooter + "\n";
                                        }
                                        ImGui::TextUnformatted(actualFooter.c_str());
                                    }
                                    ImGui::SetWindowFontScale(1.0f);
                                Nodes::EndNode();

                                if (node->bypassed || !node->enabled || node->executionsPerFrame.GetFrontValue() == 0) {  
                                    auto& style = Nodes::GetStyle();
                                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                                    ImVec2 padding = ImVec2(style.NodePadding.x, style.NodePadding.y);
                                    ImVec2 nodeSize = Nodes::GetNodeSize(node->nodeID);
                                    drawList->AddRectFilled(s_originalCursor - padding, s_originalCursor + Nodes::GetNodeSize(node->nodeID) - padding, ImColor(0, 0, 0, 60));
                                }
                            }

                            s_deferredNodeCreations.clear();

                            for (auto& pair : s_currentComposition->nodes) {
                                auto& node = pair.second;
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
                                        {
                                            RASTER_SYNCHRONIZED(Workspace::s_pinCacheMutex);
                                            if (Workspace::s_pinCache.GetFrontValue().find(pin.connectedPinID) != Workspace::s_pinCache.GetFrontValue().end()) {
                                                cachedValue = Workspace::s_pinCache.GetFrontValue()[pin.connectedPinID];
                                            }
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
                                                            InvalidatePinCache(addedPin.pinID);
                                                            InvalidatePinCache(addedPin.connectedPinID);
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
                                                } else if (endPin.type == startPin.type || (Workspace::GetNodeByPinID(startPin.pinID).has_value() && Workspace::GetNodeByPinID(endPin.pinID) 
                                                                && Workspace::GetNodeByPinID(startPin.pinID).value()->nodeID == Workspace::GetNodeByPinID(endPin.pinID).value()->nodeID)) {
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
                                                InvalidatePinCache(startPin.pinID);
                                                InvalidatePinCache(startPin.connectedPinID);
                                                InvalidatePinCache(endPin.pinID);
                                                InvalidatePinCache(endPin.connectedPinID);
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
                                {
                                    RASTER_SYNCHRONIZED(Workspace::s_nodesMutex);
                                    while (Nodes::QueryDeletedNode(&nodeID)) {
                                        int rawNodeID = (int) nodeID.Get();
                                        if (Nodes::AcceptDeletedItem()) {
                                            auto nodeIterator = s_currentComposition->nodes.find(rawNodeID);
                                            if (nodeIterator != s_currentComposition->nodes.end()) {
                                                s_currentComposition->nodes.erase(nodeIterator);
                                            }
                                        }
                                    }
                                }

                                {
                                    Nodes::LinkId linkID = 0;
                                    while (Nodes::QueryDeletedLink(&linkID)) {
                                        int rawLinkID = (int) linkID.Get();
                                        auto pin = Workspace::GetPinByLinkID(rawLinkID);
                                        if (pin.has_value()) {
                                            auto correctedPin = pin.value();
                                            InvalidatePinCache(correctedPin.connectedPinID);
                                            correctedPin.connectedPinID = -1;
                                            Workspace::UpdatePinByID(correctedPin, correctedPin.pinID);
                                            InvalidatePinCache(correctedPin.pinID);
                                        }
                                    }
                                }
                            }
                            Nodes::EndDelete();

                            Nodes::Suspend();
                            for (auto& pair : s_currentComposition->nodes) {
                                auto& node = pair.second;
                                std::string popupID = FormatString("##descriptionRename%i", node->nodeID);
                                if (openDescriptionRenamePopup.value_or(-1) == node->nodeID) {
                                    ImGui::OpenPopup(popupID.c_str());
                                    openDescriptionRenamePopup = std::nullopt;
                                }

                                if (ImGui::BeginPopup(popupID.c_str())) {
                                    ImGui::SetKeyboardFocusHere(0);
                                    ImGui::InputTextWithHint("##description", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("NODE_DESCRIPTION").c_str()).c_str(), &node->overridenHeader);
                                    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                                        ImGui::CloseCurrentPopup();
                                    }
                                    ImGui::EndPopup();
                                }
                            }

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

                    static std::optional<float> nodeContextMenuWidth;
                    if (ImGui::BeginPopup("##nodeContextMenu")) {
                        ImGui::PushFont(Font::s_normalFont);
                        auto nodeCandidate = Workspace::GetNodeByNodeID((int) contextNodeID.Get());
                        if (nodeCandidate.has_value()) {
                            auto& node = nodeCandidate.value();
                            ImGui::SeparatorText(FormatString("%s %s", node->Icon().c_str(), node->Header().c_str()).c_str());
                            ImGui::Text("%s %s: %i", ICON_FA_GEARS, Localization::GetString("EXECUTIONS_PER_FRAME").c_str(), node->executionsPerFrame.GetFrontValue());
                            if (ImGui::Button(FormatString("%s %s", node->enabled ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF, Localization::GetString("ENABLED").c_str()).c_str(), ImVec2(nodeContextMenuWidth.value_or(ImGui::GetWindowSize().x) / 2.0f, 0))) {
                                node->enabled = !node->enabled;
                            }
                            ImGui::SameLine(0, 2);
                            if (ImGui::Button(FormatString("%s %s", node->bypassed ? ICON_FA_CHECK : ICON_FA_XMARK, Localization::GetString("BYPASSED").c_str()).c_str(), ImVec2(nodeContextMenuWidth.value_or(ImGui::GetWindowSize().x) / 2.0f, 0))) {
                                node->bypassed = !node->bypassed;
                            }
                            if (node->DoesAudioMixing()) {
                                std::string audioMixingMessage = FormatString("%s %s", ICON_FA_VOLUME_HIGH, Localization::GetString("PERFORMS_AUDIO_MIXING").c_str());
                                std::string audioMixingWarning = FormatString("%s %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("AUDIO_MIXING_IS_DISABLED_IN_THIS_COMPOSITION").c_str());
                                ImVec2 audioMixingMessageSize = ImGui::CalcTextSize(audioMixingMessage.c_str());
                                ImVec2 audioMixingWarningSize = ImGui::CalcTextSize(audioMixingWarning.c_str());
                                ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - audioMixingMessageSize.x / 2.0f);
                                ImGui::Text("%s", audioMixingMessage.c_str());
                                if (!s_currentComposition->audioEnabled) {
                                    ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - audioMixingWarningSize.x / 2.0f);
                                    ImGui::Text("%s", audioMixingWarning.c_str());
                                }
                            }
                            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("EXPOSE_HIDE_ATTRIBUTES").c_str()).c_str())) {
                                int id = 0;
                                for (auto& attribute : node->GetAttributesList()) {
                                    bool isAttributeExposed = false;
                                    int attributeIndex = 0;
                                    std::optional<std::any> attributeValue;
                                    for (auto& inputPin : node->inputPins) {
                                        if (inputPin.linkedAttribute == attribute) {
                                            isAttributeExposed = true;
                                            {
                                                RASTER_SYNCHRONIZED(Workspace::s_pinCacheMutex);
                                                if (Workspace::s_pinCache.GetFrontValue().find(inputPin.connectedPinID) != Workspace::s_pinCache.GetFrontValue().end()) {
                                                    attributeValue = Workspace::s_pinCache.GetFrontValue()[inputPin.connectedPinID];
                                                }
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
                                    ImGui::SameLine();
                                    std::string exposeToTimelinePopupID = FormatString("##exposeToTimeline%i%s", node->nodeID, attribute.c_str());
                                    bool exposedToTimeline = false;
                                    auto compositionCandidate = Workspace::GetCompositionByNodeID(node->nodeID);
                                    if (compositionCandidate.has_value()) {
                                        auto& composition = compositionCandidate.value();
                                        std::string exposedAttributeID = FormatString("<%i>.%s", node->nodeID, attribute.c_str());
                                        for (auto& attribute : composition->attributes) {
                                            if (attribute->internalAttributeName.find(exposedAttributeID) != std::string::npos) {
                                                exposedToTimeline = true;
                                                break;
                                            }
                                        }
                                    }
                                    std::string exposeToTimelineIcon = ICON_FA_TIMELINE " " ICON_FA_PLUS;
                                    if (exposedToTimeline) exposeToTimelineIcon = ICON_FA_TIMELINE " " ICON_FA_TRASH_CAN;
                                    if (ImGui::SmallButton(FormatString("%s %s", exposeToTimelineIcon.c_str(), Localization::GetString(exposedToTimeline ? "HIDE_FROM_TIMELINE" : "EXPOSE_TO_TIMELINE").c_str()).c_str())) {
                                        if (!exposedToTimeline) {
                                            ImGui::OpenPopup(exposeToTimelinePopupID.c_str());
                                        } else {
                                            if (compositionCandidate.has_value()) {
                                                auto& composition = compositionCandidate.value();
                                                std::string exposedAttributeID = FormatString("<%i>.%s", node->nodeID, attribute.c_str());
                                                int attributeIndex = 0;
                                                for (auto& attribute : composition->attributes) {
                                                    if (attribute->internalAttributeName.find(exposedAttributeID) != std::string::npos) {
                                                        composition->attributes.erase(composition->attributes.begin() + attributeIndex);
                                                        break;
                                                    }
                                                    attributeIndex++;
                                                }
                                            }
                                        }
                                    }

                                    if (ImGui::BeginPopup(exposeToTimelinePopupID.c_str())) {
                                        ImGui::SeparatorText(FormatString("%s %s: %s", ICON_FA_TIMELINE, Localization::GetString("EXPOSE_TO_TIMELINE").c_str(), attribute.c_str()).c_str());
                                        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("CREATE_NEW_ATTRIBUTE").c_str()).c_str())) {
                                            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("CREATE_NEW_ATTRIBUTE").c_str()).c_str());
                                            for (auto& entry : Attributes::s_implementations) {
                                                if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_PLUS, entry.description.prettyName.c_str(), Localization::GetString("ATTRIBUTE").c_str()).c_str())) {
                                                    auto attributeCandidate = Attributes::InstantiateAttribute(entry.description.packageName);
                                                    if (attributeCandidate.has_value()) {
                                                        auto& exposedAttribute = attributeCandidate.value();
                                                        exposedAttribute->internalAttributeName += (exposedAttribute->internalAttributeName.empty() ? "" : " | ") + FormatString("<%i>.%s", node->nodeID, attribute.c_str());
                                                        exposedAttribute->name = attribute;
                                                        s_currentComposition->attributes.push_back(exposedAttribute);
                                                    }
                                                }
                                            }
                                            ImGui::EndMenu();
                                        }
                                        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("USE_EXISTING_ATTRIBUTE").c_str()).c_str())) {
                                            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("USE_EXISTING_ATTRIBUTE").c_str()).c_str());
                                            static std::string attributeFilter = "";
                                            ImGui::InputTextWithHint("##attributeFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_BY_NAME_OR_ATTRIBUTE_ID").c_str()).c_str(), &attributeFilter);
                                            auto parentComposition = Workspace::GetCompositionByNodeID(node->nodeID).value();
                                            bool oneCandidateWasDisplayed = false;
                                            bool mustShowByID = false;
                                            for (auto& parentAttribute : parentComposition->attributes) {
                                                if (std::to_string(parentAttribute->id) == ReplaceString(attributeFilter, " ", "")) {
                                                    mustShowByID = true;
                                                    break;
                                                }
                                            }
                                            for (auto& parentAttribute : parentComposition->attributes) {
                                                if (!mustShowByID) {
                                                    if (!attributeFilter.empty() && LowerCase(ReplaceString(parentAttribute->name, " ", "")).find(LowerCase(ReplaceString(attributeFilter, " ", ""))) == std::string::npos) continue;
                                                    if (parentAttribute->id == id) continue;
                                                } else {
                                                    if (!attributeFilter.empty() && std::to_string(parentAttribute->id) != ReplaceString(attributeFilter, " ", "")) continue;
                                                }
                                                ImGui::PushID(parentAttribute->id);
                                                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_LINK, parentAttribute->name.c_str()).c_str())) {
                                                    parentAttribute->internalAttributeName += (parentAttribute->internalAttributeName.empty() ? "" : " | ") + FormatString("<%i>.%s", node->nodeID, attribute.c_str());
                                                }
                                                oneCandidateWasDisplayed = true;
                                                ImGui::PopID();
                                            }
                                            if (!oneCandidateWasDisplayed) {
                                                ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(Localization::GetString("NOTHING_TO_SHOW").c_str()).x / 2.0f);
                                                ImGui::Text("%s", Localization::GetString("NOTHING_TO_SHOW").c_str());
                                            }
                                            ImGui::EndMenu();
                                        }
                                        ImGui::EndPopup();
                                    }
                                    ImGui::PopID();
                                }
                                ImGui::EndMenu();
                            }
                            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("NODE_DESCRIPTION").c_str()).c_str())) {
                                ImGui::InputTextWithHint("##nodeDescription", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("NODE_DESCRIPTION").c_str()).c_str(), &node->overridenHeader);
                                ImGui::SetItemTooltip("%s %s", ICON_FA_PENCIL, Localization::GetString("DESCRIPTION").c_str());
                                ImGui::EndMenu();
                            }
                            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("DELETE_SELECTED_NODES").c_str()).c_str(), "Delete")) {
                                Nodes::DeleteNode(node->nodeID);
                            }
                            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_COPY, Localization::GetString("COPY_SELECTED_NODES").c_str()).c_str(), "Ctrl+C")) {
                                ProcessCopyAction();
                            }
                            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PASTE, Localization::GetString("PASTE_SELECTED_NODES").c_str()).c_str(), "Ctrl+V")) {
                                ProcessPasteAction();
                            }
                            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_CLONE, Localization::GetString("DUPLICATE_SELECTED_NODES").c_str()).c_str(), "Ctrl+D")) {
                                ProcessCopyAction();
                                ProcessPasteAction();
                            }
                            ImGui::SameLine();
                            if (!nodeContextMenuWidth.has_value()) nodeContextMenuWidth = ImGui::GetCursorPosX();
                            ImGui::NewLine();
                        } else {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::PopFont();

                        ImGui::EndPopup();
                    } else nodeContextMenuWidth = std::nullopt;


                    previousDimPercentage = dimPercentage;
                    if (dimPercentage >= 0 && dimming) {
                        dimPercentage += ImGui::GetIO().DeltaTime * 2.5;
                        if (dimPercentage >= dimB) {
                            dimming = false;
                        }
                        dimPercentage = std::clamp(dimPercentage, 0.0f, dimB);
                    }
                    if (ImGui::BeginPopup("##createNewNode")) {
                        ImGui::PushFont(Font::s_normalFont);
                        if (!nodeSearchMousePos.has_value()) nodeSearchMousePos = s_mousePos;
                        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LAYER_GROUP, s_currentComposition->name.c_str()).c_str());
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("CREATE_NEW_NODE").c_str()).c_str(), "Tab")) {
                            openSearchPopup = true;
                        }
                        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("CREATE_NEW_ATTRIBUTE").c_str()).c_str())) {
                            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("ADD_ATTRIBUTE").c_str()).c_str());
                            for (auto& entry : Attributes::s_implementations) {
                                if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_PLUS, entry.description.prettyName.c_str(), Localization::GetString("ATTRIBUTE").c_str()).c_str())) {
                                    auto attributeCandidate = Attributes::InstantiateAttribute(entry.description.packageName);
                                    auto& project = Workspace::GetProject();
                                    if (attributeCandidate.has_value()) {
                                        s_currentComposition->attributes.push_back(attributeCandidate.value());
                                        project.selectedAttributes = {attributeCandidate.value()->id};
                                        auto attributeNode = Workspace::InstantiateNode(RASTER_PACKAGED "get_attribute_value").value();
                                        attributeNode->SetAttributeValue("AttributeID", s_currentComposition->attributes.back()->id);
                                        RASTER_SYNCHRONIZED(Workspace::s_nodesMutex);
                                        s_currentComposition->nodes[attributeNode->nodeID] = attributeNode;
                                        s_deferredNodeCreations.push_back(DeferredNodeCreation{
                                            .nodeID = attributeNode->nodeID,
                                            .position = nodeSearchMousePos.value_or(s_mousePos),
                                            .mustBeSelected = true
                                        });
                                    }
                                }
                            }
                            ImGui::EndMenu();
                        }
                        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_LAYER_GROUP, Localization::GetString("COMPOSITION_PROPERTIES").c_str()).c_str())) {
                            TimelineUI::RenderCompositionPopup(s_currentComposition);
                            ImGui::EndMenu();
                        }
                        ImGui::Separator();
                        if (Workspace::IsProjectLoaded()) {
                            auto& project = Workspace::GetProject();
                            if (!project.customData.contains("RenderingCompositionLock")) {
                                project.customData["RenderingCompositionLock"] = false;
                            }

                            bool compositionLock = project.customData["RenderingCompositionLock"];

                            if (ImGui::MenuItem(FormatString("%s %s", compositionLock ? ICON_FA_LOCK : ICON_FA_LOCK_OPEN, Localization::GetString("COMPOSITION_LOCK").c_str()).c_str())) {
                                compositionLock = !compositionLock;
                            }

                            project.customData["RenderingCompositionLock"] = compositionLock;
                        }
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_ROUTE, Localization::GetString("NAVIGATE_TO_NODE").c_str()).c_str(), "`")) {
                            openNavigateToNodePopup = true;
                        }
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
                    if (ImGui::Shortcut(ImGuiKey_GraveAccent)) {
                        openNavigateToNodePopup = true;
                    }
                    if (openSearchPopup) {
                        ImGui::OpenPopup("##createNewNodeSearch");
                        drawLastLine = true;
                    }
                    if (openNavigateToNodePopup) {
                        ImGui::OpenPopup("##navigateToNodePopup");
                    }
                    if (ImGui::BeginPopup("##createNewNodeSearch")) {
                        if (ignoreLastLine) drawLastLine = false;
                        if (!nodeSearchMousePos.has_value() || ignoreLastLine) nodeSearchMousePos = s_mousePos;
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
                        if (ImGui::BeginChild("##searchWrapper", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
                            if (ImGui::BeginChild("##tabChild", ImVec2(180, 0), ImGuiChildFlags_AutoResizeY)) {
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
                                if (ImGui::ButtonEx(FormatString("%s %s (%i)", ICON_FA_LIST, Localization::GetString("ALL_NODES").c_str(), allCategoryTotalNodes).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
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
                            }
                            ImGui::EndChild();
                            ImGui::SameLine(0, 6);
                            if (ImGui::BeginChild("##nodeCandidates", ImVec2(RASTER_PREFERRED_POPUP_WIDTH * 2, ImGui::GetContentRegionAvail().y))) {
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
                                                            InvalidatePinCache(nodeInputPin.pinID);
                                                            InvalidatePinCache(nodeInputPin.connectedPinID);
                                                            InvalidatePinCache(lastLinePin.pinID);
                                                            InvalidatePinCache(lastLinePin.connectedPinID);
                                                        }
                                                    } else {
                                                        if (!lastLineNode->inputPins.empty() && !node->outputPins.empty()) {
                                                            lastLinePin.connectedPinID = node->outputPins[0].pinID;
                                                            Workspace::UpdatePinByID(lastLinePin, lastLinePin.pinID);
                                                            InvalidatePinCache(lastLinePin.pinID);
                                                            InvalidatePinCache(lastLinePin.connectedPinID);
                                                        }
                                                    }
                                                }
                                                connectedLastLinePin = 0;
                                            }
                                        }
                                        ImGui::CloseCurrentPopup();
                                    }
                                }
                                if (!hasCandidates) {
                                    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 2.0f - ImGui::CalcTextSize(Localization::GetString("NOTHING_TO_SHOW").c_str()).x / 2.0f);
                                    ImGui::Text(Localization::GetString("NOTHING_TO_SHOW").c_str());
                                }
                            }
                            ImGui::EndChild();
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
                    ImGui::PushFont(Font::s_normalFont);
                    static bool s_navigateToNodeSearchFocused = false;
                    if (ImGui::BeginPopup("##navigateToNodePopup")) {
                        if ((ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_GraveAccent)) && !ImGui::IsWindowAppearing()) ImGui::CloseCurrentPopup();
                        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_ROUTE, Localization::GetString("NAVIGATE_TO_NODE").c_str()).c_str());
                        static std::string s_nodeFilter = "";
                        if (!s_navigateToNodeSearchFocused) {
                            ImGui::SetKeyboardFocusHere();
                            s_navigateToNodeSearchFocused = true;
                        }
                        ImGui::InputTextWithHint("##nodeSearchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_nodeFilter);
                        if (ImGui::BeginChild("##nodeCandidates", ImVec2(250, 300))) {
                            bool first = true;
                            for (auto& pair : s_currentComposition->nodes) {
                                auto& node = pair.second;
                                if (!s_nodeFilter.empty() && LowerCase(node->Header()).find(LowerCase(s_nodeFilter)) == std::string::npos) continue;
                                ImGui::PushID(node->nodeID);
                                    if ((ImGui::MenuItem(FormatString("%s %s", node->Icon().c_str(), node->Header().c_str()).c_str())) || (first && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                                        Nodes::SelectNode(node->nodeID);
                                        s_mustNavigateToSelection = true;
                                        ImGui::CloseCurrentPopup();
                                    }
                                ImGui::PopID();
                                first = false;
                            }
                        }
                        ImGui::EndChild();
                        ImGui::EndPopup();
                    } else s_navigateToNodeSearchFocused = false;
                    ImGui::PopFont();
                    Nodes::Resume();
                    if (Workspace::IsProjectLoaded()) {
                        Workspace::GetProject().selectedNodes.clear();
                    }

                    std::vector<Nodes::NodeId> temporarySelectedNodes(Nodes::GetSelectedObjectCount());
                    Nodes::GetSelectedNodes(temporarySelectedNodes.data(), Nodes::GetSelectedObjectCount());

                    if (Workspace::IsProjectLoaded()) {
                        auto& project = Workspace::GetProject();
                        for (auto& nodeID : temporarySelectedNodes) {
                            project.selectedNodes.push_back((int) nodeID.Get());
                        }
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

                    ImGui::PopFont();
                    Nodes::End();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    if (dimPercentage > 0.05f) {
                        drawList->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, dimPercentage)));
                    }
                }
                if (s_currentComposition) {
                    ImGui::SetCursorPos(ImGui::GetCursorStartPos() + ImVec2(5, 5));
                    ImGui::PushFont(Font::s_denseFont);
                        ImGui::SetWindowFontScale(1.7f);
                            ImGui::Text("%s %s", ICON_FA_LAYER_GROUP, s_currentComposition->name.c_str());
                        ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopFont();
                    static bool isDescriptionHovered = false;
                    static bool isEditingDescription = false;
                    ImGui::PushFont(Font::s_normalFont);
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
                    ImGui::PopFont();
                    if (ImGui::BeginPopup("##compositionDescriptionEditor")) {
                        ImGui::InputTextMultiline("##description", &s_currentComposition->description);
                        ImGui::SameLine();
                        if (ImGui::Button(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str())) {
                            isEditingDescription = false;
                            isDescriptionHovered = false;
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    } else {
                        isEditingDescription = false;
                    }
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
                            RASTER_SYNCHRONIZED(Workspace::s_nodesMutex);
                            s_currentComposition->nodes[attributeNode->nodeID] = attributeNode;
                            s_deferredNodeCreations.push_back(DeferredNodeCreation{
                                .nodeID = attributeNode->nodeID,
                                .position = nodeSearchMousePos.value_or(s_mousePos),
                                .mustBeSelected = true
                            });
                        }
                    }
                }
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                    int assetID = *((int*) payload->Data);
                    if (s_currentComposition) {
                        auto assetHandleNodeCandidate = Workspace::InstantiateNode(RASTER_PACKAGED "get_asset_id");
                        if (assetHandleNodeCandidate.has_value()) {
                            auto& assetHandleNode = assetHandleNodeCandidate.value();
                            assetHandleNode->SetAttributeValue("AssetID", assetID);
                            RASTER_SYNCHRONIZED(Workspace::s_nodesMutex);
                            s_currentComposition->nodes[assetHandleNode->nodeID] = assetHandleNode;
                            s_deferredNodeCreations.push_back(DeferredNodeCreation{
                                .nodeID = assetHandleNode->nodeID,
                                .position = nodeSearchMousePos.value_or(s_mousePos),
                                .mustBeSelected = true
                            });
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }
            Nodes::SetCurrentEditor(nullptr);

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
        
            if (s_compositionToAdd.has_value()) {
                auto& project = Workspace::GetProject();
                project.compositions.push_back(s_compositionToAdd.value());
                s_compositionToAdd = std::nullopt;
            }
        }
        
        ImGui::End();
    }
}