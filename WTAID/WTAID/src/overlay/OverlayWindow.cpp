#include "overlay/OverlayWindow.h"
#include "overlay/OverlayRenderer.h"
#include "ui/GUI.h"
#include "config/ConfigManager.h"
#include <windows.h>
#include <winuser.h>
#include <string>
#include <mutex>
#include <iostream>

// ---------------------------------------------------------------------------
// ID глобальных хоткеев
// ---------------------------------------------------------------------------
static constexpr int HOTKEY_MOVE_LEFT  = 0x0100;
static constexpr int HOTKEY_MOVE_RIGHT = 0x0101;
static constexpr int HOTKEY_MOVE_UP    = 0x0102;
static constexpr int HOTKEY_MOVE_DOWN  = 0x0103;

// ---------------------------------------------------------------------------
// Глобальные данные
// ---------------------------------------------------------------------------
static OverlayRenderer* g_overlayRenderer = nullptr;
static std::mutex g_overlayMutex;
static bool g_classRegistered = false;
static bool g_hotkeysRegistered = false;

// ---------------------------------------------------------------------------
// Прототипы
// ---------------------------------------------------------------------------
static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool registerHotkeysForWindow(HWND hwnd);
static void unregisterHotkeysForWindow(HWND hwnd);



// ---------------------------------------------------------------------------
// Создание окна оверлея
// ---------------------------------------------------------------------------
bool createOverlayWindow(int width, int height) {
    std::lock_guard<std::mutex> lock(g_overlayMutex);
    
    if (g_overlayRenderer) {
        return false;
    }

    // Получаем размеры монитора
    int monitorWidth = GetSystemMetrics(SM_CXSCREEN);
    int monitorHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Используем размеры монитора для полноэкранного оверлея
    width = monitorWidth;
    height = monitorHeight;

    // Получаем hInstance
    HINSTANCE hInst = GetModuleHandleW(nullptr);
    if (!hInst) {
        hInst = GetModuleHandleW(L"WTAID.exe");
    }
    if (!hInst) {
        hInst = GetModuleHandleA(nullptr);
    }
    if (!hInst) {
        return false;
    }

    // Регистрируем класс окна (только один раз)
    if (!g_classRegistered) {
        const wchar_t* className = L"WTAID_OverlayClass";
        
        WNDCLASSW wc = {};
        wc.lpfnWndProc = OverlayWndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));
        wc.hbrBackground = nullptr; // Прозрачный фон
        wc.style = CS_HREDRAW | CS_VREDRAW;

        if (!RegisterClassW(&wc)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) {
                return false;
            }
        }
        g_classRegistered = true;
    }

    // Создаём полноэкранное окно с WS_EX_TOPMOST для наложения поверх других
    // WS_EX_LAYERED + WS_EX_TRANSPARENT для прозрачности фона
    // WS_EX_TOOLWINDOW для скрытия с панели задач
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        L"WTAID_OverlayClass",
        L"WTAID Overlay",
        WS_POPUP,
        0, 0,
        width, height,
        nullptr,
        nullptr,
        hInst,
        nullptr
    );

    if (!hwnd) {
        DWORD err = GetLastError();
        return false;
    }

    // Устанавливаем цвет ключа для прозрачности фона (RGB(0,0,0) = прозрачный)
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    // Создаём рендерер
    OverlayRenderer* renderer = new (std::nothrow) OverlayRenderer();
    if (!renderer) {
        DestroyWindow(hwnd);
        return false;
    }

    // Инициализируем рендерер
    if (!renderer->initialize(hwnd, width, height)) {
        delete renderer;
        DestroyWindow(hwnd);
        return false;
    }

    // Регистрируем глобальные хоткеи
    if (!registerHotkeysForWindow(hwnd)) {
        // Хоткеи не критичны, продолжаем без них
    }

    // Показываем окно
    renderer->show();
    g_overlayRenderer = renderer;

    return true;
}

