#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

/**
 * Структура данных из /indicators (имя ТС и базовые параметры).
 */
struct IndicatorsRawData
{
    bool valid = false;
    std::string army;       // "air", "navy", "ground"
    std::string type;       // WT-имя ТС (например "su_30mk2v_venezuela")
    float speed = 0.0f;
    float fuelConsumeKgS = 0.0f;  // расход топлива, кг/с
};

/**
 * Парсер данных из /indicators.
 */
class IndicatorsParser
{
public:
    static std::optional<IndicatorsRawData> parse(const std::string& jsonString);

private:
    static float readFloat(const nlohmann::json& j, const std::string& key, float defaultValue = 0.0f);
};
