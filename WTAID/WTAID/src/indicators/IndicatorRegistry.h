#pragma once

#include "indicators/Indicator.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

/**
 * Реестр индикаторов.
 * Управляет созданием, хранением и оценкой индикаторов.
 */
class IndicatorRegistry
{
public:
    using IndicatorFactory = std::function<std::unique_ptr<Indicator>(const IndicatorConfig&)>;

    IndicatorRegistry();

    /**
     * Зарегистрировать фабрику индикатора.
     * @param type  Имя типа индикатора
     * @param factory  Фабрика для создания индикаторов
     */
    void registerFactory(const std::string& type, IndicatorFactory factory);

    /**
     * Создать индикатор по конфигурации.
     * @param config  Конфигурация индикатора
     * @return Умный указатель на индикатор
     */
    std::unique_ptr<Indicator> createIndicator(const IndicatorConfig& config) const;

    /**
     * Добавить индикатор в реестр.
     */
    void addIndicator(std::unique_ptr<Indicator> indicator);

    /**
     * Получить индикатор по имени.
     */
    const Indicator* getIndicator(const std::string& name) const;

    /**
     * Оценить все индикаторы.
     * @param raw  Сырые данные телеметрии
     * @param fmData  Данные FM-базы (может быть nullptr)
     * @return Список результатов
     */
    std::vector<IndicatorResult> evaluateAll(const TelemetryRawData& raw,
                                              const AircraftFMData* fmData) const;

    /**
     * Установить количество двигателей для скрытия ненужных индикаторов.
     * @param engineCount  Количество двигателей (1, 2, 3, 4...)
     */
    void setEngineCount(int engineCount);

    /**
     * Установить критическую перегрузку для всех индикаторов.
     * @param critG  Критическая перегрузка (g)
     */
    void setCriticalG(float critG);

    /**
     * Установить критическую скорость для всех индикаторов.
     * @param critSpeed  Критическая скорость (км/ч)
     */
    void setCriticalAirSpeed(float critSpeed);

    /**
     * Получить все индикаторы.
     */
    const std::vector<std::unique_ptr<Indicator>>& getAllIndicators() const;

    /**
     * Получить количество индикаторов.
     */
    size_t getIndicatorCount() const;

    /**
     * Очистить реестр.
     */
    void clear();

private:
    std::unordered_map<std::string, IndicatorFactory> factories_;
    std::vector<std::unique_ptr<Indicator>> indicators_;
    int engineCount_ = 1;  // количество двигателей
};
