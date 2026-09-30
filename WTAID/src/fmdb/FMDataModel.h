#pragma once

#include <string>
#include <vector>
#include <unordered_map>

/**
 * Структура данных Flight Model (FM) для одного транспортного средства.
 * Загружается из fm_data_db.csv
 */
struct AircraftFMData
{
    std::string name;             // имя из fm_data_db.csv

    // Габариты
    float length = 0.0f;          // длина, м
    float wingspan = 0.0f;        // размах крыла, м
    float wingArea = 0.0f;        // площадь крыла, м²
    float emptyMass = 0.0f;       // масса пустого, кг
    float maxFuelMass = 0.0f;     // макс. масса топлива, кг

    // Критические скорости
    float critAirSpd = 0.0f;      // критическая воздушная скорость, км/ч
    float critAirSpdMach = 0.0f;  // критическая скорость (Мах)
    float critGearSpd = 0.0f;     // критическая скорость шасси, км/ч

    // Закрылки
    float combatFlaps = 0.0f;     // боевые закрылки (%)
    float takeoffFlaps = 0.0f;    // взлётные закрылки (%)
    float critFlapsSpeed = 0.0f;  // критическая скорость закрылков, км/ч

    // Перегрузка
    float critWingOverload = 0.0f; // критическая перегрузка крыла, g

    // Двигатели
    int numEngines = 0;           // количество двигателей
    std::vector<float> rpmRange;  // диапазон RPM (мин, опт, макс)

    // Нитро
    float maxNitro = 0.0f;        // макс. нитро, кг
    float nitroConsumption = 0.0f; // расход нитро, кг/с

    // Критические углы атаки
    std::vector<float> critAoA;   // критический угол атаки (4 значения)
};

/**
 * Структура маппинга имён из fm_names_db.csv.
 */
struct VehicleNameMapping
{
    std::string wtName;           // WT-имя (например "F-16C_50")
    std::string fmName;           // FM-имя (например "f_16c_block_50")
    std::string type;             // тип: "fighter", "bomber", "strike" и т.д.
    std::string englishName;      // английское название
};
