#include "ColorPickerDialog.h"
#include <commctrl.h>
#include <algorithm>
#include <strsafe.h>

#pragma comment(lib, "comctl32.lib")

namespace
{
    COLORREF g_selectedColor = RGB(0, 0, 0);
}

ColorPickerDialog::ColorPickerDialog() : selectedColor_(RGB(0, 0, 0)) {}

ColorPickerDialog::~ColorPickerDialog() {}

bool ColorPickerDialog::showDialog(HWND parentHwnd, COLORREF initialColor, const wchar_t* colorName)
{
    (void)parentHwnd;
    (void)colorName;
    
    initialColor_ = initialColor;
    selectedColor_ = initialColor;
    g_selectedColor = initialColor;

    // Создаём структуру CHOOSECOLOR
    CHOOSECOLORW cc = {};
    cc.lStructSize = sizeof(CHOOSECOLORW);
    cc.hwndOwner = parentHwnd;
    cc.rgbResult = initialColor;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
    
    // Пользовательские цвета (начинаем с initialColor)
    static COLORREF customColors[16] = { initialColor };
    cc.lpCustColors = customColors;
    
    // Показываем стандартный диалог выбора цвета Windows
    if (ChooseColorW(&cc))
    {
        g_selectedColor = cc.rgbResult;
        selectedColor_ = g_selectedColor;
        return true;
    }
    
    return false;
}

COLORREF ColorPickerDialog::getSelectedColor() const
{
    return selectedColor_;
}

void ColorPickerDialog::initControls(HWND hwnd)
{
    (void)hwnd;
}

void ColorPickerDialog::updateColorFromSliders(HWND hwnd)
{
    (void)hwnd;
}

void ColorPickerDialog::updatePreview(HWND hwnd)
{
    (void)hwnd;
}

void ColorPickerDialog::updatePreviewColor(HWND hwnd)
{
    (void)hwnd;
}

void ColorPickerDialog::cancel()
{
    selectedColor_ = initialColor_;
}

HWND ColorPickerDialog::getSliderR() const { return nullptr; }
HWND ColorPickerDialog::getSliderG() const { return nullptr; }
HWND ColorPickerDialog::getSliderB() const { return nullptr; }
