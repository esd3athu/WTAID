#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "config/ConfigManager.h"

// ---------------------------------------------------------------------------
// Диалог выбора профиля
// ---------------------------------------------------------------------------
class ProfileDialog
{
public:
    ProfileDialog();
    ~ProfileDialog();

    /**
     * Показать диалог выбора профиля.
     * @param profiles  Список профилей.
     * @param current   Текущий профиль.
     * @return Выбранный профиль или nullptr.
     */
    const Profile* show(HWND parent,
                        const std::vector<std::unique_ptr<Profile>>& profiles,
                        const std::string& current);

    /**
     * Показать диалог создания нового профиля.
     * @return Имя нового профиля или пустая строка.
     */
    std::string showCreate(HWND parent);

private:
    static INT_PTR CALLBACK ProfileDlgProc(HWND hwnd, UINT msg,
                                           WPARAM wParam, LPARAM lParam,
                                           LPARAM userData);
    static INT_PTR CALLBACK CreateProfileDlgProc(HWND hwnd, UINT msg,
                                                 WPARAM wParam, LPARAM lParam,
                                                 LPARAM userData);

    std::string selectedProfile_;
    std::string newProfileName_;
};
