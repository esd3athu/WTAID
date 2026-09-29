#include "data/IndicatorsData.h"
#include <nlohmann/json.hpp>

std::optional<IndicatorsRawData> IndicatorsParser::parse(const std::string& jsonString)
{
    IndicatorsRawData data;

    try
    {
        nlohmann::json j = nlohmann::json::parse(jsonString);

        data.valid = true;

        if (j.contains("type"))
            data.type = j["type"].get<std::string>();

        if (j.contains("army"))
            data.army = j["army"].get<std::string>();

        if (j.contains("speed"))
            data.speed = static_cast<float>(j["speed"].get<double>());

        if (j.contains("fuel_consume"))
            data.fuelConsumeKgS = static_cast<float>(j["fuel_consume"].get<double>());
    }
    catch (...)
    {
        return std::nullopt;
    }

    return data;
}

float IndicatorsParser::readFloat(const nlohmann::json& j, const std::string& key, float defaultValue)
{
    try
    {
        if (j.contains(key))
            return static_cast<float>(j[key].get<double>());
    }
    catch (...)
    {
    }
    return defaultValue;
}
