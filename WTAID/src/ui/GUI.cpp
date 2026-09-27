#include "GUI.h"
#include "overlay/OverlayWindow.h"
#include "ui/IndicatorSelectorDialog.h"
#include "config/ConfigManager.h"

#include <iostream>

// Глобальный указатель на GUI
GUI* g_guiInstance = nullptr;

#include <commctrl.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <strsafe.h>

#pragma comment(lib, "comctl32.lib")

namespace
{
    const wchar_t* WINDOW_CLASS_NAME = L"WTAID_GUI_Class";
    const wchar_t* WINDOW_TITLE = L"WTAID - War Thunder Telemetry HUD";

    // Названия цветов для диалога выбора
    const wchar_t* COLOR_NAMES[] = {
        L"Main Background",
        L"Panel Background",
        L"Panel Border",
        L"Normal Text",
        L"Label Text",
        L"Value Text",
        L"Alert Text",
        L"Normal Bar",
        L"Warning Bar",
        L"Alert Bar",
        L"Bar Background",
        L"Header Text",
        L"Alert Background",
        L"Alert Text"
    };

    const int MARGIN_X = 16;
    const int MARGIN_Y = 12;
    const int PANEL_PADDING = 10;
    const int SLOT_HEIGHT = 28;
    const int SLOT_LABEL_WIDTH = 120;
    const int BAR_WIDTH = 200;
    const int SECTION_GAP = 12;
    const int HEADER_HEIGHT = 40;
    const int ALERT_BAR_HEIGHT = 24;
    const int OVERLAY_BTN_WIDTH = 90;
    const int OVERLAY_BTN_HEIGHT = 24;
    const int CONFIG_INDICATORS_BTN_WIDTH = 140;

    std::wstring utf8ToWide(const std::string& utf8)
    {
        if (utf8.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (len <= 0) return L"";
        std::wstring wstr(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], len);
        size_t nullPos = wstr.find(L'\0');
        if (nullPos != std::wstring::npos)
            wstr.resize(nullPos);
        return wstr;
    }

    COLORREF getBarColor(const ColorScheme& colors, bool alert)
    {
        return alert ? colors.barAlert : colors.barNormal;
    }
}

LRESULT CALLBACK GUI::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            GUI* self = reinterpret_cast<GUI*>(cs->lpCreateParams);
            if (self) return self->handleCreate(hwnd, msg, wParam, lParam);
            return 0;
        }
    case WM_PAINT:
        {
            GUI* self = reinterpret_cast<GUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (self) return self->handlePaint(hwnd, msg, wParam, lParam);
            break;
        }
    case WM_SIZE:
        {
            GUI* self = reinterpret_cast<GUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (self) return self->handleSize(hwnd, msg, wParam, lParam);
            break;
        }
    case WM_DESTROY:
        {
            GUI* self = reinterpret_cast<GUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (self) return self->handleDestroy(hwnd, msg, wParam, lParam);
            break;
        }
    case WM_TIMER:
        {
            GUI* self = reinterpret_cast<GUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (self) return self->handleTimer(hwnd, msg, wParam, lParam);
            break;
        }
    case WM_LBUTTONDOWN:
        {
            GUI* self = reinterpret_cast<GUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (self) return self->handleMouse(hwnd, msg, wParam, lParam);
            break;
        }
    case WM_LBUTTONUP:
        {
            GUI* self = reinterpret_cast<GUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (self) return self->handleMouseUp(hwnd, msg, wParam, lParam);
            break;
        }
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        {
            // Игнорируем правую кнопку
            return 0;
            break;
        }
    case WM_KEYDOWN:
        {
            GUI* self = reinterpret_cast<GUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (self) return self->handleKeyDown(hwnd, msg, wParam, lParam);
            break;
        }
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            GUI* self = reinterpret_cast<GUI*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (self) {
                mmi->ptMinTrackSize.x = 600;
                mmi->ptMinTrackSize.y = self->calculateMinHeight();
            } else {
                mmi->ptMinTrackSize.x = 600;
                mmi->ptMinTrackSize.y = 500;
            }
            return 0;
        }
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

GUI::GUI() : currentFrame_() {}
GUI::~GUI() { if (running_) stop(); destroyFonts(); destroyBrushes(); }

bool GUI::create(int width, int height)
{
    windowWidth_ = width;
    windowHeight_ = height;

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    if (!hInst) hInst = GetModuleHandleW(NULL);
    if (!hInst) hInst = GetModuleHandleA(nullptr);

    WNDCLASSW wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = GUI::WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassW(&wc))
    {
        DWORD err = GetLastError();
        // ERROR_CLASS_ALREADY_EXISTS is OK - class is already registered from previous run
        if (err != ERROR_CLASS_ALREADY_EXISTS)
        {
            // Class registration failed for another reason
            return false;
        }
        // Class already registered from previous process run - this is fine
        // The class procedure and other properties are the same, so we can proceed
    }

    hwnd_ = CreateWindowW(
        WINDOW_CLASS_NAME, WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, hInst, this);

    if (!hwnd_)
    {
        DWORD err = GetLastError();
        (void)err;
        return false;
    }

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    g_guiInstance = this;
    return true;
}

void GUI::close() { if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; } }

void GUI::updateFrame(const GUIFrame& frame)
{
    std::lock_guard<std::mutex> lock(frameMutex_);
    
    // Сохраняем смещения всех слотов по имени перед обновлением
    struct SlotOffset {
        std::string slotName;
        int offsetX;
        int offsetY;
    };
    std::vector<SlotOffset> allOffsets;
    
    for (const auto& section : currentFrame_.sections) {
        for (const auto& slot : section.slots) {
            SlotOffset offset;
            offset.slotName = slot.label;
            offset.offsetX = slot.offsetX;
            offset.offsetY = slot.offsetY;
            allOffsets.push_back(offset);
        }
    }
    
    currentFrame_ = frame;
    
    // Восстанавливаем смещения для существующих слотов по имени
    for (auto& section : currentFrame_.sections) {
        for (auto& slot : section.slots) {
            for (const auto& offset : allOffsets) {
                if (offset.slotName == slot.label) {
                    slot.offsetX = offset.offsetX;
                    slot.offsetY = offset.offsetY;
                    break;
                }
            }
        }
    }
    
    if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
    // Overlay рендерится в handlePaint из GUI потока
    
    // Применяем состояние выделения
    applySelectionState();
}

HWND GUI::getHWND() const { return hwnd_; }

void GUI::setColorScheme(const ColorScheme& scheme)
{
    colors_ = scheme;
    if (hBrushBg_) DeleteObject(hBrushBg_);
    if (hBrushPanel_) DeleteObject(hBrushPanel_);
    createBrushes();
}

