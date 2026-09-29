#pragma once

#include <string>
#include <cmath>

/**
 * Структура сырых данных телеметрии из War Thunder.
 * Заполняется парсером из JSON, полученного с http://127.0.0.1:8111/state
 */
struct TelemetryRawData
{
    // Валидность данных
    bool valid = false;

    // Имя текущего ТС (из /indicators)
    std::string vehicleType;

    // Управление
    float aileronPercent = 0.0f;
    float elevatorPercent = 0.0f;
    float rudderPercent = 0.0f;
    float flapsPercent = 0.0f;
    float gearPercent = 0.0f;
    float airbrakePercent = 0.0f;

    // Полёт
    float altitudeM = 0.0f;           // H, m
    float tasKmh = 0.0f;             // TAS, km/h
    float iasKmh = 0.0f;             // IAS, km/h
    float machNumber = 0.0f;          // M
    float aoaDeg = 0.0f;             // AoA, deg
    float aosDeg = 0.0f;             // AoS, deg
    float gForce = 0.0f;             // Ny
    float climbRateMs = 0.0f;        // Vy, m/s
    float turnRateDegS = 0.0f;       // Wx, deg/s

    // Топливо
    float fuelMassKg = 0.0f;         // Mfuel, kg
    float fuelMass0Kg = 0.0f;       // Mfuel0, kg

    // Двигатель 1
    float throttle1Percent = 0.0f;
    float power1Hp = 0.0f;
    float rpm1 = 0.0f;
    float manifoldPressure1Atm = 0.0f;
    float oilTemp1C = 0.0f;
    float thrust1Kgs = 0.0f;
    float efficiency1Percent = 0.0f;

    // Двигатель 2
    float throttle2Percent = 0.0f;
    float power2Hp = 0.0f;
    float rpm2 = 0.0f;
    float manifoldPressure2Atm = 0.0f;
    float oilTemp2C = 0.0f;
    float thrust2Kgs = 0.0f;
    float efficiency2Percent = 0.0f;

    // Расход топлива (из /indicators)
    float fuelConsumeKgS = 0.0f;  // кг/с

    // Вычисляемые параметры
    float tasMs() const { return tasKmh / 3.6f; }
    float iasMs() const { return iasKmh / 3.6f; }
    float altitudeFt() const { return altitudeM * 3.28084f; }
    float tasKnots() const { return tasKmh * 0.539957f; }
    float gForceNormalized() const { return gForce / 9.81f; }
};

/**
 * Структура обработанных данных телеметрии.
 * Содержит сырые данные + вычисленные параметры.
 */
struct TelemetryProcessedData
{
    // Сырые данные
    TelemetryRawData raw;

    // Вычисленные параметры
    float machNumberComputed = 0.0f;
    float tasMsComputed = 0.0f;
    float iasMsComputed = 0.0f;
    float dragKg = 0.0f;
    float turnRadiusM = 0.0f;
    float turnRateDegSComputed = 0.0f;
    float glideRangeM = 0.0f;
    float thrustToWeight = 0.0f;
    float specificExcessPower = 0.0f;
    float aoaComputed = 0.0f;

    // Фильтрованные значения
    float altitudeFiltered = 0.0f;
    float tasFiltered = 0.0f;
    float iasFiltered = 0.0f;
    float gForceFiltered = 0.0f;
    float climbRateFiltered = 0.0f;
    float turnRateFiltered = 0.0f;
    float aoaFiltered = 0.0f;
    float fuelFiltered = 0.0f;

    // Флаги
    bool computed = false;
};
