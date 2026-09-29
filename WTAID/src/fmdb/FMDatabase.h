#pragma once

#include "fmdb/FMDataModel.h"
#include <string>
#include <vector>

/**
 * База данных Flight Model (FM).
 * Загружает и парсит CSV-файлы с аэродинамическими данными.
 */
class FMDatabase
{
public:
    FMDatabase();

    /**
     * Загрузить базу данных из CSV-файлов.
     * @param dataPath  Путь к fm_data_db.csv
     * @param namesPath Путь к fm_names_db.csv
     * @return true если загрузка успешна
     */
    bool load(const std::string& dataPath, const std::string& namesPath);

    /**
     * Загрузить версию FM-базы.
     * @param versionPath  Путь к fm_version
     * @return true если загрузка успешна
     */
    bool loadVersion(const std::string& versionPath);

    /**
     * Найти данные по FM-имени.
     */
    const AircraftFMData* findByName(const std::string& name) const;

    /**
     * Найти данные по WT-имени (через маппинг).
     */
    const AircraftFMData* findByWTName(const std::string& wtName) const;

    /**
     * Получить список всех FM-имён.
     */
    std::vector<std::string> getAllNames() const;

    /**
     * Получить версию FM-базы.
     */
    std::string getVersion() const;

    /**
     * Проверить, загружена ли база данных.
     */
    bool isLoaded() const;

private:
    std::unordered_map<std::string, AircraftFMData> data_;
    std::unordered_map<std::string, std::string> nameMapping_; // WT name -> FM name
    std::vector<VehicleNameMapping> vehicleMappings_;
    std::string version_;

    // Парсинг CSV
    bool parseDataCSV(const std::string& path);
    bool parseNamesCSV(const std::string& path);

    // Вспомогательные методы
    static std::vector<std::string> splitString(const std::string& str, char delimiter);
    static std::vector<float> parseFloatList(const std::string& str);
    static std::string trim(const std::string& str);
};