void GUI::setSaveCallback(SaveConfigCallback callback)
{
    saveCallback_ = std::move(callback);
}

void GUI::run()
{
    if (running_) return;
    running_ = true;
    MSG msg = {};
    while (running_ && GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void GUI::stop()
{
    running_ = false;
    if (hwnd_) PostMessageW(hwnd_, WM_QUIT, 0, 0);
}

bool GUI::isRunning() const { return running_; }

LRESULT GUI::handleCreate(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)hwnd; (void)msg; (void)wParam; (void)lParam;
    createFonts();
    createBrushes();
    return 0;
}

LRESULT GUI::handlePaint(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)msg;
    PAINTSTRUCT ps = {};
    HDC hdc = BeginPaint(hwnd, &ps);

    HDC hdcMem = CreateCompatibleDC(hdc);
    if (hdcMem)
    {
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
        if (hBitmap)
        {
            HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(hdcMem, hBitmap));
            renderFrame(hdcMem, width, height);
            BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
            SelectObject(hdcMem, hOldBitmap);
            DeleteObject(hBitmap);
        }
        DeleteDC(hdcMem);
    }
    
    // Рендерим в overlay из GUI потока (после отрисовки окна)
    if (overlayEnabled_) {
        renderToOverlay();
    }
    
    EndPaint(hwnd, &ps);
    return 0;
}

LRESULT GUI::handleSize(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)msg;
    if (wParam == SIZE_MINIMIZED) return 0;
    windowWidth_ = LOWORD(lParam);
    windowHeight_ = HIWORD(lParam);
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
}

LRESULT GUI::handleDestroy(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)msg; (void)wParam; (void)lParam; (void)hwnd;
    running_ = false;
    PostQuitMessage(0);
    return 0;
}

LRESULT GUI::handleTimer(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)msg; (void)wParam; (void)lParam; (void)hwnd;
    return 0;
}

