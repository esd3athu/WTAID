#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

// ---------------------------------------------------------------------------
// Конфигурация индикатора (видимость, порядок)
// ---------------------------------------------------------------------------
struct IndicatorConfigEntry
{
    std::string name;           // имя индикатора
    bool visible = true;        // показывать в GUI
    int order = 0;              // порядок сортировки
};

// ---------------------------------------------------------------------------
// Конфигурация GUI
// ---------------------------------------------------------------------------
struct GUIConfig
{
    int width = 720;
    int height = 680;
    bool showVehicleHeader = true;
    bool showAlerts = true;
    int fontSize = 15;
    std::vector<IndicatorConfigEntry> indicators;
};

// ---------------------------------------------------------------------------
// Конфигурация алертов
// ---------------------------------------------------------------------------
struct AlertConfig
{
    std::string name;
    bool enabled = true;
    std::string color;
    float threshold = 0.0f;
};

// ---------------------------------------------------------------------------
// Видимость одного индикатора
// ---------------------------------------------------------------------------
struct IndicatorVisibility
{
    std::string name;           // имя индикатора
    bool visible = true;        // показывать в оверлее
    std::string section;        // секция (FLIGHT, FUEL, CONTROLS, ENGINE 1, ENGINE 2)
    
    // Смещения для оверлея
    int overlayOffsetX = 0;
    int overlayOffsetY = 0;
};

// ---------------------------------------------------------------------------
// Профиль — набор настроек для конкретного типа ВС
// ---------------------------------------------------------------------------
struct Profile
{
    std::string name;               // имя профиля
    std::string vehicleType;        // тип ВС (пусто = общий профиль)
    std::string vehicleTypeFilter;  // фильтр по имени ТС

    // Цветовая схема (RGB как int)
    int bgMain = 0x0F1216;
    int bgPanel = 0x191E26;
    int bgPanelBorder = 0x323C4B;
    int textNormal = 0xC8D2DC;
    int textLabel = 0x8C9BAA;
    int textValue = 0xFFFFFF;
    int textAlert = 0xFF4646;
    int barNormal = 0x3CA0FF;
    int barWarning = 0xFFB428;
    int barAlert = 0xFF3232;
    int barBg = 0x232A34;
    int headerText = 0x64C8FF;
    int alertBg = 0x501414;
    int alertText = 0xFF6464;

    // Настройки GUI
    GUIConfig guiConfig;

    // Алерты
    std::vector<AlertConfig> alerts;

    // Видимость индикаторов
    std::vector<IndicatorVisibility> indicatorVisibility;

    // Виден ли профиль
    bool isActive = true;
};

// ---------------------------------------------------------------------------
// ConfigManager — загрузка/сохранение конфигурации
// ---------------------------------------------------------------------------
class ConfigManager
{
public:
    ConfigManager();

    /**
     * Загрузить конфигурацию из файла.
     * @param path  Путь к JSON-файлу.
     * @return true если успешно.
     */
    bool load(const std::string& path);

    /**
     * Сохранить конфигурацию в файл.
     * @param path  Путь к JSON-файлу.
     * @return true если успешно.
     */
    bool save(const std::string& path);

    /**
     * Получить путь к конфигурации по умолчанию.
     */
    static std::string getDefaultPath();

    /**
     * Создать директорию если не существует.
     */
    static void ensureDirectoryExists(const std::string& path);

private:
    nlohmann::json root_;
    std::string configPath_;

    void initDefaultConfig();

    friend class ProfileManager;
};

// ---------------------------------------------------------------------------
// ProfileManager — управление профилями
// ---------------------------------------------------------------------------
class ProfileManager
{
public:
    ProfileManager();

    /**
     * Загрузить профили из файла.
     */
    bool load(const std::string& path);

    /**
     * Сохранить профили в файл.
     */
    bool save(const std::string& path);

    /**
     * Добавить профиль.
     */
    void addProfile(std::unique_ptr<Profile> profile);

    /**
     * Удалить профиль по имени.
     */
    bool removeProfile(const std::string& name);

    /**
     * Получить профиль по имени.
     */
    const Profile* getProfile(const std::string& name) const;

    /**
     * Получить активный профиль для текущего ТС.
     * Ищет профиль по vehicleType, если не найден — возвращает общий.
     */
    const Profile* getActiveProfile(const std::string& vehicleType) const;

    /**
     * Получить список всех профилей.
     */
    const std::vector<std::unique_ptr<Profile>>& getAllProfiles() const;

    /**
     * Установить активный профиль.
     */
    void setActiveProfile(const std::string& name);

    /**
     * Получить активный профиль.
     */
    const Profile* getActiveProfile() const;

    /**
     * Проверить, виден ли индикатор.
     */
    bool isIndicatorVisible(const std::string& indicatorName) const;

    /**
     * Установить видимость индикатора.
     */
    void setIndicatorVisible(const std::string& indicatorName, bool visible);

    /**
     * Создать профиль по умолчанию.
     */
    static std::unique_ptr<Profile> createDefaultProfile();

    /**
     * Получить список всех доступных индикаторов по умолчанию.
     */
    static std::vector<IndicatorVisibility> getDefaultIndicatorVisibility();

private:
    std::vector<std::unique_ptr<Profile>> profiles_;
    std::string activeProfileName_;

    const Profile* findProfileByType(const std::string& vehicleType) const;
};
