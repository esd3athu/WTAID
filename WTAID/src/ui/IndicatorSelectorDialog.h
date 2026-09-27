#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "config/ConfigManager.h"

// ---------------------------------------------------------------------------
// Диалог выбора видимых индикаторов
// ---------------------------------------------------------------------------
class IndicatorSelectorDialog
{
public:
    IndicatorSelectorDialog();
    ~IndicatorSelectorDialog();

    /**
     * Показать диалог выбора индикаторов.
     * @param parent          Родительское окно.
     * @param indicatorVis    Список видимости индикаторов.
     * @param indicatorNames  Список всех доступных индикаторов.
     * @return true если пользователь нажал OK.
     */
    bool show(HWND parent,
              std::vector<IndicatorVisibility>& indicatorVis,
              const std::vector<std::string>& indicatorNames);

private:
    // DlgProc объявлен в .cpp файле

    std::vector<IndicatorVisibility>* m_indicatorVis;
    std::vector<std::string>* m_indicatorNames;
    HWND m_listBox;
};
