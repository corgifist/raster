#include "merge.h"
#include "common/dispatchers.h"

namespace Raster {

    Merge::Merge() {
        NodeBase::Initialize();

        SetupAttribute("A", Framebuffer());
        SetupAttribute("B", Framebuffer());
        SetupAttribute("Opacity", 1.0f);
        SetupAttribute("BlendingMode", std::string(""));

        AddInputPin("A");
        AddInputPin("B");
        AddOutputPin("Output");
    }

    Merge::~Merge() {
        if (m_framebuffer.handle) {
            for (auto& attachment : m_framebuffer.attachments) {
                GPU::DestroyTexture(attachment);
            }
            GPU::DestroyFramebuffer(m_framebuffer);
        }
    }

    AbstractPinMap Merge::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto aCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("A"));
        auto bCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("B"));
        auto opacityCandidate = GetAttribute<float>("Opacity");
        auto blendingModeCandidate = GetAttribute<std::string>("BlendingMode");

        if (aCandidate.has_value() && bCandidate.has_value() && opacityCandidate.has_value() && blendingModeCandidate.has_value()) {
            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);
            auto& a = aCandidate.value();
            auto& b = bCandidate.value();
            auto& opacity = opacityCandidate.value();
            auto& blendingMode = blendingModeCandidate.value();

            std::vector<CompositorTarget> targets;
            targets.push_back(CompositorTarget{
                .colorAttachment = a.attachments.at(0),
                .uvAttachment = a.attachments.size() > 1 ? a.attachments.at(1) : Texture(),
                .opacity = 1.0f,
                .blendMode = "",
                .compositionID = -1
            });
            targets.push_back(CompositorTarget{
                .colorAttachment = b.attachments.at(0),
                .uvAttachment = b.attachments.size() > 1 ? b.attachments.at(1) : Texture(),
                .opacity = opacity,
                .blendMode = blendingMode,
                .compositionID = -1
            });
            Compositor::PerformManualComposition(targets, m_framebuffer, glm::vec4(0));

            TryAppendAbstractPinMap(result, "Output", m_framebuffer);
        }

        return result;
    }

    void Merge::AbstractRenderProperties() {
        RenderAttributeProperty("Opacity");

        auto reservedPropertyDispatcher = Dispatchers::s_propertyDispatchers[typeid(std::string)];
        Dispatchers::s_propertyDispatchers[typeid(std::string)] = Merge::DispatchStringAttribute;
        RenderAttributeProperty("BlendingMode");
        Dispatchers::s_propertyDispatchers[typeid(std::string)] = reservedPropertyDispatcher;
    }

    void Merge::DispatchStringAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        auto& blending = Compositor::s_blending;

        std::string blendMode = std::any_cast<std::string>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        
        auto currentBlendModeCandidate = blending.GetModeByCodeName(blendMode);
        std::string blendingModeSelectorText = FormatString("%s %s: %s", ICON_FA_DROPLET, Localization::GetString("BLENDING_MODE").c_str(), currentBlendModeCandidate.has_value() ? currentBlendModeCandidate.value().name.c_str() : Localization::GetString("NONE").c_str());
        std::string blendingSelectorPopupID = FormatString("##blendingSelectorPopup%i", t_owner->nodeID);
        if (ImGui::Button(blendingModeSelectorText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            ImGui::OpenPopup(blendingSelectorPopupID.c_str());
        }

        if (ImGui::BeginPopup(blendingSelectorPopupID.c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_DROPLET, Localization::GetString("BLENDING_MODE").c_str()).c_str());
            static std::string s_searchFilter = "";
            ImGui::InputTextWithHint("##blendingFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_searchFilter);
            if (ImGui::BeginChild("##blendingModesContainer", ImVec2(ImGui::GetContentRegionAvail().x, 300))) {
                if (ImGui::Selectable(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("NORMAL").c_str()).c_str())) {
                    blendMode = "";
                    ImGui::CloseCurrentPopup();
                }
                for (auto& mode : blending.modes) {
                    if (!s_searchFilter.empty() && LowerCase(mode.name).find(LowerCase(s_searchFilter)) == std::string::npos) continue;
                    if (ImGui::MenuItem(FormatString("%s %s", Font::GetIcon(mode.icon).c_str(), mode.name.c_str()).c_str())) {
                        blendMode = mode.codename;
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }

        t_value = blendMode;
    }

    void Merge::AbstractLoadSerialized(Json t_data) {
        SetAttributeValue("BlendingMode", t_data["BlendingMode"].get<std::string>());
        SetAttributeValue("Opacity", t_data["Opacity"].get<float>());
    }

    Json Merge::AbstractSerialize() {
        return {
            {"BlendingMode", RASTER_ATTRIBUTE_CAST(std::string, "BlendingMode")},
            {"Opacity", RASTER_ATTRIBUTE_CAST(float, "Opacity")}
        };
    }

    bool Merge::AbstractDetailsAvailable() {
        return false;
    }

    std::string Merge::AbstractHeader() {
        return "Merge";
    }

    std::string Merge::Icon() {
        return ICON_FA_IMAGES;
    }

    std::optional<std::string> Merge::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Merge>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Merge",
            .packageName = RASTER_PACKAGED "merge",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    } 
}