void GUI::toggleEditMode()
{
    if (editMode_)
    {
        // Применяем и сохраняем цвета
        applyEditColors();
    }
    else
    {
        editMode_ = true;
        // Копируем текущие цвета для редактирования
        editColors_ = colors_;
        // Перерисовываем окно
        if (hwnd_)
        {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
    }
}

void GUI::applyEditColors()
{
    colors_ = editColors_;
    if (hBrushBg_) DeleteObject(hBrushBg_);
    if (hBrushPanel_) DeleteObject(hBrushPanel_);
    createBrushes();
    // Перерисовываем окно
    if (hwnd_)
    {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
    // Вызываем колбэк сохранения, если он есть
    if (saveCallback_)
    {
        saveCallback_(colors_);
    }
}

LRESULT GUI::handleMouse(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)hwnd; (void)msg; (void)wParam;
    int clickX = LOWORD(lParam);
    int clickY = HIWORD(lParam);
    
    // Проверяем зажат ли SHIFT
    bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    
    // Сначала проверяем клик по индикатору (до кнопок)
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        
        if (!currentFrame_.valid) return 0;
        if (currentFrame_.sections.empty()) return 0;
        
        // Клики ниже header и выше alert bar
        int contentTop = MARGIN_Y + HEADER_HEIGHT + SECTION_GAP;
        
        // Рассчитываем реальную нижнюю границу контента
        int contentBottom = contentTop;
        const int MIN_SECTION_GAP = 12;
        for (const auto& section : currentFrame_.sections)
        {
            int numSlots = static_cast<int>(section.slots.size());
            int sectionRealHeight = 22 + numSlots * SLOT_HEIGHT + 10;
            contentBottom += sectionRealHeight + MIN_SECTION_GAP;
        }
        
        // Учитываем высоту панели алертов
        contentBottom += ALERT_BAR_HEIGHT;
        
        if (clickY >= contentTop && clickY <= contentBottom)
        {
            int y = contentTop;
            const int MIN_SECTION_GAP = 12;
            
            for (int si = 0; si < static_cast<int>(currentFrame_.sections.size()); ++si)
            {
                auto& section = currentFrame_.sections[si];
                int numSlots = static_cast<int>(section.slots.size());
                int sectionRealHeight = 22 + numSlots * SLOT_HEIGHT + 10;
                
                for (int i = 0; i < numSlots; ++i)
                {
                    int slotY = y + 22 + i * SLOT_HEIGHT;
                    
                    if (clickY >= slotY && clickY <= slotY + SLOT_HEIGHT)
                    {
                        if (shiftDown) {
                            // SHIFT+click — добавить/убрать из группы
                            bool found = false;
                            for (size_t j = 0; j < selectedSlots_.size(); ++j) {
                                if (selectedSlots_[j].sectionIdx == si && selectedSlots_[j].slotIdx == i) {
                                    // Сохраняем смещения в currentFrame_ перед удалением
                                    if (si >= 0 && si < static_cast<int>(currentFrame_.sections.size())) {
                                        auto& section = currentFrame_.sections[si];
                                        if (i >= 0 && i < static_cast<int>(section.slots.size())) {
                                            section.slots[i].offsetX = selectedSlots_[j].offsetX;
                                            section.slots[i].offsetY = selectedSlots_[j].offsetY;
                                        }
                                    }
                                    selectedSlots_.erase(selectedSlots_.begin() + j);
                                    found = true;
                                    break;
                                }
                            }
                            
                            if (!found) {
                                // Берём смещение из currentFrame_ и округляем до 20
                                int offsetX = 0;
                                int offsetY = 0;
                                if (si >= 0 && si < static_cast<int>(currentFrame_.sections.size())) {
                                    auto& section = currentFrame_.sections[si];
                                    if (i >= 0 && i < static_cast<int>(section.slots.size())) {
                                        offsetX = section.slots[i].offsetX;
                                        offsetY = section.slots[i].offsetY;
                                    }
                                }
                                // Округляем до 20
                                offsetX = static_cast<int>(std::round(static_cast<float>(offsetX) / 20.0f) * 20.0f);
                                offsetY = static_cast<int>(std::round(static_cast<float>(offsetY) / 20.0f) * 20.0f);
                                SelectedSlot sel;
                                sel.sectionIdx = si;
                                sel.slotIdx = i;
                                sel.offsetX = offsetX;
                                sel.offsetY = offsetY;
                                selectedSlots_.push_back(sel);
                            }
                        } else {
                            // Простой клик — проверяем есть ли уже выделение
                            bool found = false;
                            for (size_t j = 0; j < selectedSlots_.size(); ++j) {
                                if (selectedSlots_[j].sectionIdx == si && selectedSlots_[j].slotIdx == i) {
                                    // Сохраняем смещения в currentFrame_ перед удалением
                                    if (si >= 0 && si < static_cast<int>(currentFrame_.sections.size())) {
                                        auto& section = currentFrame_.sections[si];
                                        if (i >= 0 && i < static_cast<int>(section.slots.size())) {
                                            section.slots[i].offsetX = selectedSlots_[j].offsetX;
                                            section.slots[i].offsetY = selectedSlots_[j].offsetY;
                                        }
                                    }
                                    // Уже выделен — снимаем выделение
                                    selectedSlots_.erase(selectedSlots_.begin() + j);
                                    found = true;
                                    break;
                                }
                            }
                            
                            if (!found) {
                                // Не выделен — выделяем только этот, берём смещение из currentFrame_
                                selectedSlots_.clear();
                                int offsetX = 0;
                                int offsetY = 0;
                                if (si >= 0 && si < static_cast<int>(currentFrame_.sections.size())) {
                                    auto& section = currentFrame_.sections[si];
                                    if (i >= 0 && i < static_cast<int>(section.slots.size())) {
                                        offsetX = section.slots[i].offsetX;
                                        offsetY = section.slots[i].offsetY;
                                    }
                                }
                                // Округляем до 20
                                offsetX = static_cast<int>(std::round(static_cast<float>(offsetX) / 20.0f) * 20.0f);
                                offsetY = static_cast<int>(std::round(static_cast<float>(offsetY) / 20.0f) * 20.0f);
                                SelectedSlot sel;
                                sel.sectionIdx = si;
                                sel.slotIdx = i;
                                sel.offsetX = offsetX;
                                sel.offsetY = offsetY;
                                selectedSlots_.push_back(sel);
                            }
                        }
                        
                        // Перерисовываем
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                }
                
                y += sectionRealHeight + MIN_SECTION_GAP;
            }
        }
    }
    
    // Проверяем кнопки
    int configIndicatorsBtnX = windowWidth_ - CONFIG_INDICATORS_BTN_WIDTH - OVERLAY_BTN_WIDTH - OVERLAY_BTN_WIDTH - 20;
    int editBtnX = configIndicatorsBtnX + CONFIG_INDICATORS_BTN_WIDTH + 10;
    int overlayBtnX = editBtnX + OVERLAY_BTN_WIDTH + 10;
    int btnY = 20;
    int btnW = OVERLAY_BTN_WIDTH;
    int btnH = OVERLAY_BTN_HEIGHT;
    int configBtnW = CONFIG_INDICATORS_BTN_WIDTH;
    int configBtnH = OVERLAY_BTN_HEIGHT;
    
    // Проверяем Configure Indicators кнопку
    if (clickX >= configIndicatorsBtnX && clickX <= configIndicatorsBtnX + configBtnW &&
        clickY >= btnY && clickY <= btnY + configBtnH)
    {
        return 0; // Кнопка нажата
    }
    
    // Проверяем EDIT кнопку
    if (clickX >= editBtnX && clickX <= editBtnX + btnW &&
        clickY >= btnY && clickY <= btnY + btnH)
    {
        return 0; // Кнопка нажата
    }
    
    // Проверяем OVERLAY кнопку
    if (clickX >= overlayBtnX && clickX <= overlayBtnX + btnW &&
        clickY >= btnY && clickY <= btnY + btnH)
    {
        return 0; // Кнопка нажата
    }
    
    return 0;
}

LRESULT GUI::handleMouseUp(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)hwnd; (void)msg; (void)wParam;
    int clickX = LOWORD(lParam);
    int clickY = HIWORD(lParam);
    
    // Проверяем кнопки
    int configIndicatorsBtnX = windowWidth_ - CONFIG_INDICATORS_BTN_WIDTH - OVERLAY_BTN_WIDTH - OVERLAY_BTN_WIDTH - 20;
    int editBtnX = configIndicatorsBtnX + CONFIG_INDICATORS_BTN_WIDTH + 10;
    int overlayBtnX = editBtnX + OVERLAY_BTN_WIDTH + 10;
    int btnY = 20;
    int btnW = OVERLAY_BTN_WIDTH;
    int btnH = OVERLAY_BTN_HEIGHT;
    int configBtnW = CONFIG_INDICATORS_BTN_WIDTH;
    int configBtnH = OVERLAY_BTN_HEIGHT;
    
    if (clickX >= configIndicatorsBtnX && clickX <= configIndicatorsBtnX + configBtnW &&
        clickY >= btnY && clickY <= btnY + configBtnH)
    {
        // Показываем диалог выбора индикаторов
        showIndicatorSelector();
        return 0;
    }
    
    if (clickX >= editBtnX && clickX <= editBtnX + btnW &&
        clickY >= btnY && clickY <= btnY + btnH)
    {
        // Переключаем режим редактирования
        toggleEditMode();
        return 0;
    }
    
    // Проверяем кнопку OVERLAY
    if (clickX >= overlayBtnX && clickX <= overlayBtnX + btnW &&
        clickY >= btnY && clickY <= btnY + btnH)
    {
        // Переключаем overlay
        setOverlayEnabled(!overlayEnabled_);
        return 0;
    }
    
    // В режиме редактирования проверяем клик по цветовым образцам ПЕРЕД индикаторами
    if (editMode_)
    {
        goto process_color_picker;
    }
    
    // Проверяем клик по индикатору (работает всегда)
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        
        if (!currentFrame_.valid) return 0;
        if (currentFrame_.sections.empty()) return 0;
        if (currentFrame_.sections[0].slots.empty()) return 0;
        
        // Для теста - переключаем первый индикатор при любом клике в области контента
        int contentTop = MARGIN_Y + HEADER_HEIGHT + SECTION_GAP;
        int contentBottom = windowHeight_ - ALERT_BAR_HEIGHT - MARGIN_Y;
        
        if (clickY >= contentTop && clickY <= contentBottom)
        {
            // Переключаем первый индикатор первой секции
            currentFrame_.sections[0].slots[0].selected = 
                !currentFrame_.sections[0].slots[0].selected;
            currentFrame_.sections[0].slots[0].moved = 
                currentFrame_.sections[0].slots[0].selected;
            
            // Перерисовываем
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
    }
    
process_color_picker:
    // В режиме редактирования обрабатываем клики по цветовым образцам
    {
        int startX = windowWidth_ / 2 - 220;
        int startY = 130;
        int swatchSize = 40;
        int labelWidth = 120;
        int colWidth = 240;
        int rowHeight = 45;
        int cols = 2;
        
        for (int i = 0; i < 14; ++i)
        {
            int col = i % cols;
            int row = i / cols;
            
            int swatchX = startX + col * colWidth + labelWidth + 10;
            int swatchY = startY + row * rowHeight + 2;
        
        if (clickX >= swatchX && clickX <= swatchX + swatchSize &&
            clickY >= swatchY && clickY <= swatchY + swatchSize)
        {
            // Получаем текущий цвет
            COLORREF currentColor;
            switch (i) {
                case 0: currentColor = editColors_.bgMain; break;
                case 1: currentColor = editColors_.bgPanel; break;
                case 2: currentColor = editColors_.bgPanelBorder; break;
                case 3: currentColor = editColors_.textNormal; break;
                case 4: currentColor = editColors_.textLabel; break;
                case 5: currentColor = editColors_.textValue; break;
                case 6: currentColor = editColors_.textAlert; break;
                case 7: currentColor = editColors_.barNormal; break;
                case 8: currentColor = editColors_.barWarning; break;
                case 9: currentColor = editColors_.barAlert; break;
                case 10: currentColor = editColors_.barBg; break;
                case 11: currentColor = editColors_.headerText; break;
                case 12: currentColor = editColors_.alertBg; break;
                case 13: currentColor = editColors_.alertText; break;
                default: continue;
            }
            
            // Открываем диалог выбора цвета
            ColorPickerDialog colorPicker;
            if (colorPicker.showDialog(hwnd_, currentColor, COLOR_NAMES[i]))
            {
                // Применяем выбранный цвет
                COLORREF newColor = colorPicker.getSelectedColor();
                switch (i) {
                    case 0: editColors_.bgMain = newColor; break;
                    case 1: editColors_.bgPanel = newColor; break;
                    case 2: editColors_.bgPanelBorder = newColor; break;
                    case 3: editColors_.textNormal = newColor; break;
                    case 4: editColors_.textLabel = newColor; break;
                    case 5: editColors_.textValue = newColor; break;
                    case 6: editColors_.textAlert = newColor; break;
                    case 7: editColors_.barNormal = newColor; break;
                    case 8: editColors_.barWarning = newColor; break;
                    case 9: editColors_.barAlert = newColor; break;
                    case 10: editColors_.barBg = newColor; break;
                    case 11: editColors_.headerText = newColor; break;
                    case 12: editColors_.alertBg = newColor; break;
                    case 13: editColors_.alertText = newColor; break;
                    default: break;
                }
                
                InvalidateRect(hwnd_, nullptr, FALSE);
            }
            
            return 0;
        }
    }
    
    }
    
    return 0;
}

