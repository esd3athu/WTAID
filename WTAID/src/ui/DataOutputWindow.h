#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>

/**
 * Структура для хранения данных с временной меткой.
 */
struct DataEntry
{
    std::string data;
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * Окно для вывода сырых данных телеметрии.
 * Использует стандартный Win32 API для создания отдельного окна.
 */
class DataOutputWindow
{
public:
    DataOutputWindow();
    ~DataOutputWindow();

    // Запрещаем копирование
    DataOutputWindow(const DataOutputWindow&) = delete;
    DataOutputWindow& operator=(const DataOutputWindow&) = delete;

    /**
     * Создать и показать окно.
     * @param width   Ширина окна в пикселях.
     * @param height  Высота окна в пикселях.
     * @return true если окно успешно создано.
     */
    bool create(int width = 800, int height = 600);

    /**
     * Закрыть окно.
     */
    void close();

    /**
     * Добавить новые данные в вывод.
     * @param data  Строка с данными для отображения.
     */
    void appendData(const std::string& data);

    /**
     * Установить время жизни данных в секундах.
     * Данные старше этого времени будут автоматически удалены.
     * @param seconds  Время жизни в секундах (по умолчанию 5)
     */
    void setDataTTL(int seconds = 5);

    /**
     * Очистить вывод.
     */
    void clear();

    /**
     * Получить дескриптор окна.
     */
    HWND getHWND() const;

    /**
     * Запустить цикл обработки сообщений (блокирующий).
     */
    void run();

    /**
     * Завершить цикл обработки сообщений.
     */
    void stop();

    /**
     * Проверить, запущен ли цикл сообщений.
     */
    bool isRunning() const;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND hwnd_;
    HWND editControl_;
    std::mutex dataMutex_;
    std::vector<DataEntry> pendingData_;
    int ttlSeconds_ = 5;  // время жизни данных в секундах
    size_t maxEntries_ = 100;  // максимальное количество элементов в очереди

    bool running_;

    void updateWindow();
};
