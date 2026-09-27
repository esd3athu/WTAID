#include "indicators/BuiltinIndicators.h"

#include <string>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <chrono>

// AltitudeIndicator
AltitudeIndicator::AltitudeIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float AltitudeIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                            const AircraftFMData* fmData) const
{
    return raw.altitudeM;
}

// TASIndicator
TASIndicator::TASIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float TASIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                       const AircraftFMData* fmData) const
{
    return raw.tasKmh;
}

// IASIndicator
IASIndicator::IASIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float IASIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                       const AircraftFMData* fmData) const
{
    return raw.iasKmh;
}

// MachIndicator
MachIndicator::MachIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float MachIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                        const AircraftFMData* fmData) const
{
    return raw.machNumber;
}

// AoAIndicator
AoAIndicator::AoAIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float AoAIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                       const AircraftFMData* fmData) const
{
    return raw.aoaDeg;
}

// GForceIndicator
GForceIndicator::GForceIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float GForceIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                          const AircraftFMData* fmData) const
{
    return raw.gForce;
}

// ClimbRateIndicator
ClimbRateIndicator::ClimbRateIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float ClimbRateIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                             const AircraftFMData* fmData) const
{
    return raw.climbRateMs;
}

// FuelIndicator
FuelIndicator::FuelIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float FuelIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                        const AircraftFMData* fmData) const
{
    return raw.fuelMassKg;
}

// FuelPercentIndicator
FuelPercentIndicator::FuelPercentIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float FuelPercentIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                               const AircraftFMData* fmData) const
{
    // War Thunder telemetry server всегда возвращает Mfuel0 = максимум ВС
    // с подвесными баками, независимо от текущей конфигурации.
    // Решение: отслеживаем максимальное значение Mfuel с момента загрузки
    // миссии (когда бак полон) и используем его как знаменатель.
    // Это значение равно максимуму внутреннего топлива для текущей
    // конфигурации (с учётом установленных подвесных баков).
    static float cachedMaxFuel = 0.0f;
    static std::string lastVehicleType;

    // Сбрасываем при смене транспортного средства или выходе с миссии
    // (fuel = 0 означает, что данные больше неактуальны)
    if (raw.fuelMassKg == 0.0f)
    {
        cachedMaxFuel = 0.0f;
        lastVehicleType.clear();
    }
    else if (lastVehicleType != raw.vehicleType)
    {
        cachedMaxFuel = 0.0f;
        lastVehicleType = raw.vehicleType;
    }

    if (raw.fuelMassKg > cachedMaxFuel)
    {
        cachedMaxFuel = raw.fuelMassKg;
    }

    if (cachedMaxFuel > 0.0f)
        return (raw.fuelMassKg / cachedMaxFuel) * 100.0f;

    // Fallback: если не удалось определить максимум
    if (raw.fuelMass0Kg > 0.0f)
        return (raw.fuelMassKg / raw.fuelMass0Kg) * 100.0f;

    return 0.0f;
}

// FuelPercentFMIndicator
FuelPercentFMIndicator::FuelPercentFMIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float FuelPercentFMIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                                 const AircraftFMData* fmData) const
{
    // Процент от максимального возможного топлива ВС (из FM-базы).
    // Это значение включает максимальные подвесные баки.
    if (fmData && fmData->maxFuelMass > 0.0f)
        return (raw.fuelMassKg / fmData->maxFuelMass) * 100.0f;

    return 0.0f;
}

// TimeRemainingIndicator
TimeRemainingIndicator::TimeRemainingIndicator(const IndicatorConfig& config)
    : Indicator(config), isApproximate_(false)
{
}

float TimeRemainingIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                                 const AircraftFMData* fmData) const
{
    // fuel_consume из /indicators — это кг/мин (не кг/с).
    // Переводим в кг/с и считаем время до исчерпания.
    if (raw.fuelConsumeKgS > 0.001f)
    {
        isApproximate_ = false;
        float consumeKgS = raw.fuelConsumeKgS / 60.0f;
        return raw.fuelMassKg / consumeKgS;
    }
    
    // Если fuel_consume недоступен (0), используем оценку по скорости убывания топлива
    // Запоминаем топливо, через секунду вычисляем разность как расход кг/с
    static float prevFuelMass = 0.0f;
    static bool hasPrev = false;
    static std::chrono::steady_clock::time_point prevTime;
    
    if (raw.fuelMassKg > 0.0f)
    {
        auto now = std::chrono::steady_clock::now();
        
        if (hasPrev)
        {
            float elapsed = std::chrono::duration<float>(now - prevTime).count();
            
            if (elapsed >= 1.0f)
            {
                float fuelDrop = prevFuelMass - raw.fuelMassKg;
                
                if (fuelDrop > 1.0f) // Минимальный перепад 1 кг для надёжности
                {
                    float avgConsumeKgS = fuelDrop / elapsed;
                    
                    if (avgConsumeKgS > 0.01f)
                    {
                        isApproximate_ = true;
                        return raw.fuelMassKg / avgConsumeKgS;
                    }
                }
                
                prevFuelMass = raw.fuelMassKg;
                prevTime = now;
            }
        }
        else
        {
            prevFuelMass = raw.fuelMassKg;
            prevTime = now;
            hasPrev = true;
        }
    }
    else
    {
        // Сброс при нулевом топливе
        hasPrev = false;
        prevFuelMass = 0.0f;
    }
    
    isApproximate_ = false;
    return 0.0f;
}

std::string TimeRemainingIndicator::formatValueFloat(float value) const
{
    if (value <= 0.0f)
        return "--:--";

    int totalSeconds = static_cast<int>(std::round(value));
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    char buf[32];
    if (isApproximate_)
        snprintf(buf, sizeof(buf), "~%d:%02d", minutes, seconds);
    else
        snprintf(buf, sizeof(buf), "%d:%02d", minutes, seconds);
    
    return std::string(buf);
}

// FlapsIndicator
FlapsIndicator::FlapsIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float FlapsIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                         const AircraftFMData* fmData) const
{
    return raw.flapsPercent;
}

// GearIndicator
GearIndicator::GearIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float GearIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                        const AircraftFMData* fmData) const
{
    return raw.gearPercent;
}

// RPMIndicator
RPMIndicator::RPMIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float RPMIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                       const AircraftFMData* fmData) const
{
    if (config_.stateKey.find("1") != std::string::npos)
        return raw.rpm1;
    return raw.rpm2;
}

// ThrustIndicator
ThrustIndicator::ThrustIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float ThrustIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                          const AircraftFMData* fmData) const
{
    if (config_.stateKey.find("1") != std::string::npos)
        return raw.thrust1Kgs;
    return raw.thrust2Kgs;
}

// AirbrakeIndicator
AirbrakeIndicator::AirbrakeIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float AirbrakeIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                            const AircraftFMData* fmData) const
{
    return raw.airbrakePercent;
}

// TASSpeedIndicator
TASSpeedIndicator::TASSpeedIndicator(const IndicatorConfig& config)
    : Indicator(config)
{
}

float TASSpeedIndicator::evaluateValueImpl(const TelemetryRawData& raw,
                                            const AircraftFMData* fmData) const
{
    return raw.tasKmh;
}
