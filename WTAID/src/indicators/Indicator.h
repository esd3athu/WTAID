#pragma once

#include "data/TelemetryData.h"
#include "fmdb/FMDataModel.h"
#include <string>
#include <vector>
#include <functional>

/**
 * Тип индикатора.
 */
enum class IndicatorType
{
    NUMBER,       // числовое значение
    STRING,       // строковое значение
    LABEL,        // только метка
    LUA_CUSTOM    // пользовательский Lua-скрипт
};

/**
 * Правило алерта.
 */
struct AlertRule
{
    std::string name;               // имя алерта

    enum class Relation { EQUAL, LESS, GREATER, RANGE };
    Relation relation = Relation::GREATER;
    float thresholdA = 0.0f;
    float thresholdB = 0.0f;        // для RANGE (a < x < b)

    std::string colorName;          // имя цвета
    float blinkInterval = 0.0f;     // в интервалах обновления
    std::string soundFile;          // путь к звуку
    bool repeat = true;
    int repeatInterval = 0;
    int priority = 0;               // LOW=0, NORMAL=1, HIGH=2
    std::vector<std::string> dependencies;
    bool multiDepAnd = true;        // AND или OR для зависимостей
};

/**
 * Конфигурация индикатора.
 */
struct IndicatorConfig
{
    std::string name;               // имя индикатора
    std::string stateKey;           // ключ данных (например "altitude, m")
    std::string stateKeyFmt;        // формат ключа (%d для мультизначений)
    IndicatorType type = IndicatorType::NUMBER;
    std::string units;              // единицы измерения
    float valueMultiplier = 1.0f;   // множитель значения
    int precision = 1;              // точность
    std::string luaScriptPath;      // путь к Lua-скрипту
    std::vector<AlertRule> alerts;  // правила алертов
    std::string groupName;          // имя группы
    bool showInOSD = true;          // показывать в оверлее
    bool hideIfUnavailable = false; // скрывать если недоступно

    IndicatorConfig()
        : type(IndicatorType::NUMBER)
        , valueMultiplier(1.0f)
        , precision(1)
        , showInOSD(true)
        , hideIfUnavailable(false)
    {}

    IndicatorConfig(const std::string& n, const std::string& key,
                    const std::string& keyFmt, IndicatorType t,
                    const std::string& u, float mult, int prec)
        : name(n), stateKey(key), stateKeyFmt(keyFmt), type(t),
          units(u), valueMultiplier(mult), precision(prec),
          showInOSD(true), hideIfUnavailable(false)
    {}
};

/**
 * Результат оценки индикатора.
 */
struct IndicatorResult
{
    std::string displayName;        // отображаемое имя
    std::string formattedValue;     // отформатированное значение
    std::string units;              // единицы измерения
    bool available = true;          // доступен ли индикатор
    bool alertTriggered = false;    // сработал ли алерт
    std::string alertName;          // имя сработавшего алерта
    std::string colorName;          // имя цвета
};

/**
 * Базовый класс индикатора.
 */
class Indicator
{
public:
    Indicator(const IndicatorConfig& config);
    virtual ~Indicator();

    /**
     * Инициализировать индикатор.
     */
    virtual void init();

    /**
     * Оценить значение индикатора на основе телеметрии.
     * @param raw  Сырые данные телеметрии
     * @param fmData  Данные FM-базы (может быть nullptr)
     * @return Результат оценки
     */
    virtual IndicatorResult evaluate(const TelemetryRawData& raw,
                                     const AircraftFMData* fmData) const;

    /**
     * Оценить числовое значение.
     * @param raw  Сырые данные телеметрии
     * @param fmData  Данные FM-базы (может быть nullptr)
     * @return Вычисленное значение
     */
    virtual float evaluateValue(const TelemetryRawData& raw,
                                 const AircraftFMData* fmData) const;

    /**
     * Отформатировать значение.
     * @param value  Числовое значение
     * @return Отформатированная строка
     */
    std::string formatValue(float value) const;

    /**
     * Отформатировать строковое значение.
     * @param str  Строковое значение
     * @return Отформатированная строка
     */
    std::string formatValueStr(const std::string& str) const;

    /**
     * Отформатировать числовое значение (виртуальный для переопределения).
     * @param value  Числовое значение
     * @return Отформатированная строка
     */
    virtual std::string formatValueFloat(float value) const;

    /**
     * Проверить алерты.
     * @param value  Текущее значение
     * @return true если алерт сработал
     */
    bool checkAlerts(float value) const;

    /**
     * Получить конфигурацию.
     */
    const IndicatorConfig& getConfig() const;

    /**
     * Получить список алертов.
     */
    const std::vector<AlertRule>& getAlerts() const;

    /**
     * Установить критическую перегрузку из FM-базы.
     */
    void setCriticalG(float g);

    /**
     * Установить критическую скорость из FM-базы.
     */
    void setCriticalAirSpeed(float speedKmh);

protected:
    IndicatorConfig config_;
    mutable float cachedValue_ = 0.0f;
    mutable std::string cachedStr_;
    bool isAvailable_ = true;
    float criticalG_ = 9.0f;  // критическая перегрузка из FM-базы
    float critAirSpd_ = 0.0f; // критическая скорость из FM-базы

private:
    // Виртуальные методы для переопределения
    virtual float evaluateValueImpl(const TelemetryRawData& raw,
                                     const AircraftFMData* fmData) const = 0;
};
