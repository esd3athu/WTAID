#pragma once

#include "indicators/Indicator.h"
#include <string>
#include <vector>
#include <functional>

/**
 * Система алертов.
 * Управляет предупреждениями на основе индикаторов.
 */
class AlertSystem
{
public:
    using AlertCallback = std::function<void(const std::string& alertName,
                                              float value,
                                              const std::string& color)>;

    AlertSystem();

    /**
     * Установить колбэк для обработки алертов.
     */
    void onAlert(AlertCallback callback);

    /**
     * Установить критическую перегрузку из FM-базы.
     * @param critG  Критическая перегрузка (g)
     */
    void setCriticalG(float critG);

    /**
     * Проверить индикатор на алерты.
     * @param indicator  Индикатор для проверки
     * @param value  Текущее значение
     * @return true если алерт сработал
     */
    bool checkIndicator(const Indicator* indicator, float value);

    /**
     * Проверить все индикаторы.
     * @param results  Результаты оценки индикаторов
     */
    void checkAll(const std::vector<IndicatorResult>& results);

    /**
     * Получить список активных алертов.
     */
    std::vector<std::string> getActiveAlerts() const;

    /**
     * Сбросить все алерты.
     */
    void reset();

    /**
     * Добавить правило алерта.
     */
    void addAlertRule(const std::string& indicatorName, const AlertRule& rule);

private:
    AlertCallback alertCallback_;
    std::vector<std::string> activeAlerts_;
    std::unordered_map<std::string, std::vector<AlertRule>> alertRules_;
    float criticalG_ = 9.0f;  // критическая перегрузка по умолчанию

    void processAlert(const std::string& indicatorName,
                      const AlertRule& rule,
                      float value,
                      const std::string& color);
};
