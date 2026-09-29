#include "config/ConfigManager.h"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <shlobj.h>

#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string getAppDataPath()
{
    // C:\Users\<User>\AppData\Roaming\WTAID
    char appData[512];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appData)))
    {
        return std::string(appData) + "\\WTAID";
    }
    return "WTAID";
}

// ---------------------------------------------------------------------------
// ConfigManager
// ---------------------------------------------------------------------------
ConfigManager::ConfigManager()
{
    initDefaultConfig();
}

void ConfigManager::initDefaultConfig()
{
    root_ = nlohmann::json::object();
    root_["version"] = "1.0";
    root_["profiles"] = nlohmann::json::array();
}

bool ConfigManager::load(const std::string& path)
{
    configPath_ = path;

    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[ConfigManager] Failed to open config: " << path << std::endl;
        return false;
    }

    try
    {
        file >> root_;
        file.close();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ConfigManager] Parse error: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::save(const std::string& path)
{
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open())
    {
        std::cerr << "[ConfigManager] Failed to save config: " << path << std::endl;
        return false;
    }

    try
    {
        file << root_.dump(2);
        file.close();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ConfigManager] Save error: " << e.what() << std::endl;
        return false;
    }
}

std::string ConfigManager::getDefaultPath()
{
    return "config\\config.json";
}

void ConfigManager::ensureDirectoryExists(const std::string& path)
{
    fs::path p(path);
    if (p.has_parent_path())
    {
        fs::create_directories(p.parent_path());
    }
}

// ---------------------------------------------------------------------------
// ProfileManager
// ---------------------------------------------------------------------------
ProfileManager::ProfileManager()
{
    // Создаём профиль по умолчанию
    profiles_.push_back(createDefaultProfile());
    activeProfileName_ = "Default";
}

bool ProfileManager::load(const std::string& path)
{
    ConfigManager config;
    if (!config.load(path))
        return false;

    // Очищаем текущие профили
    profiles_.clear();

    auto& profiles = config.root_["profiles"];
    if (!profiles.is_array())
        return true;

    for (const auto& p : profiles)
    {
        auto profile = std::make_unique<Profile>();

        profile->name = p.value("name", "Unnamed");
        profile->vehicleType = p.value("vehicleType", "");
        profile->vehicleTypeFilter = p.value("vehicleTypeFilter", "");
        profile->isActive = p.value("isActive", true);

        // Цвета
        profile->bgMain = p.value("bgMain", 0x0F1216);
        profile->bgPanel = p.value("bgPanel", 0x191E26);
        profile->bgPanelBorder = p.value("bgPanelBorder", 0x323C4B);
        profile->textNormal = p.value("textNormal", 0xC8D2DC);
        profile->textLabel = p.value("textLabel", 0x8C9BAA);
        profile->textValue = p.value("textValue", 0xFFFFFF);
        profile->textAlert = p.value("textAlert", 0xFF4646);
        profile->barNormal = p.value("barNormal", 0x3CA0FF);
        profile->barWarning = p.value("barWarning", 0xFFB428);
        profile->barAlert = p.value("barAlert", 0xFF3232);
        profile->barBg = p.value("barBg", 0x232A34);
        profile->headerText = p.value("headerText", 0x64C8FF);
        profile->alertBg = p.value("alertBg", 0x501414);
        profile->alertText = p.value("alertText", 0xFF6464);

        // Видимость индикаторов
        if (p.contains("indicatorVisibility") && p["indicatorVisibility"].is_array())
        {
            for (const auto& iv : p["indicatorVisibility"])
            {
                IndicatorVisibility indVis;
                indVis.name = iv.value("name", "");
                indVis.visible = iv.value("visible", true);
                indVis.section = iv.value("section", "");
                indVis.overlayOffsetX = iv.value("overlayOffsetX", 0);
                indVis.overlayOffsetY = iv.value("overlayOffsetY", 0);
                profile->indicatorVisibility.push_back(indVis);
            }
        }

        std::cerr << "[Config] Loaded profile: " << profile->name << " with " << profile->indicatorVisibility.size() << " indicators" << std::endl;
        profiles_.push_back(std::move(profile));
    }

    return true;
}

bool ProfileManager::save(const std::string& path)
{
    ConfigManager config;

    nlohmann::json profiles = nlohmann::json::array();

    for (const auto& p : profiles_)
    {
        nlohmann::json j;
        j["name"] = p->name;
        j["vehicleType"] = p->vehicleType;
        j["vehicleTypeFilter"] = p->vehicleTypeFilter;
        j["isActive"] = p->isActive;

        // Цвета
        j["bgMain"] = p->bgMain;
        j["bgPanel"] = p->bgPanel;
        j["bgPanelBorder"] = p->bgPanelBorder;
        j["textNormal"] = p->textNormal;
        j["textLabel"] = p->textLabel;
        j["textValue"] = p->textValue;
        j["textAlert"] = p->textAlert;
        j["barNormal"] = p->barNormal;
        j["barWarning"] = p->barWarning;
        j["barAlert"] = p->barAlert;
        j["barBg"] = p->barBg;
        j["headerText"] = p->headerText;
        j["alertBg"] = p->alertBg;
        j["alertText"] = p->alertText;

        // Видимость индикаторов
        nlohmann::json indicators = nlohmann::json::array();
        for (const auto& iv : p->indicatorVisibility)
        {
            nlohmann::json jiv;
            jiv["name"] = iv.name;
            jiv["visible"] = iv.visible;
            jiv["section"] = iv.section;
            jiv["overlayOffsetX"] = iv.overlayOffsetX;
            jiv["overlayOffsetY"] = iv.overlayOffsetY;
            indicators.push_back(jiv);
        }
        j["indicatorVisibility"] = indicators;

        profiles.push_back(j);
    }

    config.root_["profiles"] = profiles;
    
    // Логирование что сохраняется
    std::cerr << "[Config] Saving profiles, checking Fuel offset..." << std::endl;
    for (const auto& p : profiles_)
    {
        for (const auto& iv : p->indicatorVisibility)
        {
            if (iv.name == "Fuel")
            {
                std::cerr << "[Config]   Fuel in profile: offsetX=" << iv.overlayOffsetX << " offsetY=" << iv.overlayOffsetY << std::endl;
            }
        }
    }
    
    bool result = config.save(path);
    std::cerr << "[Config] Save " << (result ? "SUCCESS" : "FAILED") << " to: " << path << std::endl;
    return result;
}