LRESULT GUI::handleKeyDown(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)hwnd; (void)msg; (void)lParam;
    
    // ESC для отмены изменений и выхода из режима редактирования
    if (wParam == VK_ESCAPE && editMode_)
    {
        editMode_ = false;
        // Перерисовываем окно
        if (hwnd_)
        {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;
    }
    
    return 0;
}

void GUI::renderFrame(HDC hdc, int width, int height)
{
    RECT bgRect = {0, 0, width, height};
    HBRUSH hBgBrush = CreateSolidBrush(colors_.bgMain);
    FillRect(hdc, &bgRect, hBgBrush);
    DeleteObject(hBgBrush);

    std::lock_guard<std::mutex> lock(frameMutex_);

    if (!currentFrame_.valid)
    {
        std::wstring wWaiting = utf8ToWide("Waiting for telemetry data...");
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFontMain_));
        SetTextColor(hdc, RGB(100, 120, 140));
        SetBkMode(hdc, TRANSPARENT);
        RECT textRect = {0, 0, width, height};
        DrawTextW(hdc, wWaiting.c_str(), -1, &textRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, hOldFont);
        return;
    }

    int y = MARGIN_Y;
    int usableWidth = width - 2 * MARGIN_X;

    renderVehicleHeader(hdc, currentFrame_.vehicleType, MARGIN_X, y, usableWidth);
    y += HEADER_HEIGHT + SECTION_GAP;

    // Минимальное расстояние между секциями
    const int MIN_SECTION_GAP = 12;
    
    // Рендерим секции с их реальными высотами
    for (const auto& section : currentFrame_.sections)
    {
        // Рассчитываем реальную высоту секции
        int sectionRealHeight = 22 + static_cast<int>(section.slots.size()) * SLOT_HEIGHT + 10;
        
        renderSection(hdc, section, MARGIN_X, y, usableWidth, sectionRealHeight);
        y += sectionRealHeight + MIN_SECTION_GAP;
    }

    renderAlerts(hdc, currentFrame_.activeAlerts,
                 MARGIN_X, height - ALERT_BAR_HEIGHT - MARGIN_Y, usableWidth);
    
    // Если режим редактирования, рисуем панель редактирования
    if (editMode_)
    {
        renderEditMode(hdc, width, height);
    }
}

