LRESULT GUI::handleMouse(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)hwnd; (void)msg; (void)wParam;
    int clickX = LOWORD(lParam);
    int clickY = HIWORD(lParam);
    
    // Просто проверяем координаты кнопки, но не переключаем здесь
    if (!editMode_)
    {
        int btnX = windowWidth_ - 80;
        int btnY = 8;
        int btnW = 70;
        int btnH = 28;
        
        if (clickX >= btnX && clickX <= btnX + btnW &&
            clickY >= btnY && clickY <= btnY + btnH)
        {
            // Кнопка нажата, но переключаем в handleMouseUp
            return 0;
        }
    }
    
    return 0;
}

LRESULT GUI::handleMouseUp(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)hwnd; (void)msg; (void)wParam;
    int clickX = LOWORD(lParam);
    int clickY = HIWORD(lParam);
    
    // Всегда проверяем кнопку EDIT/SAVE сначала
    int btnX = windowWidth_ - 80;
    int btnY = 8;
    int btnW = 70;
    int btnH = 28;
    
    if (clickX >= btnX && clickX <= btnX + btnW &&
        clickY >= btnY && clickY <= btnY + btnH)
    {
        // Переключаем режим при отпускании кнопки мыши
        toggleEditMode();
        return 0;
    }
    
    if (!editMode_)
    {
        return 0;
    }
    
    // В режиме редактирования обрабатываем клики по цветовым образцам
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
            
            // Инвертируем яркость
            int r = 255 - GetRValue(currentColor);
            int g = 255 - GetGValue(currentColor);
            int b = 255 - GetBValue(currentColor);
            COLORREF newColor = RGB(r, g, b);
            
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
            return 0;
        }
    }
    
    return 0;
}
