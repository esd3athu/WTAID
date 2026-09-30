#include "fmdb/FMDatabase.h"

#include <fstream>
#include <sstream>
#include <algorithm>

FMDatabase::FMDatabase()
{
}

bool FMDatabase::load(const std::string& dataPath, const std::string& namesPath)
{
    bool result = parseDataCSV(dataPath);
    result = parseNamesCSV(namesPath) && result;
    return result;
}

bool FMDatabase::loadVersion(const std::string& versionPath)
{
    std::ifstream file(versionPath);
    if (!file.is_open())
        return false;

    std::stringstream ss;
    ss << file.rdbuf();
    version_ = ss.str();

    // Убираем лишние пробелы и переносы
    version_ = trim(version_);

    return !version_.empty();
}

const AircraftFMData* FMDatabase::findByName(const std::string& name) const
{
    auto it = data_.find(name);
    if (it != data_.end())
        return &it->second;
    return nullptr;
}

const AircraftFMData* FMDatabase::findByWTName(const std::string& wtName) const
{
    auto it = nameMapping_.find(wtName);
    if (it != nameMapping_.end())
    {
        return findByName(it->second);
    }
    return nullptr;
}

std::vector<std::string> FMDatabase::getAllNames() const
{
    std::vector<std::string> names;
    names.reserve(data_.size());
    for (const auto& pair : data_)
        names.push_back(pair.first);
    return names;
}

std::string FMDatabase::getVersion() const
{
    return version_;
}

bool FMDatabase::isLoaded() const
{
    return !data_.empty();
}

std::vector<std::string> FMDatabase::splitString(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter))
        tokens.push_back(token);
    return tokens;
}

std::vector<float> FMDatabase::parseFloatList(const std::string& str)
{
    std::vector<float> result;
    auto parts = splitString(str, ',');
    for (const auto& part : parts)
    {
        std::string trimmed = trim(part);
        if (!trimmed.empty())
        {
            try
            {
                result.push_back(std::stof(trimmed));
            }
            catch (...)
            {
            }
        }
    }
    return result;
}

std::string FMDatabase::trim(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool FMDatabase::parseDataCSV(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::string line;
    bool isFirstLine = true;

    while (std::getline(file, line))
    {
        if (isFirstLine)
        {
            isFirstLine = false;
            continue; // Пропускаем заголовок
        }

        line = trim(line);
        if (line.empty())
            continue;

        auto fields = splitString(line, ';');
        if (fields.size() < 17)
            continue;

        AircraftFMData aircraft;
        aircraft.name = trim(fields[0]);

        try
        {
            aircraft.length = std::stof(trim(fields[1]));
            aircraft.wingspan = std::stof(trim(fields[2]));
            aircraft.wingArea = std::stof(trim(fields[3]));
            aircraft.emptyMass = std::stof(trim(fields[4]));
            aircraft.maxFuelMass = std::stof(trim(fields[5]));
            aircraft.critAirSpd = std::stof(trim(fields[6]));
            aircraft.critAirSpdMach = std::stof(trim(fields[7]));
            aircraft.critGearSpd = std::stof(trim(fields[8]));
            aircraft.combatFlaps = std::stof(trim(fields[9]));
            aircraft.takeoffFlaps = std::stof(trim(fields[10]));
            aircraft.critFlapsSpeed = std::stof(trim(fields[11]));
            aircraft.critWingOverload = std::stof(trim(fields[12]));

            aircraft.numEngines = std::stoi(trim(fields[13]));
            aircraft.rpmRange = parseFloatList(fields[14]);
            aircraft.maxNitro = std::stof(trim(fields[15]));
            aircraft.nitroConsumption = std::stof(trim(fields[16]));

            if (fields.size() > 17)
                aircraft.critAoA = parseFloatList(fields[17]);
        }
        catch (...)
        {
            continue;
        }

        data_[aircraft.name] = aircraft;
    }

    return !data_.empty();
}

bool FMDatabase::parseNamesCSV(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::string line;
    bool isFirstLine = true;

    while (std::getline(file, line))
    {
        if (isFirstLine)
        {
            isFirstLine = false;
            continue; // Пропускаем заголовок
        }

        line = trim(line);
        if (line.empty())
            continue;

        auto fields = splitString(line, ';');
        if (fields.size() < 4)
            continue;

        VehicleNameMapping mapping;
        mapping.wtName = trim(fields[0]);
        mapping.fmName = trim(fields[1]);
        mapping.type = trim(fields[2]);
        mapping.englishName = trim(fields[3]);

        vehicleMappings_.push_back(mapping);
        nameMapping_[mapping.wtName] = mapping.fmName;
    }

    return true;
}
