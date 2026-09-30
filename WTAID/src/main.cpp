#include "network/HTTPClient.h"
#include "data/DataParser.h"
#include "data/IndicatorsData.h"
#include "data/TelemetryProcessor.h"
#include "fmdb/FMDatabase.h"
#include "indicators/IndicatorRegistry.h"
#include "indicators/AlertSystem.h"
#include "indicators/BuiltinIndicators.h"
#include "ui/GUI.h"
#include <vector>
#include <unordered_map>
#include "config/ConfigManager.h"
#include "ui/ProfileDialog.h"
#include "ui/IndicatorSelectorDialog.h"
#include <fstream>
#include <sstream>

#include <iostream>
#include <string>
#include <unordered_map>
#include <windows.h>

// ---------------------------------------------------------------------------
// Вспомогательные функции для построения GUI-кадра
// ---------------------------------------------------------------------------
static float estimateMaxForIndicator(const std::string& name,
                                      const AircraftFMData* fmData)
{
    if (!fmData)
    {
        // Значения по умолчанию
        if (name == "Altitude") return 10000.0f;
        if (name == "TAS" || name == "IAS") return 900.0f;
        if (name == "Mach") return 1.5f;
        if (name == "AoA") return 20.0f;
        if (name == "GForce") return 12.0f;
        if (name == "ClimbRate") return 100.0f;
        if (name == "Fuel") return 5000.0f;
        if (name == "FuelPercent") return 100.0f;
        if (name == "Flaps" || name == "Gear" || name == "Airbrake")
            return 100.0f;
        if (name.find("RPM") != std::string::npos) return 3000.0f;
        if (name.find("Thrust") != std::string::npos) return 1000.0f;
        return 100.0f;
    }

    if (name == "Altitude") return 10000.0f;
    if (name == "TAS" || name == "IAS") return fmData->critAirSpd * 1.1f;
    if (name == "Mach") return 1.5f;
    if (name == "AoA") return 20.0f;
    if (name == "GForce") return fmData->critWingOverload * 1.2f;
    if (name == "ClimbRate") return 100.0f;
    if (name == "Fuel")
    {
        return fmData && fmData->maxFuelMass > 0 ? fmData->maxFuelMass : 5000.0f;
    }
    if (name == "FuelPercent") return 100.0f;
    if (name == "FuelPercentFM") return 100.0f;
    if (name == "Flaps" || name == "Gear" || name == "Airbrake")
        return 100.0f;
    if (name.find("RPM") != std::string::npos) return 3000.0f;
    if (name.find("Thrust") != std::string::npos) return 1000.0f;
    return 100.0f;
}

