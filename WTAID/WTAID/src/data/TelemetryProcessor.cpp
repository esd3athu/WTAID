#include "data/TelemetryProcessor.h"

#include <algorithm>
#include <cmath>

TelemetryProcessor::TelemetryProcessor()
    : emaAltitude_(0.0f)
    , emaTAS_(0.0f)
    , emaIAS_(0.0f)
    , emaGForce_(0.0f)
    , emaClimbRate_(0.0f)
    , emaTurnRate_(0.0f)
    , emaAoA_(0.0f)
    , emaFuel_(0.0f)
{
}

TelemetryProcessedData TelemetryProcessor::process(const TelemetryRawData& raw)
{
    TelemetryProcessedData processed;
    processed.raw = raw;

    if (!raw.valid)
        return processed;

    // Вычисляем параметры
    float tasMs = raw.tasKmh / 3.6f;
    float iasMs = raw.iasKmh / 3.6f;

    // Mach number (упрощённый расчёт)
    processed.machNumberComputed = raw.machNumber;

    // TAS и IAS в м/с
    processed.tasMsComputed = tasMs;
    processed.iasMsComputed = iasMs;

    // Drag (сопротивление воздуха)
    processed.dragKg = calculateDrag(raw.fuelMassKg + EMPTY_MASS_KG,
                                      tasMs, WING_AREA_M2);

    // Turn radius
    if (raw.turnRateDegS > 0.1f)
        processed.turnRadiusM = calculateTurnRadius(tasMs, raw.turnRateDegS);

    // Turn rate from bank angle (если доступен)
    processed.turnRateDegSComputed = raw.turnRateDegS;

    // Glide range
    if (raw.climbRateMs < -1.0f)
        processed.glideRangeM = calculateGlideRange(raw.altitudeM, raw.climbRateMs);

    // Thrust to Weight
    float totalThrust = raw.thrust1Kgs + raw.thrust2Kgs;
    float totalMass = raw.fuelMassKg + EMPTY_MASS_KG;
    processed.thrustToWeight = calculateThrustToWeight(totalThrust, totalMass);

    // Specific Excess Power
    float totalWeight = totalMass * 9.81f;
    processed.specificExcessPower = calculateSpecificExcessPower(
        totalThrust, processed.dragKg, tasMs, totalWeight);

    // AoA computed
    processed.aoaComputed = raw.aoaDeg;

    // Применяем EMA-фильтры
    float alpha = 0.3f;

    processed.altitudeFiltered = applyEMA(raw.altitudeM, emaAltitude_, alpha);
    emaAltitude_ = processed.altitudeFiltered;

    processed.tasFiltered = applyEMA(tasMs, emaTAS_, alpha);
    emaTAS_ = processed.tasFiltered;

    processed.iasFiltered = applyEMA(iasMs, emaIAS_, alpha);
    emaIAS_ = processed.iasFiltered;

    processed.gForceFiltered = applyEMA(raw.gForce, emaGForce_, alpha);
    emaGForce_ = processed.gForceFiltered;

    processed.climbRateFiltered = applyEMA(raw.climbRateMs, emaClimbRate_, alpha);
    emaClimbRate_ = processed.climbRateFiltered;

    processed.turnRateFiltered = applyEMA(raw.turnRateDegS, emaTurnRate_, alpha);
    emaTurnRate_ = processed.turnRateFiltered;

    processed.aoaFiltered = applyEMA(raw.aoaDeg, emaAoA_, alpha);
    emaAoA_ = processed.aoaFiltered;

    processed.fuelFiltered = applyEMA(raw.fuelMassKg, emaFuel_, 0.1f);
    emaFuel_ = processed.fuelFiltered;

    processed.computed = true;
    return processed;
}

float TelemetryProcessor::applyEMA(float newValue, float prevValue, float alpha)
{
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    return alpha * newValue + (1.0f - alpha) * prevValue;
}

float TelemetryProcessor::getLastFilteredValue() const
{
    return emaAltitude_;
}

void TelemetryProcessor::resetFilters()
{
    emaAltitude_ = 0.0f;
    emaTAS_ = 0.0f;
    emaIAS_ = 0.0f;
    emaGForce_ = 0.0f;
    emaClimbRate_ = 0.0f;
    emaTurnRate_ = 0.0f;
    emaAoA_ = 0.0f;
    emaFuel_ = 0.0f;
}

float TelemetryProcessor::calculateMachNumber(float tasMs, float temperatureK) const
{
    // Упрощённая формула: M = TAS / a, где a = sqrt(gamma * R * T)
    const float gamma = 1.4f;
    const float R = 287.05f;
    float speedOfSound = std::sqrt(gamma * R * temperatureK);
    return tasMs / speedOfSound;
}

float TelemetryProcessor::calculateTASFromIAS(float iasMs, float altitudeM) const
{
    // Упрощённая формула с поправкой на сжимаемость
    float densityRatio = std::exp(-altitudeM / 7640.0f);
    return iasMs * std::sqrt(1.0f / std::max(densityRatio, 0.001f));
}

float TelemetryProcessor::calculateDrag(float massKg, float speedMs, float wingAreaM2) const
{
    // Упрощённая формула сопротивления: D = 0.5 * rho * V^2 * S * Cd
    // Cd = Cd0 + Cl^2 / (pi * e * AR)
    // Упрощённо: Cd ~ 0.02 - 0.05 для самолётов
    const float Cd = 0.03f;
    const float rho = AIR_DENSITY_KGM3;
    return 0.5f * rho * speedMs * speedMs * wingAreaM2 * Cd / 9.81f;
}

float TelemetryProcessor::calculateTurnRadius(float speedMs, float turnRateDegS) const
{
    // R = V / (omega * pi / 180)
    float omegaRad = turnRateDegS * 3.14159f / 180.0f;
    if (omegaRad < 0.01f) return 0.0f;
    return speedMs / omegaRad;
}

float TelemetryProcessor::calculateTurnRate(float speedMs, float bankAngleDeg) const
{
    // omega = g * tan(bank) / V
    const float g = 9.81f;
    float bankRad = bankAngleDeg * 3.14159f / 180.0f;
    return g * std::tan(bankRad) / std::max(speedMs, 1.0f) * 180.0f / 3.14159f;
}

float TelemetryProcessor::calculateGlideRange(float altitudeM, float sinkRateMs) const
{
    // R = V_horizontal / |V_vertical| * altitude
    // Упрощённо: R = altitude / |sinkRate| * horizontalSpeed
    if (sinkRateMs > -0.1f) return 0.0f;
    return altitudeM * 2.0f / std::abs(sinkRateMs);
}

float TelemetryProcessor::calculateThrustToWeight(float thrustKgs, float massKg) const
{
    if (massKg < 1.0f) return 0.0f;
    return thrustKgs / massKg;
}

float TelemetryProcessor::calculateSpecificExcessPower(float thrustKgs, float dragKgs,
                                                        float speedMs, float weightN) const
{
    // SEP = (T - D) * V / W
    float netForce = thrustKgs - dragKgs;
    return netForce * speedMs / std::max(weightN, 1.0f);
}

float TelemetryProcessor::calculateAoAFromAoS(float aosDeg, float pitchDeg) const
{
    // AoA = AoS + alpha_correction
    return aosDeg;
}
