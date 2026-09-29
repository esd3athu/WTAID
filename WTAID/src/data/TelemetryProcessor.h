#pragma once

#include "data/TelemetryData.h"
#include <vector>

/**
 * Обработчик телеметрии.
 * Вычисляет производные параметры, применяет фильтры.
 */
class TelemetryProcessor
{
public:
    TelemetryProcessor();

    /**
     * Обработать сырые данные телеметрии.
     * @param raw  Сырые данные из JSON
     * @return Обработанная структура с вычисляемыми параметрами
     */
    TelemetryProcessedData process(const TelemetryRawData& raw);

    /**
     * Применить EMA-фильтр к значению.
     * @param newValue      Новое значение
     * @param prevValue     Предыдущее отфильтрованное значение
     * @param alpha         Коэффициент сглаживания (0.0 - 1.0)
     * @return Отфильтрованное значение
     */
    static float applyEMA(float newValue, float prevValue, float alpha = 0.3f);

    /**
     * Получить последнее отфильтрованное значение.
     */
    float getLastFilteredValue() const;

    /**
     * Сбросить фильтры.
     */
    void resetFilters();

private:
    // Вычисляемые параметры
    float calculateMachNumber(float tasMs, float temperatureK) const;
    float calculateTASFromIAS(float iasMs, float altitudeM) const;
    float calculateDrag(float massKg, float speedMs, float wingAreaM2) const;
    float calculateTurnRadius(float speedMs, float turnRateDegS) const;
    float calculateTurnRate(float speedMs, float bankAngleDeg) const;
    float calculateGlideRange(float altitudeM, float sinkRateMs) const;
    float calculateThrustToWeight(float thrustKgs, float massKg) const;
    float calculateSpecificExcessPower(float thrustKgs, float dragKgs,
                                        float speedMs, float weightN) const;
    float calculateAoAFromAoS(float aosDeg, float pitchDeg) const;

    // EMA-фильтры
    float emaAltitude_;
    float emaTAS_;
    float emaIAS_;
    float emaGForce_;
    float emaClimbRate_;
    float emaTurnRate_;
    float emaAoA_;
    float emaFuel_;

    // Параметры по умолчанию для вычислений
    static constexpr float SEA_LEVEL_TEMPERATURE_K = 288.15f;
    static constexpr float AIR_DENSITY_KGM3 = 1.225f;
    static constexpr float WING_AREA_M2 = 40.0f;
    static constexpr float EMPTY_MASS_KG = 10000.0f;
};
