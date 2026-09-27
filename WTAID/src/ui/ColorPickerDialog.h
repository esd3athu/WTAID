#pragma once

#include <windows.h>
#include <string>

// ---------------------------------------------------------------------------
// Диалог выбора цвета - обёртка над стандартным Windows Color Picker
// ---------------------------------------------------------------------------
class ColorPickerDialog
{
public:
    ColorPickerDialog();
    ~ColorPickerDialog();

    // Запрещаем копирование
    ColorPickerDialog(const ColorPickerDialog&) = delete;
    ColorPickerDialog& operator=(const ColorPickerDialog&) = delete;

    /**
     * Показать стандартный диалог выбора цвета Windows.
     * @param parentHwnd Родительское окно
     * @param initialColor Исходный цвет
     * @param colorName Название цвета для отображения
     * @return true если пользователь нажал OK
     */
    bool showDialog(HWND parentHwnd, COLORREF initialColor, const wchar_t* colorName);

    /**
     * Получить выбранный цвет.
     */
    COLORREF getSelectedColor() const;

    /**
     * Получить RGB компоненты выбранного цвета.
     */
    void getRGB(int& r, int& g, int& b) const
    {
        r = GetRValue(selectedColor_);
        g = GetGValue(selectedColor_);
        b = GetBValue(selectedColor_);
    }

    // Внутренние методы (оставлены для совместимости)
    void initControls(HWND hwnd);
    void updateColorFromSliders(HWND hwnd);
    void updatePreview(HWND hwnd);
    void updatePreviewColor(HWND hwnd);
    void cancel();

    // Получение дескрипторов слайдеров (оставлено для совместимости)
    HWND getSliderR() const;
    HWND getSliderG() const;
    HWND getSliderB() const;

private:
    HWND hwnd_ = nullptr;
    HWND hwndParent_ = nullptr;
    COLORREF initialColor_ = RGB(0, 0, 0);
    COLORREF selectedColor_ = RGB(0, 0, 0);
};
