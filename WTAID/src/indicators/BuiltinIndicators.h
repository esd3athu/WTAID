#pragma once

#include "indicators/Indicator.h"

/**
 * Индикатор высоты.
 */
class AltitudeIndicator : public Indicator
{
public:
    AltitudeIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор истинной воздушной скорости.
 */
class TASIndicator : public Indicator
{
public:
    TASIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор указательной воздушной скорости.
 */
class IASIndicator : public Indicator
{
public:
    IASIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор числа Маха.
 */
class MachIndicator : public Indicator
{
public:
    MachIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор угла атаки.
 */
class AoAIndicator : public Indicator
{
public:
    AoAIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор перегрузки.
 */
class GForceIndicator : public Indicator
{
public:
    GForceIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор скорости набора высоты.
 */
class ClimbRateIndicator : public Indicator
{
public:
    ClimbRateIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор топлива.
 */
class FuelIndicator : public Indicator
{
public:
    FuelIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор процента топлива (от текущего максимума с учётом подвесных баков).
 */
class FuelPercentIndicator : public Indicator
{
public:
    FuelPercentIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор процента топлива от максимального возможного (FM-база).
 * Показывает процент от maxFuelMass — максимум ВС с подвесными баками.
 */
class FuelPercentFMIndicator : public Indicator
{
public:
    FuelPercentFMIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор времени до окончания топлива.
 * Формат: "12:34" (минуты:секунды) или "~12:34" при приблизительном расчёте.
 */
class TimeRemainingIndicator : public Indicator
{
public:
    TimeRemainingIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;

    std::string formatValueFloat(float value) const override;

private:
    mutable bool isApproximate_ = false;
};

/**
 * Индикатор закрылков.
 */
class FlapsIndicator : public Indicator
{
public:
    FlapsIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор шасси.
 */
class GearIndicator : public Indicator
{
public:
    GearIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор RPM двигателя.
 */
class RPMIndicator : public Indicator
{
public:
    RPMIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор тяги двигателя.
 */
class ThrustIndicator : public Indicator
{
public:
    ThrustIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор тормозной панели.
 */
class AirbrakeIndicator : public Indicator
{
public:
    AirbrakeIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};

/**
 * Индикатор истинной воздушной скорости.
 */
class TASSpeedIndicator : public Indicator
{
public:
    TASSpeedIndicator(const IndicatorConfig& config);

protected:
    float evaluateValueImpl(const TelemetryRawData& raw,
                            const AircraftFMData* fmData) const override;
};