// ---------------------------------------------------------------------------
// Уничтожение окна оверлея
// ---------------------------------------------------------------------------
void destroyOverlayWindow() {
    std::lock_guard<std::mutex> lock(g_overlayMutex);

    if (g_overlayRenderer) {
        g_overlayRenderer->hide();
        g_overlayRenderer->shutdown();
        delete g_overlayRenderer;
        g_overlayRenderer = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Обновление кадра оверлея
// ---------------------------------------------------------------------------
void updateOverlayFrame(const GUIFrame& frame, const ColorScheme& colors) {
    std::lock_guard<std::mutex> lock(g_overlayMutex);

    if (g_overlayRenderer && g_overlayRenderer->isInitialized()) {
        g_overlayRenderer->render(frame, colors);
    }
}

// ---------------------------------------------------------------------------
// Показать оверлей
// ---------------------------------------------------------------------------
void showOverlay() {
    std::lock_guard<std::mutex> lock(g_overlayMutex);

    if (g_overlayRenderer) {
        g_overlayRenderer->show();
    }
}

// ---------------------------------------------------------------------------
// Скрыть оверлей
// ---------------------------------------------------------------------------
void hideOverlay() {
    std::lock_guard<std::mutex> lock(g_overlayMutex);

    if (g_overlayRenderer) {
        g_overlayRenderer->hide();
    }
}

// ---------------------------------------------------------------------------
// Регистрация глобальных хоткеев
// ---------------------------------------------------------------------------
static bool registerHotkeysForWindow(HWND hwnd) {
    if (g_hotkeysRegistered) {
        return true;
    }

    // SHIFT+ALT+LEFT
    if (!RegisterHotKey(hwnd, HOTKEY_MOVE_LEFT, MOD_SHIFT | MOD_ALT, VK_LEFT)) {
        return false;
    }
    // SHIFT+ALT+RIGHT
    if (!RegisterHotKey(hwnd, HOTKEY_MOVE_RIGHT, MOD_SHIFT | MOD_ALT, VK_RIGHT)) {
        return false;
    }
    // SHIFT+ALT+UP
    if (!RegisterHotKey(hwnd, HOTKEY_MOVE_UP, MOD_SHIFT | MOD_ALT, VK_UP)) {
        return false;
    }
    // SHIFT+ALT+DOWN
    if (!RegisterHotKey(hwnd, HOTKEY_MOVE_DOWN, MOD_SHIFT | MOD_ALT, VK_DOWN)) {
        return false;
    }

    g_hotkeysRegistered = true;
    return true;
}

// ---------------------------------------------------------------------------
// Отмена регистрации хоткеев
// ---------------------------------------------------------------------------
static void unregisterHotkeysForWindow(HWND hwnd) {
    if (!g_hotkeysRegistered) {
        return;
    }

    UnregisterHotKey(hwnd, HOTKEY_MOVE_LEFT);
    UnregisterHotKey(hwnd, HOTKEY_MOVE_RIGHT);
    UnregisterHotKey(hwnd, HOTKEY_MOVE_UP);
    UnregisterHotKey(hwnd, HOTKEY_MOVE_DOWN);
    g_hotkeysRegistered = false;
}

// ---------------------------------------------------------------------------
// Обработка сообщений окна
// ---------------------------------------------------------------------------
static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            return 0;
        }

        case WM_PAINT: {
            // Оверлей отрисовывается через D2D, игнорируем стандартную отрисовку
            ValidateRect(hwnd, nullptr);
            return 0;
        }

        case WM_DESTROY: {
            // Отменяем регистрацию хоткеев
            unregisterHotkeysForWindow(hwnd);
            // Не выходим из приложения, просто скрываем
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }

        case WM_HOTKEY: {
            int dx = 0;
            int dy = 0;

            switch (wParam) {
                case HOTKEY_MOVE_LEFT:  dx = -10; break;
                case HOTKEY_MOVE_RIGHT: dx =  10; break;
                case HOTKEY_MOVE_UP:    dy = -10; break;
                case HOTKEY_MOVE_DOWN:  dy =  10; break;
            }

            if (dx != 0 || dy != 0) {
                std::lock_guard<std::mutex> lock(g_overlayMutex);
                if (g_overlayRenderer) {
                    // Проверяем, есть ли выделенные слоты
                    const auto& infos = g_overlayRenderer->getSelectedSlotInfos();
                    if (!infos.empty()) {
                        // Перемещаем ВСЕ выделенные слоты
                        if (g_overlayRenderer->getSlotMoveCallback()) {
                            for (const auto& info : infos) {
                                g_overlayRenderer->getSlotMoveCallback()(info.sectionIdx, info.slotIdx, dx, dy);
                            }
                        }
                    } else {
                        // Нет выделения — перемещаем все слоты как если бы все были выделены
                        if (g_overlayRenderer)
                        {
                            auto moveCallback = g_overlayRenderer->getSlotMoveCallback();
                            if (moveCallback && g_guiInstance)
                            {
                                std::lock_guard<std::mutex> lock(g_guiInstance->frameMutex_);
                                
                                for (int si = 0; si < static_cast<int>(g_guiInstance->currentFrame_.sections.size()); ++si)
                                {
                                    auto& section = g_guiInstance->currentFrame_.sections[si];
                                    for (int slotIdx = 0; slotIdx < static_cast<int>(section.slots.size()); ++slotIdx)
                                    {
                                        moveCallback(si, slotIdx, dx, dy);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            return 0;
        }

        case WM_RBUTTONDOWN: {
            return 0;
        }

        case WM_MBUTTONDOWN: {
            return 0;
        }

        case WM_MOUSEMOVE: {
            // Окно некликабельно - клики проходят сквозь него
            return 0;
        }

        case WM_KEYDOWN: {
            // F11 - переключение видимости
            if (wParam == VK_F11) {
                if (g_overlayRenderer) {
                    if (g_overlayRenderer->getHWND()) {
                        ShowWindow(hwnd, SW_HIDE);
                    } else {
                        ShowWindow(hwnd, SW_SHOW);
                    }
                }
                return 0;
            }

            // ESC - закрыть оверлей
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hwnd);
                return 0;
            }

            return 0;
        }

        case WM_SIZE: {
            if (wParam == SIZE_MINIMIZED) return 0;
            
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            
            if (g_overlayRenderer && width > 0 && height > 0) {
                g_overlayRenderer->updateWindowSize();
            }
            return 0;
        }

        case WM_DISPLAYCHANGE: {
            // Обрабатываем изменение разрешения монитора
            if (g_overlayRenderer && g_overlayRenderer->getHWND()) {
                HWND hwnd = g_overlayRenderer->getHWND();
                
                // Изменяем размер и позицию окна оверлея
                int newWidth = GetSystemMetrics(SM_CXSCREEN);
                int newHeight = GetSystemMetrics(SM_CYSCREEN);
                
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, newWidth, newHeight,
                            SWP_NOZORDER | SWP_FRAMECHANGED);
                
                // Обновляем размеры в рендерере
                g_overlayRenderer->updateWindowSize();
            }
            return 0;
        }

        case WM_ENTERSIZEMOVE: {
            return 0;
        }

        case WM_EXITSIZEMOVE: {
            if (g_overlayRenderer) {
                g_overlayRenderer->updateWindowSize();
            }
            return 0;
        }

        default: {
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Установка позиции оверлея
// ---------------------------------------------------------------------------
void setOverlayPosition(int x, int y) {
    if (g_overlayRenderer && g_overlayRenderer->getHWND()) {
        SetWindowPos(
            g_overlayRenderer->getHWND(),
            HWND_TOPMOST,
            x, y,
            0, 0,
            SWP_NOSIZE | SWP_NOZORDER
        );
    }
}

// ---------------------------------------------------------------------------
// Установка размера оверлея
// ---------------------------------------------------------------------------
void setOverlaySize(int width, int height) {
    if (g_overlayRenderer && g_overlayRenderer->getHWND()) {
        SetWindowPos(
            g_overlayRenderer->getHWND(),
            nullptr,
            0, 0,
            width, height,
            SWP_NOMOVE | SWP_NOZORDER
        );
    }
}

// ---------------------------------------------------------------------------
// Установка callback для кликов по индикаторам
// ---------------------------------------------------------------------------
void setOverlaySlotClickCallback(SlotClickCallback callback) {
    std::lock_guard<std::mutex> lock(g_overlayMutex);
    if (g_overlayRenderer) {
        g_overlayRenderer->setSlotClickCallback(std::move(callback));
    }
}

// ---------------------------------------------------------------------------
// Установка callback для перемещения индикаторов
// ---------------------------------------------------------------------------
void setOverlaySlotMoveCallback(SlotMoveCallback callback) {
    std::lock_guard<std::mutex> lock(g_overlayMutex);
    if (g_overlayRenderer) {
        g_overlayRenderer->setSlotMoveCallback(std::move(callback));
    }
}

// ---------------------------------------------------------------------------
// Установка выделенного слота
// ---------------------------------------------------------------------------
void setOverlaySelectedSlot(int sectionIdx, int slotIdx, int offsetX, int offsetY) {
    std::lock_guard<std::mutex> lock(g_overlayMutex);
    if (g_overlayRenderer) {
        g_overlayRenderer->setSelectedSlot(sectionIdx, slotIdx);
        // Сохраняем смещение
        g_overlayRenderer->setSelectedSlotOffset(offsetX, offsetY);
    }
}

// ---------------------------------------------------------------------------
// Сброс выделенного слота
// ---------------------------------------------------------------------------
void clearOverlaySelectedSlot() {
    std::lock_guard<std::mutex> lock(g_overlayMutex);
    if (g_overlayRenderer) {
        g_overlayRenderer->clearSelectedSlot();
    }
}

// ---------------------------------------------------------------------------
// Установка списка выделенных слотов
// ---------------------------------------------------------------------------
void setOverlaySelectedSlotInfos(const std::vector<SelectedSlotInfo>& infos) {
    std::lock_guard<std::mutex> lock(g_overlayMutex);
    if (g_overlayRenderer) {
        g_overlayRenderer->setSelectedSlotInfos(infos);
    }
}
