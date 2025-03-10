#include "transform2d_attribute.h"
#include "common/asset_base.h"
#include "common/workspace.h"
#include "common/asset_id.h"
#include "common/ui_helpers.h"

namespace Raster {
    Transform2DAttribute::Transform2DAttribute() {
        AttributeBase::Initialize();

        Transform2D transform;
        transform.size = {1, 1};

        keyframes.push_back(
            AttributeKeyframe(
                0,
                transform
            )
        );

        this->m_parentAttributeID = -1;
        this->m_parentAssetID = -1;
    }

    std::any Transform2DAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        Transform2D a = std::any_cast<Transform2D>(t_beginValue);
        Transform2D b = std::any_cast<Transform2D>(t_endValue);
        float t = t_percentage;
        auto& project = Workspace::GetProject();

        Transform2D result;
        result.position = glm::mix(a.position, b.position, t);
        result.size = glm::mix(a.size, b.size, t);
        result.anchor = glm::mix(a.anchor, b.anchor, t);
        result.angle = glm::mix(a.angle, b.angle, t);
        result.scale = glm::mix(a.scale, b.scale, t);

        auto parentAttributeCandidate = Workspace::GetAttributeByAttributeID(m_parentAttributeID);
        if (parentAttributeCandidate.has_value()) {
            auto& parentAttribute = parentAttributeCandidate.value();
            auto parentComposition = Workspace::GetCompositionByAttributeID(parentAttribute->id).value();
            auto dynamicValue = parentAttribute->Get(project.GetCorrectCurrentTime() - parentComposition->beginFrame, parentComposition);

            if (dynamicValue.type() == typeid(Transform2D)) {
                auto transform = std::any_cast<Transform2D>(dynamicValue);
                result.parentTransform = std::make_shared<Transform2D>(transform);
            }
        }

        std::optional<AbstractAsset> parentAssetCandidate = std::nullopt;

        if (m_parentAssetID > 0) {
            if (m_parentAssetType == ParentAssetType::Asset) {
                parentAssetCandidate = Workspace::GetAssetByAssetID(m_parentAssetID);
            } else if (m_parentAssetType == ParentAssetType::Attribute) {
                auto assetAttributeCandidate = Workspace::GetAttributeByAttributeID(m_parentAssetID);
                if (assetAttributeCandidate.has_value()) {
                    auto& assetAttribute = assetAttributeCandidate.value();
                    auto assetIDCandidate = assetAttribute->Get(project.GetCorrectCurrentTime() - composition->beginFrame, composition);
                    if (assetIDCandidate.type() == typeid(int) || assetIDCandidate.type() == typeid(AssetID)) {
                        auto assetID = assetIDCandidate.type() == typeid(AssetID) ? std::any_cast<AssetID>(assetIDCandidate).id : std::any_cast<int>(assetIDCandidate);
                        parentAssetCandidate = Workspace::GetAssetByAssetID(assetID);
                    }
                }
            }
        }


        if (parentAssetCandidate.has_value()) {
            auto& parentAsset = parentAssetCandidate.value();
            auto textureCandidate = parentAsset->GetPreviewTexture();
            if (textureCandidate.has_value()) {
                auto& texture = textureCandidate.value();
                float aspectRatio = (float) texture.width / (float) texture.height;

                Transform2D correctedTransform;
                correctedTransform.size = glm::vec2(aspectRatio, 1.0f);
                correctedTransform.parentTransform = std::make_shared<Transform2D>(result);

                result = correctedTransform;
            }
        }