void GUI::renderVehicleHeader(HDC hdc, const std::string& vehicleType,
                               int x, int y, int width)
{
    RECT panelRect = {x, y, x + width, y + HEADER_HEIGHT};
    HBRUSH hPanelBrush = CreateSolidBrush(colors_.bgPanel);
    HPEN hPen = CreatePen(PS_SOLID, 1, colors_.bgPanelBorder);
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
    HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hPanelBrush));
    RoundRect(hdc, x, y, x + width, y + HEADER_HEIGHT, 6, 6);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    DeleteObject(hPanelBrush);

    if (!vehicleType.empty())
    {
        std::wstring wName = utf8ToWide(vehicleType);
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFontBold_));
        SetTextColor(hdc, colors_.headerText);
        SetBkMode(hdc, TRANSPARENT);
        RECT textRect = {x + PANEL_PADDING, y + 4,
                         x + width - PANEL_PADDING, y + HEADER_HEIGHT - 4};
        DrawTextW(hdc, wName.c_str(), -1, &textRect,
                  DT_SINGLELINE | DT_VCENTER | DT_LEFT);
        SelectObject(hdc, hOldFont);
    }
    
    // Кнопки
    int configIndicatorsBtnX = x + width - CONFIG_INDICATORS_BTN_WIDTH - OVERLAY_BTN_WIDTH - OVERLAY_BTN_WIDTH - 20;
    int editBtnX = configIndicatorsBtnX + CONFIG_INDICATORS_BTN_WIDTH + 10;
    int overlayBtnX = editBtnX + OVERLAY_BTN_WIDTH + 10;
    int btnY = y + 8;
    int btnW = OVERLAY_BTN_WIDTH;
    int btnH = OVERLAY_BTN_HEIGHT;
    int configBtnW = CONFIG_INDICATORS_BTN_WIDTH;
    int configBtnH = OVERLAY_BTN_HEIGHT;
    
    // Кнопка EDIT/SAVE
    COLORREF editBtnColor = editMode_ ? RGB(0, 150, 255) : RGB(80, 80, 90);
    COLORREF editBtnTextColor = editMode_ ? RGB(255, 255, 255) : RGB(200, 200, 200);
    
    RECT editBtnRect = {editBtnX, btnY, editBtnX + btnW, btnY + btnH};
    HBRUSH hEditBtnBrush = CreateSolidBrush(editBtnColor);
    HPEN hEditBtnPen = CreatePen(PS_SOLID, 1, colors_.bgPanelBorder);
    HPEN hOldEditBtnPen = static_cast<HPEN>(SelectObject(hdc, hEditBtnPen));
    HBRUSH hOldEditBtnBrush = static_cast<HBRUSH>(SelectObject(hdc, hEditBtnBrush));
    RoundRect(hdc, editBtnX, btnY, editBtnX + btnW, btnY + btnH, 4, 4);
    SelectObject(hdc, hOldEditBtnBrush);
    SelectObject(hdc, hOldEditBtnPen);
    DeleteObject(hEditBtnPen);
    DeleteObject(hEditBtnBrush);
    
    std::wstring wEditBtnText = editMode_ ? L"SAVE" : L"EDIT";
    
    // Кнопка Configure Indicators
    COLORREF configBtnColor = RGB(60, 100, 160);
    COLORREF configBtnTextColor = RGB(255, 255, 255);
    
    RECT configBtnRect = {configIndicatorsBtnX, btnY, 
                          configIndicatorsBtnX + configBtnW, btnY + configBtnH};
    HBRUSH hConfigBtnBrush = CreateSolidBrush(configBtnColor);
    HPEN hConfigBtnPen = CreatePen(PS_SOLID, 1, colors_.bgPanelBorder);
    HPEN hOldConfigBtnPen = static_cast<HPEN>(SelectObject(hdc, hConfigBtnPen));
    HBRUSH hOldConfigBtnBrush = static_cast<HBRUSH>(SelectObject(hdc, hConfigBtnBrush));
    RoundRect(hdc, configIndicatorsBtnX, btnY, 
              configIndicatorsBtnX + configBtnW, btnY + configBtnH, 4, 4);
    SelectObject(hdc, hOldConfigBtnBrush);
    SelectObject(hdc, hOldConfigBtnPen);
    DeleteObject(hConfigBtnPen);
    DeleteObject(hConfigBtnBrush);
    
    std::wstring wConfigBtnText = L"Configure Indicators";
    HFONT hOldConfigBtnFont = static_cast<HFONT>(SelectObject(hdc, hFontSmall_));
    SetTextColor(hdc, configBtnTextColor);
    SetBkMode(hdc, TRANSPARENT);
    RECT configBtnTextRect = {configIndicatorsBtnX + 8, btnY + 4, 
                              configIndicatorsBtnX + configBtnW - 8, btnY + configBtnH - 4};
    DrawTextW(hdc, wConfigBtnText.c_str(), -1, &configBtnTextRect,
              DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    SelectObject(hdc, hOldConfigBtnFont);
    HFONT hOldBtnFont = static_cast<HFONT>(SelectObject(hdc, hFontSmall_));
    SetTextColor(hdc, editBtnTextColor);
    SetBkMode(hdc, TRANSPARENT);
    RECT editBtnTextRect = {editBtnX + 8, btnY + 4, editBtnX + btnW - 8, btnY + btnH - 4};
    DrawTextW(hdc, wEditBtnText.c_str(), -1, &editBtnTextRect,
              DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    SelectObject(hdc, hOldBtnFont);
    
    // Кнопка OVERLAY
    COLORREF overlayBtnColor = overlayEnabled_ ? RGB(0, 200, 100) : RGB(80, 80, 90);
    COLORREF overlayBtnTextColor = overlayEnabled_ ? RGB(255, 255, 255) : RGB(200, 200, 200);
    
    RECT overlayBtnRect = {overlayBtnX, btnY, overlayBtnX + btnW, btnY + btnH};
    HBRUSH hOverlayBtnBrush = CreateSolidBrush(overlayBtnColor);
    HPEN hOverlayBtnPen = CreatePen(PS_SOLID, 1, colors_.bgPanelBorder);
    HPEN hOldOverlayBtnPen = static_cast<HPEN>(SelectObject(hdc, hOverlayBtnPen));
    HBRUSH hOldOverlayBtnBrush = static_cast<HBRUSH>(SelectObject(hdc, hOverlayBtnBrush));
    RoundRect(hdc, overlayBtnX, btnY, overlayBtnX + btnW, btnY + btnH, 4, 4);
    SelectObject(hdc, hOldOverlayBtnBrush);
    SelectObject(hdc, hOldOverlayBtnPen);
    DeleteObject(hOverlayBtnPen);
    DeleteObject(hOverlayBtnBrush);
    
    std::wstring wOverlayBtnText = overlayEnabled_ ? L"ON" : L"OFF";
    HFONT hOldOverlayBtnFont = static_cast<HFONT>(SelectObject(hdc, hFontSmall_));
    SetTextColor(hdc, overlayBtnTextColor);
    SetBkMode(hdc, TRANSPARENT);
    RECT overlayBtnTextRect = {overlayBtnX + 8, btnY + 4, overlayBtnX + btnW - 8, btnY + btnH - 4};
    DrawTextW(hdc, wOverlayBtnText.c_str(), -1, &overlayBtnTextRect,
              DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    SelectObject(hdc, hOldOverlayBtnFont);
}

void GUI::renderSection(HDC hdc, const IndicatorSection& section,
                        int x, int y, int width, int sectionHeight)
{
    HBRUSH hPanelBrush = CreateSolidBrush(colors_.bgPanel);
    HPEN hPen = CreatePen(PS_SOLID, 1, colors_.bgPanelBorder);
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
    HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hPanelBrush));
    
    // Рассчитываем реальную высоту секции
    int sectionRealHeight = 22 + static_cast<int>(section.slots.size()) * SLOT_HEIGHT + 10;
    RoundRect(hdc, x, y, x + width, y + sectionRealHeight, 6, 6);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    DeleteObject(hPanelBrush);

    std::wstring wTitle = utf8ToWide(section.title);
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFontSmall_));
    SetTextColor(hdc, colors_.headerText);
    SetBkMode(hdc, TRANSPARENT);
    RECT titleRect = {x + PANEL_PADDING, y + 3,
                      x + width - PANEL_PADDING, y + 18};
    DrawTextW(hdc, wTitle.c_str(), -1, &titleRect,
              DT_SINGLELINE | DT_TOP | DT_LEFT);
    SelectObject(hdc, hOldFont);

    int slotY = y + 22;
    int numSlots = static_cast<int>(section.slots.size());

    for (int i = 0; i < numSlots; ++i)
    {
        renderIndicatorSlot(hdc, section.slots[i], x, slotY + i * SLOT_HEIGHT, width);
    }
}

