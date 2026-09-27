#include "data/DataParser.h"

#include <nlohmann/json.hpp>

std::optional<TelemetryRawData> DataParser::parse(const std::string& jsonString)
{
    TelemetryRawData data;

    try
    {
        nlohmann::json j = nlohmann::json::parse(jsonString);

        // Валидность
        data.valid = readBool(j, "valid", false);
        if (!data.valid)
            return data;

        // Управление
        data.aileronPercent = readFloat(j, "aileron, %");
        data.elevatorPercent = readFloat(j, "elevator, %");
        data.rudderPercent = readFloat(j, "rudder, %");
        data.flapsPercent = readFloat(j, "flaps, %");
        data.gearPercent = readFloat(j, "gear, %");
        data.airbrakePercent = readFloat(j, "airbrake, %");

        // Полёт
        data.altitudeM = readFloat(j, "H, m");
        data.tasKmh = readFloat(j, "TAS, km/h");
        data.iasKmh = readFloat(j, "IAS, km/h");
        data.machNumber = readFloat(j, "M");
        data.aoaDeg = readFloat(j, "AoA, deg");
        data.aosDeg = readFloat(j, "AoS, deg");
        data.gForce = readFloat(j, "Ny");
        data.climbRateMs = readFloat(j, "Vy, m/s");
        data.turnRateDegS = readFloat(j, "Wx, deg/s");

        // Топливо
        data.fuelMassKg = readFloat(j, "Mfuel, kg");
        data.fuelMass0Kg = readFloat(j, "Mfuel0, kg");

        // Двигатель 1
        data.throttle1Percent = readFloat(j, "throttle 1, %");
        data.power1Hp = readFloat(j, "power 1, hp");
        data.rpm1 = readFloat(j, "RPM 1");
        data.manifoldPressure1Atm = readFloat(j, "manifold pressure 1, atm");
        data.oilTemp1C = readFloat(j, "oil temp 1, C");
        data.thrust1Kgs = readFloat(j, "thrust 1, kgs");
        data.efficiency1Percent = readFloat(j, "efficiency 1, %");

        // Двигатель 2
        data.throttle2Percent = readFloat(j, "throttle 2, %");
        data.power2Hp = readFloat(j, "power 2, hp");
        data.rpm2 = readFloat(j, "RPM 2");
        data.manifoldPressure2Atm = readFloat(j, "manifold pressure 2, atm");
        data.oilTemp2C = readFloat(j, "oil temp 2, C");
        data.thrust2Kgs = readFloat(j, "thrust 2, kgs");
        data.efficiency2Percent = readFloat(j, "efficiency 2, %");

        // Расход топлива (есть в /state как "fuel_consume, kg/s")
        data.fuelConsumeKgS = readFloat(j, "fuel_consume, kg/s");
    }
    catch (const nlohmann::json::parse_error&)
    {
        return std::nullopt;
    }
    catch (...)
    {
        return std::nullopt;
    }

    return data;
}

float DataParser::readFloat(const nlohmann::json& j, const std::string& key, float defaultValue)
{
    try
    {
        if (j.contains(key))
        {
            return static_cast<float>(j[key].get<double>());
        }
    }
    catch (...)
    {
    }
    return defaultValue;
}

bool DataParser::readBool(const nlohmann::json& j, const std::string& key, bool defaultValue)
{
    try
    {
        if (j.contains(key))
        {
            return j[key].get<bool>();
        }
    }
    catch (...)
    {
    }
    return defaultValue;
}
