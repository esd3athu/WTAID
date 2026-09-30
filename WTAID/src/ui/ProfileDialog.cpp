#include "ui/ProfileDialog.h"

#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

namespace
{
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

    std::string wideToUtf8(const std::wstring& wstr)
    {
        if (wstr.empty()) return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return "";
        std::string str(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], len, nullptr, nullptr);
        size_t nullPos = str.find('\0');
        if (nullPos != std::string::npos)
            str.resize(nullPos);
        return str;
    }
}

// ---------------------------------------------------------------------------
// ProfileDialog
// ---------------------------------------------------------------------------
ProfileDialog::ProfileDialog()
    : selectedProfile_()
    , newProfileName_()
{
}

ProfileDialog::~ProfileDialog()
{
}

const Profile* ProfileDialog::show(HWND parent,
                                    const std::vector<std::unique_ptr<Profile>>& profiles,
                                    const std::string& current)
{
    // Создаём контекстное меню
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu)
        return nullptr;

    UINT id = 1000;
    for (const auto& p : profiles)
    {
        std::wstring wName = utf8ToWide(p->name);
        AppendMenuW(hMenu, MF_STRING, id, wName.c_str());
        if (p->name == current)
            CheckMenuItem(hMenu, id, MF_CHECKED);
        id++;
    }

    // Добавляем разделитель и "Create New..."
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, static_cast<UINT>(-1), L"Create New Profile...");

    // Показываем меню
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    UINT selected = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                                    cursorPos.x, cursorPos.y, 0, parent, nullptr);

    DestroyMenu(hMenu);

    if (selected >= 1000 && selected < 1000 + profiles.size())
    {
        auto it = profiles.begin();
        std::advance(it, selected - 1000);
        if (it != profiles.end())
        {
            selectedProfile_ = (*it)->name;
            return (*it).get();
        }
    }

    return nullptr;
}

std::string ProfileDialog::showCreate(HWND parent)
{
    // Простой ввод через MessageBox
    wchar_t input[256] = {0};

    // Показываем простой диалог
    HINSTANCE hInst = GetModuleHandleW(nullptr);
    
    // Используем InputBox-подобный подход через диалог
    DLGTEMPLATE* dlg = nullptr;
    (void)dlg; (void)hInst; (void)parent;

    // Простой вариант: используем ShellExecute для создания файла
    // Временное решение - возвращаем "Default"
    newProfileName_ = "Default";
    return newProfileName_;
}