void ProfileManager::addProfile(std::unique_ptr<Profile> profile)
{
    if (profile)
    {
        profiles_.push_back(std::move(profile));
    }
}

bool ProfileManager::removeProfile(const std::string& name)
{
    auto it = std::find_if(profiles_.begin(), profiles_.end(),
        [&name](const std::unique_ptr<Profile>& p) {
            return p->name == name;
        });

    if (it != profiles_.end())
    {
        if ((*it)->name == activeProfileName_)
        {
            activeProfileName_ = "";
        }
        profiles_.erase(it);
        return true;
    }
    return false;
}

const Profile* ProfileManager::getProfile(const std::string& name) const
{
    for (const auto& p : profiles_)
    {
        if (p->name == name)
            return p.get();
    }
    return nullptr;
}

const Profile* ProfileManager::getActiveProfile(const std::string& vehicleType) const
{
    // Ищем профиль по типу ТС
    const Profile* found = findProfileByType(vehicleType);
    if (found)
        return found;

    // Ищем активный профиль
    for (const auto& p : profiles_)
    {
        if (p->isActive && p->vehicleType.empty())
            return p.get();
    }

    // Возвращаем первый профиль
    if (!profiles_.empty())
        return profiles_[0].get();

    return nullptr;
}

const std::vector<std::unique_ptr<Profile>>& ProfileManager::getAllProfiles() const
{
    return profiles_;
}

void ProfileManager::setActiveProfile(const std::string& name)
{
    for (const auto& p : profiles_)
    {
        if (p->name == name)
        {
            activeProfileName_ = name;
            return;
        }
    }
}

const Profile* ProfileManager::getActiveProfile() const
{
    for (const auto& p : profiles_)
    {
        if (p->name == activeProfileName_)
            return p.get();
    }

    if (!profiles_.empty())
        return profiles_[0].get();

    return nullptr;
}

bool ProfileManager::isIndicatorVisible(const std::string& indicatorName) const
{
    const Profile* profile = getActiveProfile();
    if (!profile)
        return true;

    for (const auto& iv : profile->indicatorVisibility)
    {
        if (iv.name == indicatorName)
            return iv.visible;
    }

    // Если индикатор не найден в настройках, показываем по умолчанию
    return true;
}

void ProfileManager::setIndicatorVisible(const std::string& indicatorName, bool visible)
{
    Profile* profile = const_cast<Profile*>(getActiveProfile());
    if (!profile)
        return;

    // Ищем существующую запись
    for (auto& iv : profile->indicatorVisibility)
    {
        if (iv.name == indicatorName)
        {
            iv.visible = visible;
            return;
        }
    }

    // Добавляем новую запись
    IndicatorVisibility indVis;
    indVis.name = indicatorName;
    indVis.visible = visible;
    indVis.section = "";
    profile->indicatorVisibility.push_back(indVis);
}

std::unique_ptr<Profile> ProfileManager::createDefaultProfile()
{
    auto profile = std::make_unique<Profile>();
    profile->name = "Default";
    profile->vehicleType = "";
    profile->isActive = true;
    profile->indicatorVisibility = getDefaultIndicatorVisibility();
    return profile;
}

std::vector<IndicatorVisibility> ProfileManager::getDefaultIndicatorVisibility()
{
    // Все встроенные индикаторы с их секциями
    return {
        {"Altitude", true, "FLIGHT"},
        {"TAS", true, "FLIGHT"},
        {"IAS", true, "FLIGHT"},
        {"Mach", true, "FLIGHT"},
        {"AoA", true, "FLIGHT"},
        {"GForce", true, "FLIGHT"},
        {"ClimbRate", true, "FLIGHT"},
        {"Fuel", true, "FUEL"},
        {"FuelPercent", true, "FUEL"},
        {"FuelPercentFM", true, "FUEL"},
        {"TimeRemaining", true, "FUEL"},
        {"Flaps", true, "CONTROLS"},
        {"Gear", true, "CONTROLS"},
        {"Airbrake", true, "CONTROLS"},
        {"RPM", true, "ENGINE 1"},
        {"Thrust", true, "ENGINE 1"},
        {"RPM2", true, "ENGINE 2"},
        {"Thrust2", true, "ENGINE 2"}
    };
}

const Profile* ProfileManager::findProfileByType(const std::string& vehicleType) const
{
    if (vehicleType.empty())
        return nullptr;

    for (const auto& p : profiles_)
    {
        if (!p->vehicleType.empty() && p->vehicleType == vehicleType)
            return p.get();
    }
    return nullptr;
}
