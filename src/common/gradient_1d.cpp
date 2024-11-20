#include "common/gradient_1d.h"

namespace Raster {

    Gradient1D::Gradient1D() {
        this->stops.push_back(GradientStop1D(0, glm::vec4(glm::vec3(0), 1)));
        this->stops.push_back(GradientStop1D(1, glm::vec4(1)));
    }

    Gradient1D::Gradient1D(Json t_data) {
        for (auto& stop : t_data) {
            AddStop(stop["Percentage"], glm::vec4(stop["Color"][0].get<float>(), stop["Color"][1].get<float>(), stop["Color"][2].get<float>(), stop["Color"][3].get<float>()));
            stops[stops.size() - 1].id = stop["ID"];
        }
    }

    void Gradient1D::AddStop(float t_percentage, glm::vec4 t_color) {
        for (auto& stop : stops) {
            if (stop.percentage == t_percentage) {
                stop.color = t_color;
                stop.id = Randomizer::GetRandomInteger();
                return;
            }
        }
        this->stops.push_back(GradientStop1D(t_percentage, t_color));
        SortStops();
    }

    glm::vec4 Gradient1D::Get(float t_percentage) {
        int targetKeyframeIndex = -1;
        int stopsLength = stops.size();
        float renderViewTime = t_percentage;

        for (int i = 0; i < stopsLength; i++) {
            float keyframeTimestamp = stops.at(i).percentage;
            if (renderViewTime <= keyframeTimestamp) {
                targetKeyframeIndex = i;
                break;
            }
        }

        if (targetKeyframeIndex == -1) {
            return stops[0].color;
        }

        if (targetKeyframeIndex == 0) {
            return stops[0].color;
        }

        float keyframeTimestamp = stops.at(targetKeyframeIndex).percentage;
        float interpolationPercentage = 0;
        if (targetKeyframeIndex == 1) {
            interpolationPercentage = renderViewTime / keyframeTimestamp;
        } else {
            float previousFrame = stops.at(targetKeyframeIndex - 1).percentage;
            interpolationPercentage = (renderViewTime - previousFrame) / (keyframeTimestamp - previousFrame);
        }

        auto& beginKeyframeValue = stops.at(targetKeyframeIndex - 1).color;
        auto& endkeyframeValue = stops.at(targetKeyframeIndex).color;

        return glm::mix(beginKeyframeValue, endkeyframeValue, interpolationPercentage);
    }

    void Gradient1D::SortStops() {
        for (int step = 0; step < stops.size() - 1; ++step) {
            for (int i = 1; i < stops.size() - step - 1; ++i) {
                if (stops.at(i).percentage > stops.at(i + 1).percentage ) {
                    std::swap(stops.at(i), stops.at(i + 1));
                }
            }
        }
    }

    std::vector<RawGradientStop1D> Gradient1D::GetRawStops() {
        std::vector<RawGradientStop1D> result;
        for (auto& stop : stops) {
            result.push_back(RawGradientStop1D(
                stop.percentage, stop.color
            ));
        }
        return result;
    }

    void Gradient1D::FillToBuffer(char* t_buffer) {
        *((float*) t_buffer) = (float) stops.size();
        t_buffer += sizeof(float);

        auto rawStops = GetRawStops();
        for (auto& stop : rawStops) {
            *((float*) t_buffer) = stop.percentage;
            t_buffer += sizeof(float);

            *((float*) t_buffer) = stop.color.r;
            t_buffer += sizeof(float);

            *((float*) t_buffer) = stop.color.g;
            t_buffer += sizeof(float);

            *((float*) t_buffer) = stop.color.b;
            t_buffer += sizeof(float);

            *((float*) t_buffer) = stop.color.a;
            t_buffer += sizeof(float);
        }
    }


    Gradient1D Gradient1D::MatchStopsCount(Gradient1D& t_reference) {
        Gradient1D result;
        result.stops.clear();
        int baseID = t_reference.stops[0].id;
        int index = 0;
        for (auto& stop : t_reference.stops) {
            result.AddStop(stop.percentage, Get(stop.percentage));
            (*result.stops.end()).id = baseID + (index++);
        }
        result.SortStops();
        return result;
    }

    Json Gradient1D::Serialize() {
        Json result = Json::array();
        for (auto& stop : stops) {
            result.push_back({
                {"ID", stop.id},
                {"Percentage", stop.percentage},
                {"Color", {stop.color.x, stop.color.y, stop.color.z, stop.color.w}}
            });
        }
        return result;
    }
};