        return result;
    }

    Json Transform2DAttribute::SerializeKeyframeValue(std::any t_value) {
        return std::any_cast<Transform2D>(t_value).Serialize();
    }  

    std::any Transform2DAttribute::LoadKeyframeValue(Json t_value) {
        return Transform2D(t_value);
    }

    void Transform2DAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }

    std::any Transform2DAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        auto& project = Workspace::s_project.value();
        Transform2D transform = std::any_cast<Transform2D>(t_originalValue);


        if (m_parentAssetID > 0) {
            if (m_parentAssetType == ParentAssetType::Asset) {
                auto testAssetCandidate = Workspace::GetAssetByAssetID(m_parentAssetID);
                if (testAssetCandidate.has_value()) {
                    if (transform.parentTransform) {
                        transform = *transform.parentTransform;
                    }
                }
            } else if (m_parentAssetType == ParentAssetType::Attribute) {
                auto assetAttributeCandidate = Workspace::GetAttributeByAttributeID(m_parentAssetID);
                if (assetAttributeCandidate.has_value()) {
                    auto& assetAttribute = assetAttributeCandidate.value();
                    auto assetIDCandidate = assetAttribute->Get(project.GetCorrectCurrentTime() - composition->beginFrame, composition);
                    if (assetIDCandidate.type() == typeid(int) || assetIDCandidate.type() == typeid(AssetID)) {
                        auto assetID = assetIDCandidate.type() == typeid(AssetID) ? std::any_cast<AssetID>(assetIDCandidate).id : std::any_cast<int>(assetIDCandidate);
                        auto testAssetCandidate = Workspace::GetAssetByAssetID(assetID);
                        if (testAssetCandidate.has_value()) {
                            if (transform.parentTransform) {
                                transform = *transform.parentTransform;
                            }
                        }
                    }
                }
            }
        }
        auto parentAttributeCandidate = Workspace::GetAttributeByAttributeID(m_parentAttributeID);
        std::string buttonText = FormatString("%s %s | %s %s", ICON_FA_UP_DOWN_LEFT_RIGHT, Localization::GetString("EDIT_VALUE").c_str(), ICON_FA_SITEMAP, parentAttributeCandidate.has_value() ? parentAttributeCandidate.value()->name.c_str() : Localization::GetString("NO_PARENT").c_str());
        if (!project.customData.contains("Transform2DAttributeData")) {
            project.customData["Transform2DAttributeData"] = {};
        }
        auto& customData = project.customData["Transform2DAttributeData"];
        auto stringID = std::to_string(id);
        if (!customData.contains(stringID)) {
            customData[stringID] = false;
        }
        bool linkedSize = customData[stringID];
        if (ImGui::Button(linkedSize ? ICON_FA_LINK : ICON_FA_LINK_SLASH)) {
            linkedSize = !linkedSize;
        }
        ImGui::SetItemTooltip("%s %s", ICON_FA_LINK, "Link Dimensions");
        ImGui::SameLine(0, 2);
        if (ImGui::Button(buttonText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0))) {
            ImGui::OpenPopup(FormatString("##%iattribute", id).c_str());
        }
        ImGui::SetItemTooltip("%s %s", ICON_FA_ARROW_POINTER, Localization::GetString("RIGHT_CLICK_FOR_CONTEXT_MENU").c_str());

        std::string popupID = FormatString("##transform2DParentAttribute%i", id);

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(popupID.c_str());
        }

        bool openMoreAttributesPopup = false;
        if (ImGui::BeginPopup(popupID.c_str())) {
            std::string buttonText = FormatString("%s %s", ICON_FA_UP_DOWN_LEFT_RIGHT, name.c_str());
            if (parentAttributeCandidate.has_value()) {
                buttonText += FormatString(" | %s %s", ICON_FA_SITEMAP, parentAttributeCandidate.value()->name.c_str());
            }
            ImGui::SeparatorText(buttonText.c_str());
            openMoreAttributesPopup = RenderParentAttributePopup();
            ImGui::EndPopup();
        }

        if (openMoreAttributesPopup) {
            ImGui::OpenPopup(FormatString("%imoreAttributes", id).c_str());
        }
        if (ImGui::BeginPopup(FormatString("%imoreAttributes", id).c_str())) {
            RenderMoreAttributesPopup();
            ImGui::EndPopup();
        }
        
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(TRANSFORM2D_PARENT_PAYLOAD)) {
                m_parentAttributeID = *((int*) payload->Data);
            }

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_ATTRIBUTE_DRAG_DROP_PAYLOAD)) {
                int assetAttributeID = *((int*) payload->Data);
                m_parentAssetType = ParentAssetType::Attribute;
                m_parentAssetID = assetAttributeID;
            }

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                int assetID = *((int*) payload->Data);
                m_parentAssetType = ParentAssetType::Asset;
                m_parentAssetID = assetID;
            }

            ImGui::EndDragDropTarget();
        }
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(TRANSFORM2D_PARENT_PAYLOAD, &id, sizeof(id));
            ImGui::Text("%s %s: %s", ICON_FA_SITEMAP, Localization::GetString("SET_AS_PARENT_ATTRIBUTE").c_str(), name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginPopup(FormatString("##%iattribute", id).c_str())) {
            ImGui::SeparatorText(FormatString("%s Edit Value: %s", ICON_FA_UP_DOWN_LEFT_RIGHT, name.c_str()).c_str());
            
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Position", ICON_FA_UP_DOWN_LEFT_RIGHT);
            ImGui::SameLine();
            float cursorX = ImGui::GetCursorPosX();
            ImGui::DragFloat2("##dragPosition", glm::value_ptr(transform.position), 0.05f);
            isItemEdited = isItemEdited || ImGui::IsItemEdited();
            

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Size", ICON_FA_SCALE_BALANCED);
            ImGui::SameLine();
            ImGui::SetCursorPosX(cursorX);
            if (ImGui::Button(linkedSize ? ICON_FA_LINK : ICON_FA_LINK_SLASH)) {
                linkedSize = !linkedSize;
            }
            ImGui::SetItemTooltip("%s %s", ICON_FA_LINK, "Link Dimensions");
            ImGui::SameLine(0, 2.0f);
            glm::vec2 reservedSize = transform.size;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragFloat2("##dragSize", glm::value_ptr(transform.size), 0.05f);
            ImGui::PopItemWidth();
            isItemEdited = isItemEdited || ImGui::IsItemEdited();
            if (linkedSize) {
                if (reservedSize.x != transform.size.x) {
                    transform.size.y = transform.size.x;
                } else if (reservedSize.y != transform.size.y) {
                    transform.size.x = transform.size.y;
                }
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Scale", ICON_FA_EXPAND);
            ImGui::SameLine();
            ImGui::SetCursorPosX(cursorX);
            ImGui::DragFloat("##dragScale", &transform.scale, 0.05f);
            isItemEdited = isItemEdited || ImGui::IsItemEdited();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Anchor", ICON_FA_ANCHOR);
            ImGui::SameLine();
            ImGui::SetCursorPosX(cursorX);
            ImGui::DragFloat2("##dragAnchor", glm::value_ptr(transform.anchor), 0.05f);
            isItemEdited = isItemEdited || ImGui::IsItemEdited();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Rotation", ICON_FA_ROTATE);
            ImGui::SameLine();
            ImGui::SetCursorPosX(cursorX);
            ImGui::DragFloat("##dragAngle", &transform.angle, 0.5f);
            isItemEdited = isItemEdited || ImGui::IsItemEdited();
            
            std::string parentAttributePopupID = FormatString("##parentAttributeOptionsPopup%i", id);
            std::string moreParentAttributesPopupID = FormatString("##moreParentAttributesOptionsPopup%i", id);
            if (ImGui::Button(FormatString("%s %s", ICON_FA_SITEMAP, parentAttributeCandidate.has_value() ? parentAttributeCandidate.value()->name.c_str() : Localization::GetString("NO_PARENT").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                ImGui::OpenPopup(parentAttributePopupID.c_str());
            }

            bool internalOpenMoreAttributesPopup = false;
            if (ImGui::BeginPopup(parentAttributePopupID.c_str())) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_UP_DOWN_LEFT_RIGHT, name.c_str()).c_str());
                internalOpenMoreAttributesPopup = RenderParentAttributePopup();
                ImGui::EndPopup();
            }

            if (internalOpenMoreAttributesPopup) {
                ImGui::OpenPopup(moreParentAttributesPopupID.c_str());
            }
            
            if (ImGui::BeginPopup(moreParentAttributesPopupID.c_str())) {
                RenderMoreAttributesPopup();
                ImGui::EndPopup();
            }


            ImGui::EndPopup();
        }

        customData[stringID] = linkedSize;
        return transform;
    }

    Json Transform2DAttribute::AbstractSerialize() {
        return {
            {"ParentAttributeID", m_parentAttributeID},
            {"ParentAssetID", m_parentAssetID},
            {"ParentAssetType", static_cast<int>(m_parentAssetType)}
        };
    }

    bool Transform2DAttribute::RenderParentAttributePopup() {
        bool openMoreAttributesPopup = false;
        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("SELECT_PARENT_ATTRIBUTE").c_str()).c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("SELECT_PARENT_ATTRIBUTE").c_str()).c_str());
            static std::string attributeFilter = "";
            ImGui::InputTextWithHint("##attributeFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_BY_NAME_OR_ATTRIBUTE_ID").c_str()).c_str(), &attributeFilter);
            auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
            bool oneCandidateWasDisplayed = false;
            bool mustShowByID = false;
            for (auto& attribute : parentComposition->attributes) {
                if (std::to_string(attribute->id) == ReplaceString(attributeFilter, " ", "")) {
                    mustShowByID = true;
                    break;
                }
            }
            for (auto& attribute : parentComposition->attributes) {
                if (!mustShowByID) {
                    if (!attributeFilter.empty() && LowerCase(ReplaceString(attribute->name, " ", "")).find(LowerCase(ReplaceString(attributeFilter, " ", ""))) == std::string::npos) continue;
                    if (attribute->id == id) continue;
                } else {
                    if (!attributeFilter.empty() && std::to_string(attribute->id) != ReplaceString(attributeFilter, " ", "")) continue;
                }
                ImGui::PushID(attribute->id);
                if (ImGui::MenuItem(FormatString("%s%s %s", m_parentAttributeID == attribute->id ? ICON_FA_CHECK " " : "", ICON_FA_LINK, attribute->name.c_str()).c_str())) {
                    m_parentAttributeID = attribute->id;
                }
                oneCandidateWasDisplayed = true;
                ImGui::PopID();
            }
            if (!oneCandidateWasDisplayed) {
                ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(Localization::GetString("NOTHING_TO_SHOW").c_str()).x / 2.0f);
                ImGui::Text("%s", Localization::GetString("NOTHING_TO_SHOW").c_str());
            }
            ImGui::Separator();
            if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_GLOBE, ICON_FA_LINK, Localization::GetString("MORE_ATTRIBUTES").c_str()).c_str())) {
                openMoreAttributesPopup = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("SELECT_PARENT_ASSET").c_str()).c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("SELECT_PARENT_ASSET").c_str()).c_str());
            static std::string assetFilter = "";
            ImGui::InputTextWithHint("##attributeFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &assetFilter);
            auto& project = Workspace::GetProject();
            std::function<void(std::vector<AbstractAsset>&)> renderAssets = [&](std::vector<AbstractAsset>& t_assets) {
                bool hasCandidates = false;
                for (auto& asset : t_assets) {
                    if (!assetFilter.empty() && LowerCase(ReplaceString(asset->name, " ", "")).find(LowerCase(ReplaceString(assetFilter, " ", ""))) == std::string::npos) continue;
                    hasCandidates = true;
                    auto implementation = Assets::GetAssetImplementationByPackageName(asset->packageName).value();
                    auto childAssetsCandidate = asset->GetChildAssets();
                    ImGui::PushID(asset->id);
                        if (!childAssetsCandidate) {
                            if (ImGui::MenuItem(FormatString("%s %s", implementation.description.icon.c_str(), asset->name.c_str()).c_str())) {
                                m_parentAssetID = asset->id;
                            }
                        } else {
                            if (ImGui::TreeNode(FormatString("%s %s", implementation.description.icon.c_str(), asset->name.c_str()).c_str())) {
                                renderAssets(**childAssetsCandidate);
                                ImGui::TreePop();
                            }
                        }
                    ImGui::PopID();
                }
                if (!hasCandidates) {
                    UIHelpers::RenderNothingToShowText();
                }
            };
            renderAssets(project.assets);
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("REMOVE_PARENT_ATTRIBUTE").c_str()).c_str())) {
            m_parentAttributeID = -1;
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("REMOVE_PARENT_ASSET").c_str()).c_str())) {
            m_parentAssetID = -1;
        }
        return openMoreAttributesPopup;
    }

    void Transform2DAttribute::RenderMoreAttributesPopup() {
        ImGui::SeparatorText(FormatString("%s %s %s", ICON_FA_GLOBE, ICON_FA_LINK, Localization::GetString("SELECT_EXTERNAL_PARENT_ATTRIBUTE").c_str()).c_str());
        static std::string attributeFilter = "";
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputTextWithHint("##attributeFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_BY_NAME_OR_ATTRIBUTE_ID").c_str()).c_str(), &attributeFilter);
        ImGui::PopItemWidth();

        auto& project = Workspace::s_project.value();
        for (auto& composition : project.compositions) {
            if (composition.attributes.empty()) continue;
            bool skip = !attributeFilter.empty();
            bool mustShowByID = false;
            for (auto& attribute : composition.attributes) {
                if (std::to_string(attribute->id) == ReplaceString(attributeFilter, " ", "")) {
                    mustShowByID = true;
                    skip = false;
                    break;
                }
                if (!attributeFilter.empty() && attribute->name.find(attributeFilter) != std::string::npos) {
                    skip = false;
                    break;
                }
            }
            if (skip) continue;
            ImGui::PushID(composition.id);
            if (ImGui::TreeNode(FormatString("%s %s", ICON_FA_LAYER_GROUP, composition.name.c_str()).c_str())) {
                for (auto& attribute : composition.attributes) {
                    if (mustShowByID && ReplaceString(attributeFilter, " ", "") != std::to_string(attribute->id)) continue;
                    ImGui::PushID(attribute->id);
                    std::string attributeIcon = ICON_FA_GLOBE " " ICON_FA_LINK;
                    if (Workspace::GetCompositionByAttributeID(id).value()->id == composition.id) {
                        attributeIcon = ICON_FA_LINK;
                    }
                    if (ImGui::MenuItem(FormatString("%s %s", attributeIcon.c_str(), attribute->name.c_str()).c_str())) {
                        m_parentAttributeID = attribute->id;
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    void Transform2DAttribute::Load(Json t_data) {
        m_parentAttributeID = t_data["ParentAttributeID"];
        m_parentAssetID = t_data["ParentAssetID"];
        m_parentAssetType = static_cast<ParentAssetType>(t_data["ParentAssetType"].get<int>());
    }

    void Transform2DAttribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
    }

    void Transform2DAttribute::AbstractRenderPopup() {
        if (RenderParentAttributePopup()) {
            ImGui::OpenPopup(FormatString("%imoreAttributes", id).c_str());
        }
        ImGui::Separator();
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::Transform2DAttribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "transform2d_attribute",
            .prettyName = ICON_FA_UP_DOWN_LEFT_RIGHT " Transform2D"
        };
    }
}