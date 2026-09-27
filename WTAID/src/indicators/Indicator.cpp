#include "indicators/Indicator.h"

#include <sstream>
#include <iomanip>
#include <cmath>

Indicator::Indicator(const IndicatorConfig& config)
    : config_(config)
    , cachedValue_(0.0f)
    , cachedStr_("")
    , isAvailable_(true)
    , criticalG_(9.0f)
{
}

Indicator::~Indicator()
{
}

void Indicator::init()
{
    isAvailable_ = true;
}

IndicatorResult Indicator::evaluate(const TelemetryRawData& raw,
                                     const AircraftFMData* fmData) const
{
    IndicatorResult result;
    result.displayName = config_.name;
    result.units = config_.units;
    result.available = isAvailable_ && raw.valid;

    if (!result.available)
    {
        if (config_.hideIfUnavailable)
            result.available = false;
        return result;
    }

    // Вычисляем значение
    cachedValue_ = evaluateValue(raw, fmData);
    cachedStr_ = formatValueFloat(cachedValue_);

    // Форматируем результат
    result.formattedValue = cachedStr_;

    // Проверяем алерты
    result.alertTriggered = checkAlerts(cachedValue_);
    if (result.alertTriggered)
    {
        // Находим первый сработавший алерт
        for (const auto& alert : config_.alerts)
        {
            bool triggered = false;
            float threshold = alert.thresholdA;

            // Для алерта "High G" используем динамический порог из FM-базы
            if (alert.name == "High G" && criticalG_ > 0.0f)
            {
                threshold = criticalG_ * 0.95f;  // 95% от критического
            }

            // Для алерта "High Speed" используем динамический порог из FM-базы
            if (alert.name == "High Speed" && critAirSpd_ > 0.0f)
            {
                threshold = critAirSpd_ * 0.95f;  // 95% от критической скорости
            }

            switch (alert.relation)
            {
            case AlertRule::Relation::GREATER:
                triggered = cachedValue_ > threshold;
                break;
            case AlertRule::Relation::LESS:
                triggered = cachedValue_ < threshold;
                break;
            case AlertRule::Relation::EQUAL:
                triggered = std::abs(cachedValue_ - threshold) < 0.01f;
                break;
            case AlertRule::Relation::RANGE:
                triggered = cachedValue_ > alert.thresholdA &&
                           cachedValue_ < alert.thresholdB;
                break;
            }

            if (triggered)
            {
                result.alertName = alert.name;
                result.colorName = alert.colorName;
                break;
            }
        }
    }

    return result;
}

float Indicator::evaluateValue(const TelemetryRawData& raw,
                                const AircraftFMData* fmData) const
{
    float value = evaluateValueImpl(raw, fmData);
    return value * config_.valueMultiplier;
}

std::string Indicator::formatValue(float value) const
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(config_.precision) << value;
    return oss.str();
}

std::string Indicator::formatValueStr(const std::string& str) const
{
    return str;
}

std::string Indicator::formatValueFloat(float value) const
{
    return formatValue(value);
}

bool Indicator::checkAlerts(float value) const
{
    for (const auto& alert : config_.alerts)
    {
        bool triggered = false;
        float threshold = alert.thresholdA;

        // Для алерта "High G" используем динамический порог из FM-базы
        if (alert.name == "High G" && criticalG_ > 0.0f)
        {
            threshold = criticalG_ * 0.95f;  // 95% от критического
        }

        // Для алерта "High Speed" используем динамический порог из FM-базы
        if (alert.name == "High Speed" && critAirSpd_ > 0.0f)
        {
            threshold = critAirSpd_ * 0.95f;  // 95% от критической скорости
        }

        switch (alert.relation)
        {
        case AlertRule::Relation::GREATER:
            triggered = value > threshold;
            break;
        case AlertRule::Relation::LESS:
            triggered = value < threshold;
            break;
        case AlertRule::Relation::EQUAL:
            triggered = std::abs(value - threshold) < 0.01f;
            break;
        case AlertRule::Relation::RANGE:
            triggered = value > alert.thresholdA &&
                       value < alert.thresholdB;
            break;
        }

        if (triggered)
            return true;
    }
    return false;
}

const IndicatorConfig& Indicator::getConfig() const
{
    return config_;
}

const std::vector<AlertRule>& Indicator::getAlerts() const
{
    return config_.alerts;
}

void Indicator::setCriticalG(float g)
{
    criticalG_ = g;
}

void Indicator::setCriticalAirSpeed(float speedKmh)
{
    critAirSpd_ = speedKmh;
}
