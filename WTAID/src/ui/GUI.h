#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>

#include "data/TelemetryData.h"
#include "indicators/Indicator.h"
#include "ui/ColorPickerDialog.h"
#include "config/ConfigManager.h"

// Forward declarations для overlay
struct GUIFrame;
struct ColorScheme;
bool createOverlayWindow(int width, int height);
void destroyOverlayWindow();
void updateOverlayFrame(const GUIFrame& frame, const ColorScheme& colors);

// ---------------------------------------------------------------------------
// Структура одного строкового индикатора
// ---------------------------------------------------------------------------
struct IndicatorSlot
{
    std::string label;            // "Altitude:"
    std::string value;            // "1250 m"
    float fraction = 0.0f;        // 0..1 для прогресс-бара
    float maxVal = 1.0f;          // максимальное значение для шкалы
    bool alert = false;
    bool horizontalBar = true;    // горизонтальный или вертикальный бар
    
    // Независимое перемещение
    bool selected = false;        // выбран для перемещения
    int offsetX = 0;              // независимый сдвиг по X
    int offsetY = 0;              // независимый сдвиг по Y
    bool moved = false;           // был ли уже сдвинут
};

// ---------------------------------------------------------------------------
// Структура секции (группы) индикаторов
// ---------------------------------------------------------------------------
struct IndicatorSection
{
    std::string title;            // "FLIGHT", "CONTROLS", ...
    std::vector<IndicatorSlot> slots;
};

// ---------------------------------------------------------------------------
// Структура всего кадра GUI
// ---------------------------------------------------------------------------
struct GUIFrame
{
    std::string vehicleType;
    std::vector<IndicatorSection> sections;
    std::vector<std::string> activeAlerts;
    bool valid = false;
};

// ---------------------------------------------------------------------------
// Цветовая схема
// ---------------------------------------------------------------------------
struct ColorScheme
{
    COLORREF bgMain = RGB(15, 18, 22);
    COLORREF bgPanel = RGB(25, 30, 38);
    COLORREF bgPanelBorder = RGB(50, 60, 75);
    COLORREF textNormal = RGB(200, 210, 220);
    COLORREF textLabel = RGB(140, 155, 170);
    COLORREF textValue = RGB(255, 255, 255);
    COLORREF textAlert = RGB(255, 70, 70);
    COLORREF barNormal = RGB(60, 160, 255);
    COLORREF barWarning = RGB(255, 180, 40);
    COLORREF barAlert = RGB(255, 50, 50);
    COLORREF barBg = RGB(35, 42, 52);
    COLORREF headerText = RGB(100, 200, 255);
    COLORREF alertBg = RGB(80, 20, 20);
    COLORREF alertText = RGB(255, 100, 100);
};

// ---------------------------------------------------------------------------
// Колбэк для сохранения конфигурации
// ---------------------------------------------------------------------------
typedef std::function<void(const ColorScheme&)> SaveConfigCallback;

// ---------------------------------------------------------------------------
// Основной класс GUI — HUD-интерфейс War Thunder Telemetry
// ---------------------------------------------------------------------------
class GUI
{
public:
    GUI();
    ~GUI();

    // Запрещаем копирование
    GUI(const GUI&) = delete;
    GUI& operator=(const GUI&) = delete;

    /**
     * Создать и показать окно GUI.
     * @param width   Ширина окна в пикселях.
     * @param height  Высота окна в пикселях.
     * @return true если окно успешно создано.
     */
    bool create(int width = 720, int height = 680);

    /**
     * Закрыть окно.
     */
    void close();

    /**
     * Установить новый кадр данных для отрисовки.
     * @param frame  Сформированный кадр GUI.
     */
    void updateFrame(const GUIFrame& frame);

    /**
     * Получить дескриптор окна.
     */
    HWND getHWND() const;

    /**
     * Установить цветовую схему.
     */
    void setColorScheme(const ColorScheme& scheme);

    /**
     * Установить колбэк для сохранения конфигурации.
     */
    void setSaveCallback(SaveConfigCallback callback);

    /**
     * Запустить цикл обработки сообщений (блокирующий).
     */
    void run();

    /**
     * Завершить цикл обработки сообщений.
     */
    void stop();

    /**
     * Проверить, запущен ли цикл сообщений.
     */
    bool isRunning() const;

    // Внутренние данные
    HWND hwnd_ = nullptr;
    bool running_ = false;
    bool editMode_ = false;
    ColorScheme colors_;
    ColorScheme editColors_;
    HBRUSH hBrushBg_ = nullptr;
    HBRUSH hBrushPanel_ = nullptr;
    HFONT hFontMain_ = nullptr;
    HFONT hFontBold_ = nullptr;
    HFONT hFontSmall_ = nullptr;
    
    // Overlay
    bool overlayEnabled_ = false;
    int overlayWidth_ = 720;
    int overlayHeight_ = 680;

    // Состояние выделения индикаторов (сохраняется между обновлениями)
    struct SelectedSlot {
        int sectionIdx = -1;
        int slotIdx = -1;
        int offsetX = 0;
        int offsetY = 0;
    };
    std::vector<SelectedSlot> selectedSlots_;

    // ProfileManager для доступа к настройкам индикаторов
    ProfileManager* profileManager_ = nullptr;

    // Текущие данные
    std::mutex frameMutex_;
    GUIFrame currentFrame_;

    // Размер окна
    int windowWidth_ = 720;
    int windowHeight_ = 680;

    // Колбэк для сохранения
    SaveConfigCallback saveCallback_;
    
    // Кэш смещений оверлея по имени индикатора
    std::unordered_map<std::string, std::pair<int, int>> cachedOverlayOffsets_;
    
    // Метод для инициализации кэша из профиля
    void initOverlayOffsetCache(ProfileManager& pm);

    // Внутренние методы
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleCreate(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handlePaint(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleSize(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleDestroy(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleTimer(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMouse(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMouseUp(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleKeyDown(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void renderFrame(HDC hdc, int width, int height);
    void renderSection(HDC hdc, const IndicatorSection& section, int x, int y, int width, int sectionHeight);
    void renderIndicatorSlot(HDC hdc, const IndicatorSlot& slot, int x, int y, int width);
    void renderAlerts(HDC hdc, const std::vector<std::string>& alerts, int x, int y, int width);
    void renderVehicleHeader(HDC hdc, const std::string& vehicleType, int x, int y, int width);
    void renderEditMode(HDC hdc, int width, int height);
    void createFonts();
    void destroyFonts();
    void createBrushes();
    void destroyBrushes();
    
    // Методы для overlay
    void renderToOverlay();
    bool isOverlayEnabled() const { return overlayEnabled_; }
    void setOverlayEnabled(bool enabled);
    void toggleEditMode();
    void applyEditColors();
    
    // Применить состояние выделения к currentFrame_
    void applySelectionState();

    // Установить ProfileManager
    void setProfileManager(ProfileManager* pm) { profileManager_ = pm; }

    // Показать диалог выбора индикаторов
    void showIndicatorSelector();
    
    // Рассчитать минимальную высоту окна
    int calculateMinHeight() const;
    
    // Сохранить смещения оверлея в профиль
    void saveOverlayOffsets();
};

// Глобальный указатель на GUI для доступа из OverlayWindow
extern GUI* g_guiInstance;
