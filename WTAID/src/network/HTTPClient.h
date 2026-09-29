#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <optional>
#include <thread>
#include <atomic>

/**
 * HTTP-клиент для получения телеметрии из War Thunder.
 * Подключается к http://127.0.0.1:8111 и получает JSON-данные.
 */
class HTTPClient
{
public:
    using ResponseCallback = std::function<void(const std::string& responseData)>;

    HTTPClient();
    ~HTTPClient();

    // Запрещаем копирование
    HTTPClient(const HTTPClient&) = delete;
    HTTPClient& operator=(const HTTPClient&) = delete;

    /**
     * Инициализировать клиент с указанным хостом, портом и путем.
     * @param host   Адрес сервера (по умолчанию 127.0.0.1)
     * @param port   Порт сервера (по умолчанию 8111)
     * @param path   Путь к ресурсу (по умолчанию /state)
     * @param timeoutMs  Таймаут запроса в миллисекундах (по умолчанию 1000)
     */
    void initialize(const std::string& host = "127.0.0.1",
                    int port = 8111,
                    const std::string& path = "/state",
                    int timeoutMs = 1000);

    /**
     * Инициализировать второй эндпоинт для данных индикаторов.
     */
    void initializeIndicators(const std::string& host = "127.0.0.1",
                              int port = 8111,
                              const std::string& path = "/indicators",
                              int timeoutMs = 1000);

    /**
     * Установить колбэк для обработки полученных данных.
     */
    void onResponse(ResponseCallback callback);

    /**
     * Установить колбэк для обработки данных индикаторов (из /indicators).
     */
    void onIndicatorsResponse(ResponseCallback callback);

    /**
     * Выполнить один HTTP GET-запрос и вернуть ответ.
     * @return std::string с телом ответа, либо std::nullopt при ошибке.
     */
    std::optional<std::string> fetch();

    /**
     * Выполнять запросы циклически с указанным интервалом.
     * Работает в отдельном потоке.
     * @param intervalMs  Интервал между запросами в миллисекундах.
     */
    void startPolling(int intervalMs = 25);

    /**
     * Остановить фоновый поток опроса.
     */
    void stopPolling();

    /**
     * Запустить опрос второго эндпоинта (/indicators).
     */
    void startPollingIndicators(int intervalMs = 25);

    /**
     * Остановить фоновый поток опроса второго эндпоинта.
     */
    void stopPollingIndicators();

    /**
     * Проверить, запущен ли фоновый поток.
     */
    bool isRunning() const;

    /**
     * Получить последнее сообщение об ошибке.
     */
    std::string getLastError() const;

private:
    std::string host_;
    int port_;
    std::string path_;
    int timeoutMs_;
    ResponseCallback responseCallback_;

    // Второй эндпоинт для /indicators
    std::string indicatorsHost_;
    int indicatorsPort_;
    std::string indicatorsPath_;
    int indicatorsTimeoutMs_;
    ResponseCallback indicatorsResponseCallback_;
    std::thread indicatorsPollingThread_;
    std::atomic<bool> indicatorsRunning_;

    std::string lastError_;

    std::thread pollingThread_;
    std::atomic<bool> running_;

    std::optional<std::string> performRequest();
    std::optional<std::string> performIndicatorsRequest();
};