void GUI::renderIndicatorSlot(HDC hdc, const IndicatorSlot& slot,
                               int x, int y, int width)
{
    // Рисуем рамку выделения
    if (slot.selected)
    {
        RECT selRect = {x, y, x + width - 20, y + SLOT_HEIGHT};
        HPEN hSelPen = CreatePen(PS_SOLID, 2, RGB(0, 200, 100));
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hSelPen));
        HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(HOLLOW_BRUSH)));
        Rectangle(hdc, selRect.left, selRect.top, selRect.right, selRect.bottom);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hSelPen);
    }
    
    std::wstring wLabel = utf8ToWide(slot.label);
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFontMain_));
    SetTextColor(hdc, colors_.textLabel);
    SetBkMode(hdc, TRANSPARENT);
    RECT labelRect = {x + PANEL_PADDING, y + 2,
                      x + PANEL_PADDING + SLOT_LABEL_WIDTH, y + SLOT_HEIGHT - 2};
    DrawTextW(hdc, wLabel.c_str(), -1, &labelRect,
              DT_SINGLELINE | DT_VCENTER | DT_LEFT);

    int barX = x + PANEL_PADDING + SLOT_LABEL_WIDTH + 12;
    int barY = y + 6;
    int barW = BAR_WIDTH;
    int barH = SLOT_HEIGHT - 12;

    RECT barBgRect = {barX, barY, barX + barW, barY + barH};
    HBRUSH hBarBgBrush = CreateSolidBrush(colors_.barBg);
    FillRect(hdc, &barBgRect, hBarBgBrush);
    DeleteObject(hBarBgBrush);

    float clampedFrac = slot.fraction;
    if (clampedFrac < 0.0f) clampedFrac = 0.0f;
    if (clampedFrac > 1.0f) clampedFrac = 1.0f;
    if (clampedFrac > 0.001f)
    {
        int fillW = static_cast<int>(barW * clampedFrac);
        RECT barFillRect = {barX, barY, barX + fillW, barY + barH};
        COLORREF barColor = getBarColor(colors_, slot.alert);
        HBRUSH hBarFillBrush = CreateSolidBrush(barColor);
        FillRect(hdc, &barFillRect, hBarFillBrush);
        DeleteObject(hBarFillBrush);
    }

    HPEN hPen = CreatePen(PS_SOLID, 1, colors_.bgPanelBorder);
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
    SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, barX, barY, barX + barW, barY + barH);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    std::wstring wValue = utf8ToWide(slot.value);
    int valueX = barX + barW + 10;
    RECT valueRect = {valueX, y + 2,
                      x + width - PANEL_PADDING, y + SLOT_HEIGHT - 2};
    COLORREF valueColor = slot.alert ? colors_.textAlert : colors_.textValue;
    SetTextColor(hdc, valueColor);
    if (slot.alert) SelectObject(hdc, hFontBold_);
    DrawTextW(hdc, wValue.c_str(), -1, &valueRect,
              DT_SINGLELINE | DT_VCENTER | DT_LEFT);
    SelectObject(hdc, hFontMain_);
    SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
}

void GUI::renderAlerts(HDC hdc, const std::vector<std::string>& alerts,
                        int x, int y, int width)
{
    if (alerts.empty()) return;

    RECT alertRect = {x, y, x + width, y + ALERT_BAR_HEIGHT};
    HBRUSH hAlertBrush = CreateSolidBrush(colors_.alertBg);
    FillRect(hdc, &alertRect, hAlertBrush);
    DeleteObject(hAlertBrush);

    int currentX = x + PANEL_PADDING;
    int maxAlerts = std::min<int>(alerts.size(), 5);

    for (int i = 0; i < maxAlerts; ++i)
    {
        std::wstring wAlert = utf8ToWide(alerts[i]);
        std::wstring wPrefix = L"\xe2\x9a\xa0 ";
        std::wstring wFull = wPrefix + wAlert;

        RECT tempRect = {0, 0, 0, 0};
        DrawTextW(hdc, wFull.c_str(), -1, &tempRect,
                  DT_CALCRECT | DT_SINGLELINE);

        int badgeWidth = tempRect.right - tempRect.left + 20;
        if (currentX + badgeWidth > x + width) break;

        RECT badgeRect = {currentX, y + 2,
                          currentX + badgeWidth, y + ALERT_BAR_HEIGHT - 2};
        HBRUSH hBadgeBrush = CreateSolidBrush(
            RGB(GetRValue(colors_.alertBg) + 20,
                GetGValue(colors_.alertBg) + 10,
                GetBValue(colors_.alertBg)));
        HPEN hPen = CreatePen(PS_SOLID, 1, colors_.alertText);
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
        HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBadgeBrush));
        RoundRect(hdc, badgeRect.left, badgeRect.top,
                  badgeRect.right, badgeRect.bottom, 4, 4);
        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        DeleteObject(hBadgeBrush);

        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFontBold_));
        SetTextColor(hdc, colors_.alertText);
        SetBkMode(hdc, TRANSPARENT);
        RECT textRect = {currentX + 10, y + 3,
                         currentX + badgeWidth - 10, y + ALERT_BAR_HEIGHT - 3};
        DrawTextW(hdc, wAlert.c_str(), -1, &textRect,
                  DT_SINGLELINE | DT_VCENTER | DT_CENTER);
        SelectObject(hdc, hOldFont);

        currentX += badgeWidth + 6;
    }
}