static void buildGUIFrame(GUIFrame& frame,
                           const std::vector<IndicatorResult>& results,
                           const std::vector<std::string>& activeAlerts,
                           const std::string& vehicleType,
                           ProfileManager* profileManager)
{
    frame.vehicleType = vehicleType;
    frame.activeAlerts = activeAlerts;
    frame.valid = true;

    // Получаем смещения из профиля
    std::unordered_map<std::string, std::pair<int, int>> overlayOffsets;
    if (profileManager)
    {
        const Profile* profile = profileManager->getActiveProfile(vehicleType);
        if (!profile)
            profile = profileManager->getActiveProfile();
        
        if (profile)
        {
            for (const auto& iv : profile->indicatorVisibility)
            {
                overlayOffsets[iv.name] = {iv.overlayOffsetX, iv.overlayOffsetY};
            }
        }
    }

    // Группируем результаты по groupName из конфигурации
    // Поскольку IndicatorResult не содержит groupName, используем имя индикатора
    std::unordered_map<std::string, std::vector<const IndicatorResult*>> grouped;

    for (const auto& result : results)
    {
        if (!result.available)
            continue;

        // Проверяем, виден ли индикатор
        if (profileManager && !profileManager->isIndicatorVisible(result.displayName))
        {
            continue;
        }

        // Определяем секцию по имени
        std::string section;
        if (result.displayName == "Altitude" ||
            result.displayName == "TAS" ||
            result.displayName == "IAS" ||
            result.displayName == "Mach" ||
            result.displayName == "AoA" ||
            result.displayName == "GForce" ||
            result.displayName == "ClimbRate")
        {
            section = "FLIGHT";
        }
        else if (result.displayName == "Fuel" ||
                 result.displayName == "FuelPercent" ||
                 result.displayName == "FuelPercentFM" ||
                 result.displayName == "TimeRemaining")
        {
            section = "FUEL";
        }
        else if (result.displayName == "Flaps" ||
                 result.displayName == "Gear" ||
                 result.displayName == "Airbrake")
        {
            section = "CONTROLS";
        }
        else if (result.displayName == "RPM" ||
                 result.displayName == "Thrust")
        {
            section = "ENGINE 1";
        }
        else if (result.displayName == "RPM2" ||
                 result.displayName == "Thrust2")
        {
            section = "ENGINE 2";
        }
        else
        {
            section = "OTHER";
        }

        grouped[section].push_back(&result);
    }

    // Строим секции
    // Определяем порядок секций
    std::vector<std::string> sectionOrder = {
        "FLIGHT", "FUEL", "CONTROLS", "ENGINE 1", "ENGINE 2", "OTHER"
    };

    for (const auto& secName : sectionOrder)
    {
        auto it = grouped.find(secName);
        if (it == grouped.end() || it->second.empty())
            continue;

        IndicatorSection section;
        section.title = secName;

        for (const auto* result : it->second)
        {
            IndicatorSlot slot;
            slot.label = result->displayName + ":";
            slot.value = result->formattedValue + " " + result->units;
            slot.alert = result->alertTriggered;

            // Восстанавливаем смещения из профиля
            auto it2 = overlayOffsets.find(result->displayName);
            if (it2 != overlayOffsets.end())
            {
                slot.offsetX = it2->second.first;
                slot.offsetY = it2->second.second;
            }

            // Для процентных значений используем fraction
            if (result->displayName == "Flaps" ||
                result->displayName == "Gear" ||
                result->displayName == "Airbrake" ||
                result->displayName == "FuelPercent" ||
                result->displayName == "FuelPercentFM")
            {
                // Извлекаем числовое значение из formattedValue
                float numVal = 0.0f;
                sscanf_s(result->formattedValue.c_str(), "%f", &numVal);
                slot.fraction = numVal / 100.0f;
                slot.maxVal = 100.0f;
            }
            else
            {
                // Для других индикаторов fraction = 0 (показываем только текст)
                slot.fraction = 0.0f;
            }

            section.slots.push_back(slot);
        }

        frame.sections.push_back(section);
    }
}

