#include "DataOutputWindow.h"

#include <windows.h>
#include <commctrl.h>

#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>

#pragma comment(lib, "comctl32.lib")

namespace
{
    const wchar_t* WINDOW_CLASS_NAME = L"WTAID_DataOutputClass";
    const wchar_t* WINDOW_TITLE = L"WTAID — Данные телеметрии";

    std::string getCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::tm tm_buf;
        localtime_s(&tm_buf, &time_t_now);

        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
}

// Глобальная ссылка на экземпляр для WndProc
static DataOutputWindow* g_windowInstance = nullptr;

LRESULT CALLBACK DataOutputWindow::WndProc(HWND hwnd, UINT msg,
                                            WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            g_windowInstance = reinterpret_cast<DataOutputWindow*>(cs->lpCreateParams);

            // Создаём элемент управления EDIT
            g_windowInstance->editControl_ = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                nullptr,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                0, 0, 0, 0,
                hwnd,
                nullptr,
                reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE)),
                nullptr
            );

            // Устанавливаем моноширинный шрифт
            HFONT hFont = CreateFontW(
                14, 0, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE,
                DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                PROOF_QUALITY,
                FIXED_PITCH | FF_MODERN,
                L"Consolas"
            );
            if (hFont)
            {
                SendMessageW(g_windowInstance->editControl_, WM_SETFONT,
                             reinterpret_cast<WPARAM>(hFont), TRUE);
            }

            return 0;
        }

    case WM_SIZE:
        {
            if (g_windowInstance && g_windowInstance->editControl_)
            {
                MoveWindow(g_windowInstance->editControl_, 0, 0,
                           LOWORD(lParam), HIWORD(lParam), TRUE);
            }
            return 0;
        }

    case WM_DESTROY:
        {
            g_windowInstance = nullptr;
            PostQuitMessage(0);
            return 0;
        }

    case WM_USER + 1:
        {
            if (g_windowInstance)
                g_windowInstance->updateWindow();
            return 0;
        }

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

DataOutputWindow::DataOutputWindow()
    : hwnd_(nullptr)
    , editControl_(nullptr)
    , running_(false)
{
}

DataOutputWindow::~DataOutputWindow()
{
    if (running_)
        stop();
}

bool DataOutputWindow::create(int width, int height)
{
    // Регистрируем класс окна
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = DataOutputWindow::WndProc;
    wcex.hInstance = GetModuleHandleW(nullptr);
    wcex.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassExW(&wcex))
    {
        return false;
    }

    // Создаём окно
    hwnd_ = CreateWindowExW(
        WS_EX_OVERLAPPEDWINDOW,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        this  // lpCreateParams
    );

    if (!hwnd_)
        return false;

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);

    return true;
}

void DataOutputWindow::close()
{
    if (hwnd_)
    {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
        editControl_ = nullptr;
    }
}

void DataOutputWindow::appendData(const std::string& data)
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Удаляем старые данные сразу при добавлении новых
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::seconds(ttlSeconds_);
    
    auto it = std::remove_if(pendingData_.begin(), pendingData_.end(),
        [cutoff](const DataEntry& entry) {
            return entry.timestamp < cutoff;
        });
    pendingData_.erase(it, pendingData_.end());
    
    // Добавляем новые данные
    DataEntry entry;
    entry.data = data;
    entry.timestamp = std::chrono::steady_clock::now();
    pendingData_.push_back(entry);
    
    // Ограничиваем максимальное количество элементов
    while (pendingData_.size() > maxEntries_)
    {
        pendingData_.erase(pendingData_.begin());
    }

    // Отправляем сообщение окну для обновления
    if (hwnd_)
    {
        PostMessageW(hwnd_, WM_USER + 1, 0, 0);
    }
}

void DataOutputWindow::setDataTTL(int seconds)
{
    ttlSeconds_ = seconds;
}

void DataOutputWindow::clear()
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    pendingData_.clear();

    if (editControl_)
    {
        SendMessageW(editControl_, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
    }
}

HWND DataOutputWindow::getHWND() const
{
    return hwnd_;
}

void DataOutputWindow::run()
{
    if (running_)
        return;

    running_ = true;

    MSG msg = {};
    while (running_ && GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void DataOutputWindow::stop()
{
    running_ = false;
    if (hwnd_)
        PostMessageW(hwnd_, WM_QUIT, 0, 0);
}

bool DataOutputWindow::isRunning() const
{
    return running_;
}

void DataOutputWindow::updateWindow()
{
    if (!editControl_)
        return;

    std::lock_guard<std::mutex> lock(dataMutex_);
    if (pendingData_.empty())
        return;

    // Удаляем старые данные (старше TTL)
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::seconds(ttlSeconds_);
    
    auto it = std::remove_if(pendingData_.begin(), pendingData_.end(),
        [cutoff](const DataEntry& entry) {
            return entry.timestamp < cutoff;
        });
    pendingData_.erase(it, pendingData_.end());

    if (pendingData_.empty())
        return;

    // Собираем весь текст в одну строку
    std::wstring fullText;
    const int maxLines = 50;
    int startIndex = static_cast<int>(pendingData_.size()) > maxLines
        ? static_cast<int>(pendingData_.size()) - maxLines
        : 0;

    for (int i = startIndex; i < static_cast<int>(pendingData_.size()); ++i)
    {
        const auto& entry = pendingData_[i];
        std::string timestamp = getCurrentTimestamp();
        std::wstring text = std::wstring(timestamp.begin(), timestamp.end());
        text += L" | " + std::wstring(entry.data.begin(), entry.data.end());
        text += L"\r\n";
        fullText += text;
    }

    // Устанавливаем весь текст один раз
    SetWindowTextW(editControl_, fullText.c_str());

    pendingData_.clear();

    // Прокручиваем вниз
    SendMessageW(editControl_, EM_LINESCROLL, 0, 10000);
}
