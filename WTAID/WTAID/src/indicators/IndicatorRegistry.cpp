#include "indicators/IndicatorRegistry.h"

IndicatorRegistry::IndicatorRegistry()
{
}

void IndicatorRegistry::registerFactory(const std::string& type,
                                         IndicatorFactory factory)
{
    factories_[type] = std::move(factory);
}

std::unique_ptr<Indicator> IndicatorRegistry::createIndicator(
    const IndicatorConfig& config) const
{
    auto it = factories_.find(config.name);
    if (it != factories_.end())
    {
        return it->second(config);
    }

    // Если фабрика не найдена, создаём базовый индикатор
    return nullptr;
}

void IndicatorRegistry::addIndicator(std::unique_ptr<Indicator> indicator)
{
    if (indicator)
    {
        indicators_.push_back(std::move(indicator));
    }
}

const Indicator* IndicatorRegistry::getIndicator(const std::string& name) const
{
    for (const auto& indicator : indicators_)
    {
        if (indicator->getConfig().name == name)
            return indicator.get();
    }
    return nullptr;
}

std::vector<IndicatorResult> IndicatorRegistry::evaluateAll(
    const TelemetryRawData& raw,
    const AircraftFMData* fmData) const
{
    std::vector<IndicatorResult> results;
    results.reserve(indicators_.size());

    for (const auto& indicator : indicators_)
    {
        const auto& config = indicator->getConfig();

        // Скрываем индикаторы двигателей, если их нет на этом ТС
        if (config.name.find("RPM") != std::string::npos ||
            config.name.find("Thrust") != std::string::npos)
        {
            // Извлекаем номер двигателя из имени (RPM, RPM2, Thrust, Thrust2)
            int engineNum = 1;
            if (config.name.size() > 3 && config.name[3] >= '1' && config.name[3] <= '9')
            {
                engineNum = config.name[3] - '0';
            }
            else if (config.name.size() > 6 && config.name[6] >= '1' && config.name[6] <= '9')
            {
                engineNum = config.name[6] - '0';
            }

            if (engineNum > engineCount_)
            {
                // Скрываем этот индикатор
                IndicatorResult emptyResult;
                emptyResult.displayName = config.name;
                emptyResult.available = false;
                results.push_back(emptyResult);
                continue;
            }
        }

        results.push_back(indicator->evaluate(raw, fmData));
    }

    return results;
}

const std::vector<std::unique_ptr<Indicator>>& IndicatorRegistry::getAllIndicators() const
{
    return indicators_;
}

size_t IndicatorRegistry::getIndicatorCount() const
{
    return indicators_.size();
}

void IndicatorRegistry::clear()
{
    indicators_.clear();
}

void IndicatorRegistry::setCriticalG(float critG)
{
    for (auto& indicator : indicators_)
    {
        indicator->setCriticalG(critG);
    }
}

void IndicatorRegistry::setCriticalAirSpeed(float critSpeed)
{
    for (auto& indicator : indicators_)
    {
        indicator->setCriticalAirSpeed(critSpeed);
    }
}

void IndicatorRegistry::setEngineCount(int engineCount)
{
    engineCount_ = engineCount;
}
