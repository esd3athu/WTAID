#pragma once

#include "data/TelemetryData.h"
#include <string>
#include <optional>

#include <nlohmann/json.hpp>

/**
 * Парсер JSON-данных телеметрии в структуру TelemetryRawData.
 * Использует nlohmann/json для разбора JSON.
 */
class DataParser
{
public:
    /**
     * Распарсить JSON-строку в структуру телеметрии.
     * @param jsonString  JSON-строка от сервера War Thunder
     * @return TelemetryRawData если парсинг успешен, std::nullopt при ошибке
     */
    static std::optional<TelemetryRawData> parse(const std::string& jsonString);

private:
    static float readFloat(const nlohmann::json& j, const std::string& key, float defaultValue = 0.0f);
    static bool readBool(const nlohmann::json& j, const std::string& key, bool defaultValue = false);
};
