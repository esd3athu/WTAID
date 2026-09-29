#include "ui/IndicatorSelectorDialog.h"

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <atomic>

#pragma comment(lib, "comctl32.lib")

#define IDC_LIST_INDICATORS 1001
#define IDC_BTN_OK 1002
#define IDC_BTN_CANCEL 1003

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

    struct DlgData
    {
        std::vector<IndicatorVisibility>* indicatorVis;
        const std::vector<std::string>* indicatorNames;
        std::atomic<bool>* result;
        bool initialized;
        HWND listBox;
        std::vector<HWND> checkBoxes;
    };

    INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg,
                             WPARAM wParam, LPARAM lParam,
                             LPARAM userData)
    {
        DlgData* dlgData = nullptr;

        switch (msg)
        {
        case WM_CREATE:
            {
                CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                dlgData = reinterpret_cast<DlgData*>(cs->lpCreateParams);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dlgData));
                dlgData->initialized = false;
                return 0;
            }

        case WM_INITDIALOG:
            {
                dlgData = reinterpret_cast<DlgData*>(userData);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dlgData));
                break;
            }

        case WM_SIZE:
            {
                dlgData = reinterpret_cast<DlgData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                if (dlgData && !dlgData->initialized)
                {
                    HINSTANCE hInst = GetModuleHandleW(nullptr);

                    // Static text
                    CreateWindowExW(0, L"STATIC", 
                        L"Check/uncheck indicators to show/hide:",
                        WS_VISIBLE | WS_CHILD | SS_LEFT,
                        10, 10, 300, 15, hwnd, nullptr, hInst, nullptr);

                    // ListBox (скрытый, только для хранения индексов)
                    dlgData->listBox = CreateWindowExW(
                        0,
                        L"LISTBOX",
                        nullptr,
                        WS_VISIBLE | WS_CHILD | LBS_STANDARD,
                        -1000, 0, 10, 10,
                        hwnd,
                        (HMENU)(LONG_PTR)IDC_LIST_INDICATORS,
                        hInst,
                        nullptr
                    );

                    dlgData->checkBoxes.clear();

                    for (size_t i = 0; i < dlgData->indicatorNames->size(); ++i)
                    {
                        const std::string& name = (*dlgData->indicatorNames)[i];
                        std::wstring wName = utf8ToWide(name);

                        // Находим текущую видимость
                        bool visible = true;
                        for (const auto& iv : *dlgData->indicatorVis)
                        {
                            if (iv.name == name)
                            {
                                visible = iv.visible;
                                break;
                            }
                        }

                        // Создаём чекбокс
                        HWND hCheckBox = CreateWindowW(
                            L"BUTTON",
                            wName.c_str(),
                            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,
                            10, 30 + static_cast<INT>(i * 25),
                            300, 20,
                            hwnd,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(i)),
                            hInst,
                            nullptr
                        );

                        // Устанавливаем состояние чекбокса
                        SendMessageW(hCheckBox, BM_SETCHECK, visible ? BST_CHECKED : BST_UNCHECKED, 0);

                        dlgData->checkBoxes.push_back(hCheckBox);
                    }

                    // OK кнопка
                    CreateWindowW(L"BUTTON", L"OK",
                        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                        100, 490, 80, 25, hwnd,
                        (HMENU)(LONG_PTR)IDC_BTN_OK,
                        hInst, nullptr);

                    // Cancel кнопка
                    CreateWindowW(L"BUTTON", L"Cancel",
                        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        190, 490, 80, 25, hwnd,
                        (HMENU)(LONG_PTR)IDC_BTN_CANCEL,
                        hInst, nullptr);

                    dlgData->initialized = true;
                }
                return 0;
            }

        case WM_COMMAND:
            {
                dlgData = reinterpret_cast<DlgData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                int ctrlId = LOWORD(wParam);
                int notifyCode = HIWORD(wParam);

                if (!dlgData) return 0;

                // Обработка клика по чекбоксу
                if (notifyCode == BN_CLICKED && ctrlId >= 0 && ctrlId < static_cast<int>(dlgData->checkBoxes.size()))
                {
                    int index = ctrlId;
                    if (index >= 0 && index < static_cast<int>(dlgData->indicatorNames->size()))
                    {
                        const std::string& name = (*dlgData->indicatorNames)[index];
                        for (auto& iv : *dlgData->indicatorVis)
                        {
                            if (iv.name == name)
                            {
                                // Переключаем видимость
                                iv.visible = !iv.visible;
                                break;
                            }
                        }
                    }
                    return 0;
                }

                if (ctrlId == IDC_BTN_OK)
                {
                    *dlgData->result = true;
                    DestroyWindow(hwnd);
                    return 0;
                }

                if (ctrlId == IDC_BTN_CANCEL)
                {
                    *dlgData->result = false;
                    DestroyWindow(hwnd);
                    return 0;
                }
            }
            return 0;

        case WM_CLOSE:
            {
                dlgData = reinterpret_cast<DlgData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                if (dlgData)
                {
                    *dlgData->result = false;
                }
                DestroyWindow(hwnd);
                return 0;
            }

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return 0;
    }
}

// ---------------------------------------------------------------------------
// IndicatorSelectorDialog
// ---------------------------------------------------------------------------
IndicatorSelectorDialog::IndicatorSelectorDialog()
    : m_indicatorVis(nullptr)
    , m_indicatorNames(nullptr)
    , m_listBox(nullptr)
{
}

IndicatorSelectorDialog::~IndicatorSelectorDialog()
{
}

bool IndicatorSelectorDialog::show(HWND parent,
                                   std::vector<IndicatorVisibility>& indicatorVis,
                                   const std::vector<std::string>& indicatorNames)
{
    DlgData dlgData;
    dlgData.indicatorVis = &indicatorVis;
    dlgData.indicatorNames = &indicatorNames;
    dlgData.initialized = false;

    std::atomic<bool> result(false);
    dlgData.result = &result;

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    
    const wchar_t* className = L"WTAID_IndicatorDialog";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = reinterpret_cast<WNDPROC>(DlgProc);
    wc.hInstance = hInst;
    wc.lpszClassName = className;
    wc.hCursor = static_cast<HCURSOR>(LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW)));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        return false;
    }

    HWND hwndDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        className,
        L"Select Indicators",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        200, 200, 350, 550,
        parent,
        nullptr,
        hInst,
        &dlgData
    );

    if (!hwndDlg) return false;

    ShowWindow(hwndDlg, SW_SHOW);
    UpdateWindow(hwndDlg);
    
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        if (!IsWindow(hwndDlg))
            break;
            
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return result.load();
}