void GUI::renderEditMode(HDC hdc, int width, int height)
{
    (void)height;
    
    // Полупрозрачная панель поверх контента
    RECT panelRect = {0, 50, width, height - 100};
    HBRUSH hPanelBrush = CreateSolidBrush(RGB(25, 30, 38));
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 150, 255));
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
    HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hPanelBrush));
    SetBkMode(hdc, TRANSPARENT);
    
    // Рисуем панель с закруглёнными углами
    RoundRect(hdc, panelRect.left, panelRect.top, 
              panelRect.right, panelRect.bottom, 8, 8);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    DeleteObject(hPanelBrush);
    
    // Заголовок
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFontBold_));
    SetTextColor(hdc, RGB(100, 200, 255));
    RECT titleRect = {20, 60, width - 20, 90};
    DrawTextW(hdc, L"Color Scheme Editor", -1, &titleRect,
              DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    
    // Подсказка
    RECT hintRect = {20, 95, width - 20, 115};
    SetTextColor(hdc, RGB(150, 170, 190));
    DrawTextW(hdc, L"Click on color swatch to invert | ESC to save", -1, &hintRect,
              DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    
    // Цветовые образцы
    const char* colorNames[] = {
        "bgMain", "bgPanel", "bgPanelBorder", "textNormal", "textLabel",
        "textValue", "textAlert", "barNormal", "barWarning", "barAlert",
        "barBg", "headerText", "alertBg", "alertText"
    };
    
    COLORREF colorValues[] = {
        editColors_.bgMain, editColors_.bgPanel, editColors_.bgPanelBorder, 
        editColors_.textNormal, editColors_.textLabel, editColors_.textValue, 
        editColors_.textAlert, editColors_.barNormal, editColors_.barWarning, 
        editColors_.barAlert, editColors_.barBg, editColors_.headerText, 
        editColors_.alertBg, editColors_.alertText
    };
    
    int cols = 2;
    int rows = 7;
    int swatchSize = 40;
    int labelWidth = 120;
    int startX = width / 2 - 220;
    int startY = 130;
    int rowHeight = 45;
    int colWidth = 240;
    
    for (int i = 0; i < 14; ++i)
    {
        int col = i % cols;
        int row = i / cols;
        
        int x = startX + col * colWidth;
        int y = startY + row * rowHeight;
        
        // Название цвета
        std::wstring wName = utf8ToWide(colorNames[i]);
        RECT nameRect = {x, y, x + labelWidth, y + 20};
        SetTextColor(hdc, RGB(140, 155, 170));
        HFONT hOldNameFont = static_cast<HFONT>(SelectObject(hdc, hFontSmall_));
        DrawTextW(hdc, wName.c_str(), -1, &nameRect,
                  DT_SINGLELINE | DT_TOP | DT_LEFT);
        SelectObject(hdc, hOldNameFont);
        
        // RGB значение
        int r = GetRValue(colorValues[i]);
        int g = GetGValue(colorValues[i]);
        int b = GetBValue(colorValues[i]);
        
        char rgbStr[32];
        sprintf_s(rgbStr, "RGB(%d, %d, %d)", r, g, b);
        std::wstring wRGB = utf8ToWide(rgbStr);
        RECT rgbRect = {x, y + 18, x + labelWidth, y + 35};
        SetTextColor(hdc, RGB(100, 120, 140));
        HFONT hOldRGBFont = static_cast<HFONT>(SelectObject(hdc, hFontSmall_));
        DrawTextW(hdc, wRGB.c_str(), -1, &rgbRect,
                  DT_SINGLELINE | DT_TOP | DT_LEFT);
        SelectObject(hdc, hOldRGBFont);
        
        // Цветовой образец
        RECT swatchRect = {x + labelWidth + 10, y + 2, 
                           x + labelWidth + 10 + swatchSize, y + 2 + swatchSize};
        HBRUSH hSwatchBrush = CreateSolidBrush(colorValues[i]);
        HPEN hSwatchPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        HPEN hOldSwatchPen = static_cast<HPEN>(SelectObject(hdc, hSwatchPen));
        HBRUSH hOldSwatchBrush = static_cast<HBRUSH>(SelectObject(hdc, hSwatchBrush));
        SetRect(&swatchRect, swatchRect.left, swatchRect.top,
                swatchRect.right, swatchRect.bottom);
        FillRect(hdc, &swatchRect, hSwatchBrush);
        Rectangle(hdc, swatchRect.left, swatchRect.top,
                  swatchRect.right, swatchRect.bottom);
        SelectObject(hdc, hOldSwatchBrush);
        SelectObject(hdc, hOldSwatchPen);
        DeleteObject(hSwatchPen);
        DeleteObject(hSwatchBrush);
    }
    
    SelectObject(hdc, hOldFont);
}

void GUI::createFonts()
{
    hFontMain_ = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        PROOF_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    hFontBold_ = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        PROOF_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    hFontSmall_ = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        PROOF_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
}

void GUI::destroyFonts()
{
    if (hFontMain_) { DeleteObject(hFontMain_); hFontMain_ = nullptr; }
    if (hFontBold_) { DeleteObject(hFontBold_); hFontBold_ = nullptr; }
    if (hFontSmall_) { DeleteObject(hFontSmall_); hFontSmall_ = nullptr; }
}

void GUI::createBrushes()
{
    hBrushBg_ = CreateSolidBrush(colors_.bgMain);
    hBrushPanel_ = CreateSolidBrush(colors_.bgPanel);
}

void GUI::destroyBrushes()
{
    if (hBrushBg_) { DeleteObject(hBrushBg_); hBrushBg_ = nullptr; }
    if (hBrushPanel_) { DeleteObject(hBrushPanel_); hBrushPanel_ = nullptr; }
}

// ---------------------------------------------------------------------------
// Применение состояния выделения
// ---------------------------------------------------------------------------
void GUI::applySelectionState()
{
    // Сбрасываем все selected
    for (auto& section : currentFrame_.sections) {
        for (auto& slot : section.slots) {
            slot.selected = false;
        }
    }
    
    // Синхронизируем смещения и применяем выделение
    for (const auto& sel : selectedSlots_) {
        if (sel.sectionIdx >= 0 && sel.sectionIdx < static_cast<int>(currentFrame_.sections.size())) {
            auto& section = currentFrame_.sections[sel.sectionIdx];
            if (sel.slotIdx >= 0 && sel.slotIdx < static_cast<int>(section.slots.size())) {
                section.slots[sel.slotIdx].selected = true;
                section.slots[sel.slotIdx].offsetX = sel.offsetX;
                section.slots[sel.slotIdx].offsetY = sel.offsetY;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Расчёт минимальной высоты окна
// ---------------------------------------------------------------------------
int GUI::calculateMinHeight() const
{
    int h = MARGIN_Y + HEADER_HEIGHT + SECTION_GAP;
    
    for (const auto& section : currentFrame_.sections) {
        int numSlots = static_cast<int>(section.slots.size());
        int sectionHeight = 22 + numSlots * SLOT_HEIGHT + 10;
        h += sectionHeight + SECTION_GAP;
    }
    
    // Добавляем высоту панели алертов
    h += ALERT_BAR_HEIGHT + MARGIN_Y;
    
    // Добавляем высоту панели редактирования, если активна
    if (editMode_) {
        h += 120; // примерная высота панели редактирования
    }
    
    return h;
}

// ---------------------------------------------------------------------------
// Методы для overlay
// ---------------------------------------------------------------------------
void GUI::setOverlayEnabled(bool enabled)
{
    overlayEnabled_ = enabled;
    
    if (enabled)
    {
        // Создаём overlay окно
        if (!createOverlayWindow(overlayWidth_, overlayHeight_))
        {
            overlayEnabled_ = false;
        }
    }
    else
    {
        // При выключении оверлея сохраняем смещения
        if (profileManager_)
        {
            saveOverlayOffsets();
            profileManager_->save(ConfigManager::getDefaultPath());
        }
        
        // Уничтожаем overlay окно
        destroyOverlayWindow();
    }
}

void GUI::renderToOverlay()
{
    if (!overlayEnabled_) return;
    
    std::lock_guard<std::mutex> lock(frameMutex_);
    
    // Устанавливаем callback для перемещения слотов
    setOverlaySlotMoveCallback([this](int sectionIdx, int slotIdx, int dx, int dy) {
        // Сохраняем смещения в currentFrame_
        if (sectionIdx >= 0 && sectionIdx < static_cast<int>(currentFrame_.sections.size())) {
            auto& section = currentFrame_.sections[sectionIdx];
            if (slotIdx >= 0 && slotIdx < static_cast<int>(section.slots.size())) {
                // Восстанавливаем смещение из кэша
                std::string slotName = section.slots[slotIdx].label;
                if (!slotName.empty() && slotName.back() == ':')
                    slotName.pop_back();
                
                auto it = cachedOverlayOffsets_.find(slotName);
                if (it != cachedOverlayOffsets_.end()) {
                    section.slots[slotIdx].offsetX = it->second.first;
                    section.slots[slotIdx].offsetY = it->second.second;
                }
                
                section.slots[slotIdx].offsetX += dx;
                section.slots[slotIdx].offsetY += dy;
                
                // Обновляем кэш
                cachedOverlayOffsets_[slotName] = {section.slots[slotIdx].offsetX, section.slots[slotIdx].offsetY};
            }
        }
        // Также обновляем selectedSlots_ если слот выбран
        for (auto& sel : selectedSlots_) {
            if (sel.sectionIdx == sectionIdx && sel.slotIdx == slotIdx) {
                sel.offsetX += dx;
                sel.offsetY += dy;
                break;
            }
        }
        
        // Сохраняем смещения в профиль сразу
        if (slotIdx >= 0 && sectionIdx >= 0 && sectionIdx < static_cast<int>(currentFrame_.sections.size())) {
            auto& section = currentFrame_.sections[sectionIdx];
            if (slotIdx < static_cast<int>(section.slots.size())) {
                std::string slotName = section.slots[slotIdx].label;
                if (!slotName.empty() && slotName.back() == ':')
                    slotName.pop_back();
                
                if (profileManager_)
                {
                    Profile* activeProfile = const_cast<Profile*>(profileManager_->getActiveProfile());
                    if (activeProfile)
                    {
                        for (auto& iv : activeProfile->indicatorVisibility)
                        {
                            if (iv.name == slotName)
                            {
                                iv.overlayOffsetX = section.slots[slotIdx].offsetX;
                                iv.overlayOffsetY = section.slots[slotIdx].offsetY;
                                break;
                            }
                        }
                        profileManager_->save(ConfigManager::getDefaultPath());
                    }
                }
            }
        }
    });
    
    // Передаём все выделенные слоты в overlay
    std::vector<SelectedSlotInfo> infos;
    for (const auto& sel : selectedSlots_) {
        SelectedSlotInfo info;
        info.sectionIdx = sel.sectionIdx;
        info.slotIdx = sel.slotIdx;
        info.offsetX = sel.offsetX;
        info.offsetY = sel.offsetY;
        infos.push_back(info);
    }
    setOverlaySelectedSlotInfos(infos);
    
    updateOverlayFrame(currentFrame_, colors_);
}

void GUI::showIndicatorSelector()
{
    if (!profileManager_) return;
    
    const Profile* activeProfile = profileManager_->getActiveProfile();
    if (!activeProfile) return;
    
    // Получаем список всех доступных индикаторов
    std::vector<IndicatorVisibility> indicatorVis = 
        ProfileManager::getDefaultIndicatorVisibility();
    
    // Копируем текущие настройки из активного профиля
    for (const auto& iv : activeProfile->indicatorVisibility)
    {
        for (auto& iv2 : indicatorVis)
        {
            if (iv2.name == iv.name)
            {
                iv2.visible = iv.visible;
                iv2.section = iv.section;
                break;
            }
        }
    }
    
    // Получаем список всех имён индикаторов
    std::vector<std::string> indicatorNames;
    for (const auto& iv : indicatorVis)
    {
        indicatorNames.push_back(iv.name);
    }
    
    // Показываем диалог
    IndicatorSelectorDialog dialog;
    if (dialog.show(hwnd_, indicatorVis, indicatorNames))
    {
        // Сохраняем новые настройки в активный профиль
        Profile* profile = const_cast<Profile*>(activeProfile);
        profile->indicatorVisibility = indicatorVis;
        
        // Сохраняем конфигурацию
        if (saveCallback_)
        {
            saveCallback_(colors_);
        }
        
        // Перерисовываем окно
        if (hwnd_)
        {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
    }
}

// ---------------------------------------------------------------------------
// Сохранение смещений оверлея в профиль
// ---------------------------------------------------------------------------
void GUI::saveOverlayOffsets()
{
    if (!profileManager_) {
        std::cerr << "[Overlay] saveOverlayOffsets: no profileManager" << std::endl;
        return;
    }
    
    Profile* profile = const_cast<Profile*>(profileManager_->getActiveProfile());
    if (!profile) {
        std::cerr << "[Overlay] saveOverlayOffsets: no active profile" << std::endl;
        return;
    }

    // Сбрасываем все смещения
    for (auto& iv : profile->indicatorVisibility)
    {
        iv.overlayOffsetX = 0;
        iv.overlayOffsetY = 0;
    }

    // Применяем смещения из currentFrame_ по имени индикатора
    for (const auto& section : currentFrame_.sections)
    {
        for (const auto& slot : section.slots)
        {
            for (auto& iv : profile->indicatorVisibility)
            {
                // Сравниваем label без двоеточия
                std::string slotName = slot.label;
                if (!slotName.empty() && slotName.back() == ':')
                    slotName.pop_back();
                
                if (iv.name == slotName)
                {
                    iv.overlayOffsetX = slot.offsetX;
                    iv.overlayOffsetY = slot.offsetY;
                    break;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Инициализация кэша смещений оверлея из профиля
// ---------------------------------------------------------------------------
void GUI::initOverlayOffsetCache(ProfileManager& pm)
{
    Profile* profile = const_cast<Profile*>(pm.getActiveProfile());
    if (!profile) return;
    
    for (const auto& iv : profile->indicatorVisibility)
    {
        if (iv.overlayOffsetX != 0 || iv.overlayOffsetY != 0)
        {
            cachedOverlayOffsets_[iv.name] = {iv.overlayOffsetX, iv.overlayOffsetY};
        }
    }
}