// ---------------------------------------------------------------------------
// WinMain - Windows-приложение без консоли
// ---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Устанавливаем UTF-8 для консоли (не нужно для Windows-приложения, но оставляем на всякий случай)
    // SetConsoleOutputCP(CP_UTF8);
    // SetConsoleCP(CP_UTF8);

    // std::cout << "WTAID - War Thunder Telemetry HUD" << std::endl;
    // std::cout << "---------------------------------" << std::endl;
    // std::cout << "Connecting to War Thunder telemetry (127.0.0.1:8111)..." << std::endl;
    // std::cout << std::endl;

    // Загружаем FM-базу данных
    FMDatabase fmDatabase;
    if (fmDatabase.load("FM/fm_data_db.csv", "FM/fm_names_db.csv"))
    {
        // std::cout << "FM Database loaded successfully." << std::endl;
        // std::cout << "Version: " << fmDatabase.getVersion() << std::endl;
        // std::cout << "Aircraft count: " << fmDatabase.getAllNames().size() << std::endl;
        // std::cout << std::endl;
    }
    else
    {
        // std::cout << "WARNING: FM Database failed to load." << std::endl;
        // std::cout << std::endl;
    }

    // Загружаем конфигурацию и профили
    ConfigManager configManager;
    ProfileManager profileManager;

    std::string configPath = ConfigManager::getDefaultPath();
    ConfigManager::ensureDirectoryExists(configPath);

    if (configManager.load(configPath))
    {
        // std::cout << "Configuration loaded from: " << configPath << std::endl;
        profileManager.load(configPath);
        // std::cout << "Profiles loaded: " << profileManager.getAllProfiles().size() << std::endl;
    }
    else
    {
        // std::cout << "No configuration found, using defaults." << std::endl;
    }

    // Создаём GUI-окно
    GUI gui;
    if (!gui.create(720, 680))
    {
        // std::cerr << "Error: Failed to create GUI window." << std::endl;
        return 1;
    }

    // Загружаем смещения оверлея в GUI из профиля
    gui.initOverlayOffsetCache(profileManager);

    // Устанавливаем ProfileManager в GUI
    gui.setProfileManager(&profileManager);

    // Устанавливаем колбэк для сохранения конфигурации
    gui.setSaveCallback([&profileManager, configPath](const ColorScheme& savedScheme) {
        // Сохраняем в активный профиль
        auto* active = const_cast<Profile*>(profileManager.getActiveProfile());
        if (active)
        {
            active->bgMain = savedScheme.bgMain;
            active->bgPanel = savedScheme.bgPanel;
            active->bgPanelBorder = savedScheme.bgPanelBorder;
            active->textNormal = savedScheme.textNormal;
            active->textLabel = savedScheme.textLabel;
            active->textValue = savedScheme.textValue;
            active->textAlert = savedScheme.textAlert;
            active->barNormal = savedScheme.barNormal;
            active->barWarning = savedScheme.barWarning;
            active->barAlert = savedScheme.barAlert;
            active->barBg = savedScheme.barBg;
            active->headerText = savedScheme.headerText;
            active->alertBg = savedScheme.alertBg;
            active->alertText = savedScheme.alertText;

            profileManager.save(configPath);
            std::cout << "[Config] Color scheme saved." << std::endl;
        }
    });

    // Создаём пустой GUI-кадр для отображения при запуске на основе профиля
    GUIFrame emptyFrame;
    emptyFrame.valid = true;
    emptyFrame.vehicleType = "Waiting for telemetry...";
    
    // Получаем список видимых индикаторов из профиля
    const Profile* activeProfile = profileManager.getActiveProfile();
    if (activeProfile)
    {
        // Группируем индикаторы по секциям
        std::unordered_map<std::string, std::vector<std::string>> sectionIndicators;
        
        for (const auto& iv : activeProfile->indicatorVisibility)
        {
            if (iv.visible)
            {
                sectionIndicators[iv.section].push_back(iv.name);
            }
        }
        
        // Создаём секции с видимыми индикаторами
        for (auto& [sectionName, indicators] : sectionIndicators)
        {
            IndicatorSection section;
            section.title = sectionName;
            
            for (const auto& indicatorName : indicators)
            {
                IndicatorSlot slot;
                slot.label = indicatorName + ":";
                slot.value = "0";
                slot.fraction = 0.0f;
                section.slots.push_back(slot);
            }
            
            emptyFrame.sections.push_back(section);
        }
    }
    
    gui.updateFrame(emptyFrame);

    // Устанавливаем начальную цветовую схему из профиля
    const Profile* defaultProfile = profileManager.getActiveProfile();
    if (defaultProfile)
    {
        ColorScheme scheme;
        scheme.bgMain = RGB(GetRValue(static_cast<UINT>(defaultProfile->bgMain)),
                            GetGValue(static_cast<UINT>(defaultProfile->bgMain)),
                            GetBValue(static_cast<UINT>(defaultProfile->bgMain)));
        scheme.bgPanel = RGB(GetRValue(static_cast<UINT>(defaultProfile->bgPanel)),
                             GetGValue(static_cast<UINT>(defaultProfile->bgPanel)),
                             GetBValue(static_cast<UINT>(defaultProfile->bgPanel)));
        scheme.bgPanelBorder = RGB(GetRValue(static_cast<UINT>(defaultProfile->bgPanelBorder)),
                                   GetGValue(static_cast<UINT>(defaultProfile->bgPanelBorder)),
                                   GetBValue(static_cast<UINT>(defaultProfile->bgPanelBorder)));
        scheme.textNormal = RGB(GetRValue(static_cast<UINT>(defaultProfile->textNormal)),
                                GetGValue(static_cast<UINT>(defaultProfile->textNormal)),
                                GetBValue(static_cast<UINT>(defaultProfile->textNormal)));
        scheme.textLabel = RGB(GetRValue(static_cast<UINT>(defaultProfile->textLabel)),
                               GetGValue(static_cast<UINT>(defaultProfile->textLabel)),
                               GetBValue(static_cast<UINT>(defaultProfile->textLabel)));
        scheme.textValue = RGB(GetRValue(static_cast<UINT>(defaultProfile->textValue)),
                               GetGValue(static_cast<UINT>(defaultProfile->textValue)),
                               GetBValue(static_cast<UINT>(defaultProfile->textValue)));
        scheme.textAlert = RGB(GetRValue(static_cast<UINT>(defaultProfile->textAlert)),
                               GetGValue(static_cast<UINT>(defaultProfile->textAlert)),
                               GetBValue(static_cast<UINT>(defaultProfile->textAlert)));
        scheme.barNormal = RGB(GetRValue(static_cast<UINT>(defaultProfile->barNormal)),
                               GetGValue(static_cast<UINT>(defaultProfile->barNormal)),
                               GetBValue(static_cast<UINT>(defaultProfile->barNormal)));
        scheme.barWarning = RGB(GetRValue(static_cast<UINT>(defaultProfile->barWarning)),
                                GetGValue(static_cast<UINT>(defaultProfile->barWarning)),
                                GetBValue(static_cast<UINT>(defaultProfile->barWarning)));
        scheme.barAlert = RGB(GetRValue(static_cast<UINT>(defaultProfile->barAlert)),
                              GetGValue(static_cast<UINT>(defaultProfile->barAlert)),
                              GetBValue(static_cast<UINT>(defaultProfile->barAlert)));
        scheme.barBg = RGB(GetRValue(static_cast<UINT>(defaultProfile->barBg)),
                           GetGValue(static_cast<UINT>(defaultProfile->barBg)),
                           GetBValue(static_cast<UINT>(defaultProfile->barBg)));
        scheme.headerText = RGB(GetRValue(static_cast<UINT>(defaultProfile->headerText)),
                                GetGValue(static_cast<UINT>(defaultProfile->headerText)),
                                GetBValue(static_cast<UINT>(defaultProfile->headerText)));
        scheme.alertBg = RGB(GetRValue(static_cast<UINT>(defaultProfile->alertBg)),
                             GetGValue(static_cast<UINT>(defaultProfile->alertBg)),
                             GetBValue(static_cast<UINT>(defaultProfile->alertBg)));
        scheme.alertText = RGB(GetRValue(static_cast<UINT>(defaultProfile->alertText)),
                               GetGValue(static_cast<UINT>(defaultProfile->alertText)),
                               GetBValue(static_cast<UINT>(defaultProfile->alertText)));
        gui.setColorScheme(scheme);
    }

    // Создаём реестр индикаторов
    IndicatorRegistry registry;

    // Регистрируем фабрики индикаторов
    registry.registerFactory("Altitude", [](const IndicatorConfig& config) {
        return std::make_unique<AltitudeIndicator>(config);
    });
    registry.registerFactory("TAS", [](const IndicatorConfig& config) {
        return std::make_unique<TASIndicator>(config);
    });
    registry.registerFactory("IAS", [](const IndicatorConfig& config) {
        return std::make_unique<IASIndicator>(config);
    });
    registry.registerFactory("Mach", [](const IndicatorConfig& config) {
        return std::make_unique<MachIndicator>(config);
    });
    registry.registerFactory("AoA", [](const IndicatorConfig& config) {
        return std::make_unique<AoAIndicator>(config);
    });
    registry.registerFactory("GForce", [](const IndicatorConfig& config) {
        return std::make_unique<GForceIndicator>(config);
    });
    registry.registerFactory("ClimbRate", [](const IndicatorConfig& config) {
        return std::make_unique<ClimbRateIndicator>(config);
    });
    registry.registerFactory("Fuel", [](const IndicatorConfig& config) {
        return std::make_unique<FuelIndicator>(config);
    });
    registry.registerFactory("FuelPercent", [](const IndicatorConfig& config) {
        return std::make_unique<FuelPercentIndicator>(config);
    });
    registry.registerFactory("FuelPercentFM", [](const IndicatorConfig& config) {
        return std::make_unique<FuelPercentFMIndicator>(config);
    });
    registry.registerFactory("TimeRemaining", [](const IndicatorConfig& config) {
        return std::make_unique<TimeRemainingIndicator>(config);
    });
    registry.registerFactory("Flaps", [](const IndicatorConfig& config) {
        return std::make_unique<FlapsIndicator>(config);
    });
    registry.registerFactory("Gear", [](const IndicatorConfig& config) {
        return std::make_unique<GearIndicator>(config);
    });
    registry.registerFactory("Airbrake", [](const IndicatorConfig& config) {
        return std::make_unique<AirbrakeIndicator>(config);
    });
    registry.registerFactory("RPM", [](const IndicatorConfig& config) {
        return std::make_unique<RPMIndicator>(config);
    });
    registry.registerFactory("RPM2", [](const IndicatorConfig& config) {
        return std::make_unique<RPMIndicator>(config);
    });
    registry.registerFactory("Thrust", [](const IndicatorConfig& config) {
        return std::make_unique<ThrustIndicator>(config);
    });
    registry.registerFactory("Thrust2", [](const IndicatorConfig& config) {
        return std::make_unique<ThrustIndicator>(config);
    });

    // Создаём индикаторы с конфигурацией
    std::vector<IndicatorConfig> configs;
    configs.emplace_back("Altitude", "H, m", "", IndicatorType::NUMBER, "m", 1.0f, 1);
    configs.emplace_back("TAS", "TAS, km/h", "", IndicatorType::NUMBER, "km/h", 1.0f, 1);
    configs.emplace_back("IAS", "IAS, km/h", "", IndicatorType::NUMBER, "km/h", 1.0f, 1);
    configs.emplace_back("Mach", "M", "", IndicatorType::NUMBER, "", 1.0f, 2);
    configs.emplace_back("AoA", "AoA, deg", "", IndicatorType::NUMBER, "deg", 1.0f, 1);
    configs.emplace_back("GForce", "Ny", "", IndicatorType::NUMBER, "g", 1.0f, 2);
    configs.emplace_back("ClimbRate", "Vy, m/s", "", IndicatorType::NUMBER, "m/s", 1.0f, 1);
    configs.emplace_back("Fuel", "Mfuel, kg", "", IndicatorType::NUMBER, "kg", 1.0f, 0);
    configs.emplace_back("FuelPercent", "", "", IndicatorType::NUMBER, "%", 1.0f, 1);
    configs.emplace_back("FuelPercentFM", "", "", IndicatorType::NUMBER, "%", 1.0f, 1);
    configs.emplace_back("TimeRemaining", "", "", IndicatorType::NUMBER, "", 1.0f, 0);
    configs.emplace_back("Flaps", "flaps, %", "", IndicatorType::NUMBER, "%", 1.0f, 0);
    configs.emplace_back("Gear", "gear, %", "", IndicatorType::NUMBER, "%", 1.0f, 0);
    configs.emplace_back("Airbrake", "airbrake, %", "", IndicatorType::NUMBER, "%", 1.0f, 0);
    configs.emplace_back("RPM", "RPM 1", "", IndicatorType::NUMBER, "rpm", 1.0f, 0);
    configs.emplace_back("Thrust", "thrust 1, kgs", "", IndicatorType::NUMBER, "kgs", 1.0f, 0);
    configs.emplace_back("RPM2", "RPM 2", "", IndicatorType::NUMBER, "rpm", 1.0f, 0);
    configs.emplace_back("Thrust2", "thrust 2, kgs", "", IndicatorType::NUMBER, "kgs", 1.0f, 0);

    // Добавляем groupName
    for (size_t i = 0; i < configs.size(); ++i)
    {
        if (i < 7) configs[i].groupName = "Flight";
        else if (i < 11) configs[i].groupName = "Fuel";
        else if (i < 12) configs[i].groupName = "Controls";
        else if (i < 14) configs[i].groupName = "Engine 1";
        else configs[i].groupName = "Engine 2";
    }

    // Добавляем алерты
    // High G — сработает когда G > 95% от критического значения из FM-базы
    AlertRule highGAlert;
    highGAlert.name = "High G";
    highGAlert.relation = AlertRule::Relation::GREATER;
    highGAlert.thresholdA = 5.0f;
    highGAlert.colorName = "RED";
    highGAlert.priority = 2;
    configs[5].alerts.push_back(highGAlert);

    // Low Fuel — сработает когда Mfuel < 1000 кг
    AlertRule lowFuelAlert;
    lowFuelAlert.name = "Low Fuel";
    lowFuelAlert.relation = AlertRule::Relation::LESS;
    lowFuelAlert.thresholdA = 1000.0f;
    lowFuelAlert.colorName = "YELLOW";
    lowFuelAlert.priority = 1;
    configs[7].alerts.push_back(lowFuelAlert);

    // Low Time Remaining — сработает когда топлива < 2 минут
    AlertRule lowTimeRemainingAlert;
    lowTimeRemainingAlert.name = "Low Time Remaining";
    lowTimeRemainingAlert.relation = AlertRule::Relation::LESS;
    lowTimeRemainingAlert.thresholdA = 120.0f;  // 2 минуты = 120 секунд
    lowTimeRemainingAlert.colorName = "RED";
    lowTimeRemainingAlert.priority = 2;
    configs[10].alerts.push_back(lowTimeRemainingAlert);

    // High Speed — сработает когда IAS > 95% от критической скорости из FM-базы
    AlertRule highSpeedAlert;
    highSpeedAlert.name = "High Speed";
    highSpeedAlert.relation = AlertRule::Relation::GREATER;
    highSpeedAlert.thresholdA = 500.0f;
    highSpeedAlert.colorName = "ORANGE";
    highSpeedAlert.priority = 2;
    configs[2].alerts.push_back(highSpeedAlert);

    // Создаём индикаторы
    for (const auto& config : configs)
    {
        auto indicator = registry.createIndicator(config);
        if (indicator)
        {
            indicator->init();
            registry.addIndicator(std::move(indicator));
        }
    }

    // std::cout << "Indicators registered: " << registry.getIndicatorCount() << std::endl;
    // std::cout << std::endl;

    // Создаём систему алертов
    AlertSystem alertSystem;
    alertSystem.onAlert([](const std::string& alertName, float value,
                           const std::string& color) {
        // std::cout << "*** ALERT: " << alertName << " (value=" << value
        //           << ", color=" << color << ") ***" << std::endl;
    });

    // Создаём HTTP-клиент
    HTTPClient httpClient;
    httpClient.initialize("127.0.0.1", 8111, "/state", 1000);
    httpClient.initializeIndicators("127.0.0.1", 8111, "/indicators", 1000);

    // Создаём процессор телеметрии
    TelemetryProcessor processor;

    // Текущее имя ТС и расход топлива
    std::string currentVehicleType;
    float currentFuelConsumeKgS = 0.0f;

    // Колбэк для /indicators — определение текущего ТС и расхода топлива
    httpClient.onIndicatorsResponse([&currentVehicleType, &currentFuelConsumeKgS](const std::string& data) {
        auto parsed = IndicatorsParser::parse(data);
        if (parsed.has_value())
        {
            currentVehicleType = parsed.value().type;
            currentFuelConsumeKgS = parsed.value().fuelConsumeKgS;
        }
    });

    // Колбэк для /state — обработка телеметрии
    httpClient.onResponse([&gui, &processor, &registry, &alertSystem,
                           &fmDatabase, &currentVehicleType, &currentFuelConsumeKgS, &profileManager](const std::string& data) {
        // Парсим JSON
        auto parsed = DataParser::parse(data);
        if (parsed.has_value())
        {
            // Обрабатываем данные
            TelemetryRawData raw = parsed.value();
            // Объединяем расход топлива из /indicators
            raw.fuelConsumeKgS = currentFuelConsumeKgS;



            // Ищем FM-данные по текущему типу ТС
            const AircraftFMData* fmData = nullptr;
            float criticalG = 9.0f;
            float critAirSpd = 0.0f;

            if (fmDatabase.isLoaded() && !currentVehicleType.empty())
            {
                const AircraftFMData* found = fmDatabase.findByWTName(currentVehicleType);
                if (found)
                {
                    fmData = found;
                    criticalG = found->critWingOverload;
                    critAirSpd = found->critAirSpd;



                    // Устанавливаем количество двигателей
                    if (found->numEngines > 0)
                    {
                        registry.setEngineCount(found->numEngines);
                    }
                }
            }

            // Устанавливаем критические значения для индикаторов
            if (criticalG > 0.0f)
            {
                registry.setCriticalG(criticalG);
            }
            if (critAirSpd > 0.0f)
            {
                registry.setCriticalAirSpeed(critAirSpd);
            }

            // Оцениваем все индикаторы
            auto results = registry.evaluateAll(raw, fmData);

            // Проверяем алерты
            alertSystem.checkAll(results);
            auto activeAlerts = alertSystem.getActiveAlerts();

            // Применяем профиль для текущего ТС
            const Profile* activeProfile = profileManager.getActiveProfile(currentVehicleType);
            if (activeProfile)
            {
                ColorScheme colors;
                colors.bgMain = RGB(GetRValue(activeProfile->bgMain),
                                    GetGValue(activeProfile->bgMain),
                                    GetBValue(activeProfile->bgMain));
                colors.bgPanel = RGB(GetRValue(activeProfile->bgPanel),
                                     GetGValue(activeProfile->bgPanel),
                                     GetBValue(activeProfile->bgPanel));
                colors.bgPanelBorder = RGB(GetRValue(activeProfile->bgPanelBorder),
                                           GetGValue(activeProfile->bgPanelBorder),
                                           GetBValue(activeProfile->bgPanelBorder));
                colors.textNormal = RGB(GetRValue(activeProfile->textNormal),
                                        GetGValue(activeProfile->textNormal),
                                        GetBValue(activeProfile->textNormal));
                colors.textLabel = RGB(GetRValue(activeProfile->textLabel),
                                       GetGValue(activeProfile->textLabel),
                                       GetBValue(activeProfile->textLabel));
                colors.textValue = RGB(GetRValue(activeProfile->textValue),
                                       GetGValue(activeProfile->textValue),
                                       GetBValue(activeProfile->textValue));
                colors.textAlert = RGB(GetRValue(activeProfile->textAlert),
                                       GetGValue(activeProfile->textAlert),
                                       GetBValue(activeProfile->textAlert));
                colors.barNormal = RGB(GetRValue(activeProfile->barNormal),
                                       GetGValue(activeProfile->barNormal),
                                       GetBValue(activeProfile->barNormal));
                colors.barWarning = RGB(GetRValue(activeProfile->barWarning),
                                        GetGValue(activeProfile->barWarning),
                                        GetBValue(activeProfile->barWarning));
                colors.barAlert = RGB(GetRValue(activeProfile->barAlert),
                                      GetGValue(activeProfile->barAlert),
                                      GetBValue(activeProfile->barAlert));
                colors.barBg = RGB(GetRValue(activeProfile->barBg),
                                   GetGValue(activeProfile->barBg),
                                   GetBValue(activeProfile->barBg));
                colors.headerText = RGB(GetRValue(activeProfile->headerText),
                                        GetGValue(activeProfile->headerText),
                                        GetBValue(activeProfile->headerText));
                colors.alertBg = RGB(GetRValue(activeProfile->alertBg),
                                     GetGValue(activeProfile->alertBg),
                                     GetBValue(activeProfile->alertBg));
                colors.alertText = RGB(GetRValue(activeProfile->alertText),
                                       GetGValue(activeProfile->alertText),
                                       GetBValue(activeProfile->alertText));
                gui.setColorScheme(colors);
            }

            // Строим GUI-кадр
            GUIFrame frame;
            buildGUIFrame(frame, results, activeAlerts, currentVehicleType, &profileManager);

            // Обновляем GUI
            gui.updateFrame(frame);
        }
    });

    // std::cout << "Started telemetry polling (interval: 25 ms)." << std::endl;
    // std::cout << "Press Enter to stop..." << std::endl;
    // std::cout << std::endl;

    // Гарантируем, что GUI окно показано и отрисовано до запуска HTTP
    ShowWindow(gui.getHWND(), SW_SHOW);
    UpdateWindow(gui.getHWND());
    Sleep(100);

    httpClient.startPolling(25);
    httpClient.startPollingIndicators(25);

    // Запускаем GUI-цикл (блокирующий)
    gui.run();

    // Останавливаем опрос
    httpClient.stopPolling();
    httpClient.stopPollingIndicators();

    // Сохраняем смещения оверлея и конфигурацию
    gui.saveOverlayOffsets();
    profileManager.save(configPath);
    // std::cout << "Configuration saved to: " << configPath << std::endl;

    // std::cout << "Finished." << std::endl;
    return 0;
}
