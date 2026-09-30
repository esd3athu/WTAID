#include "HTTPClient.h"

#include <windows.h>
#include <winhttp.h>

#include <iostream>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

namespace
{
    const wchar_t* USER_AGENT = L"WTAID/1.0";
}

HTTPClient::HTTPClient()
    : host_("127.0.0.1")
    , port_(8111)
    , path_("/state")
    , timeoutMs_(1000)
    , responseCallback_(nullptr)
    , indicatorsHost_("127.0.0.1")
    , indicatorsPort_(8111)
    , indicatorsPath_("/indicators")
    , indicatorsTimeoutMs_(1000)
    , indicatorsResponseCallback_(nullptr)
    , indicatorsRunning_(false)
    , lastError_("")
    , running_(false)
{
}

HTTPClient::~HTTPClient()
{
    stopPolling();
}

void HTTPClient::initialize(const std::string& host, int port, const std::string& path, int timeoutMs)
{
    host_ = host;
    port_ = port;
    path_ = path;
    timeoutMs_ = timeoutMs;
}

void HTTPClient::onResponse(ResponseCallback callback)
{
    responseCallback_ = std::move(callback);
}

void HTTPClient::onIndicatorsResponse(ResponseCallback callback)
{
    indicatorsResponseCallback_ = std::move(callback);
}

void HTTPClient::initializeIndicators(const std::string& host,
                                       int port,
                                       const std::string& path,
                                       int timeoutMs)
{
    indicatorsHost_ = host;
    indicatorsPort_ = port;
    indicatorsPath_ = path;
    indicatorsTimeoutMs_ = timeoutMs;
}

std::optional<std::string> HTTPClient::fetch()
{
    return performRequest();
}

void HTTPClient::startPolling(int intervalMs)
{
    if (isRunning())
        return;

    running_ = true;
    pollingThread_ = std::thread([this, intervalMs]() {
        while (running_.load())
        {
            auto result = performRequest();
            if (result.has_value() && responseCallback_)
            {
                responseCallback_(result.value());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        }
    });
}

void HTTPClient::stopPolling()
{
    if (!isRunning())
        return;

    running_ = false;
    if (pollingThread_.joinable())
        pollingThread_.join();
}

bool HTTPClient::isRunning() const
{
    return running_.load();
}

std::string HTTPClient::getLastError() const
{
    return lastError_;
}

std::optional<std::string> HTTPClient::performRequest()
{
    lastError_.clear();

    // Открываем session
    HINTERNET hSession = WinHttpOpen(USER_AGENT,
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS,
                                     0);
    if (!hSession)
    {
        lastError_ = "WinHttpOpen failed";
        return std::nullopt;
    }

    WinHttpSetTimeouts(hSession, timeoutMs_, 30000, 30000, 30000);

    // Открываем соединение
    std::wstring wHost(host_.begin(), host_.end());
    HINTERNET hConnect = WinHttpConnect(hSession,
                                        wHost.c_str(),
                                        static_cast<INTERNET_PORT>(port_),
                                        0);
    if (!hConnect)
    {
        lastError_ = "WinHttpConnect failed";
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }

    // Открываем запрос
    std::wstring wPath(path_.begin(), path_.end());
    LPCWSTR path = wPath.c_str();
    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                            L"GET",
                                            path,
                                            nullptr,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            0);
    if (!hRequest)
    {
        lastError_ = "WinHttpOpenRequest failed";
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }

    // Выполняем запрос
    BOOL bResult = WinHttpSendRequest(hRequest,
                                      WINHTTP_NO_ADDITIONAL_HEADERS,
                                      0,
                                      WINHTTP_NO_REQUEST_DATA,
                                      0,
                                      0,
                                      0);
    if (!bResult)
    {
        lastError_ = "WinHttpSendRequest failed";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }

    bResult = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResult)
    {
        lastError_ = "WinHttpReceiveResponse failed";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }

    // Читаем ответ
    std::string responseBody;
    DWORD bytesRead = 0;
    char buffer[4096];
    do
    {
        bytesRead = 0;
        bResult = WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead);
        if (bResult && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            responseBody.append(buffer, bytesRead);
        }
    } while (bResult && bytesRead > 0);

    // Проверяем статус
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (statusCode != 200)
    {
        std::ostringstream oss;
        oss << "HTTP error: status code " << statusCode;
        lastError_ = oss.str();
        return std::nullopt;
    }

    return responseBody;
}

void HTTPClient::startPollingIndicators(int intervalMs)
{
    if (indicatorsRunning_.load())
        return;

    indicatorsRunning_ = true;
    indicatorsPollingThread_ = std::thread([this, intervalMs]() {
        while (indicatorsRunning_.load())
        {
            auto result = performIndicatorsRequest();
            if (result.has_value() && indicatorsResponseCallback_)
            {
                indicatorsResponseCallback_(result.value());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        }
    });
}

void HTTPClient::stopPollingIndicators()
{
    if (!indicatorsRunning_.load())
        return;

    indicatorsRunning_ = false;
    if (indicatorsPollingThread_.joinable())
        indicatorsPollingThread_.join();
}

std::optional<std::string> HTTPClient::performIndicatorsRequest()
{
    // Открываем session
    HINTERNET hSession = WinHttpOpen(USER_AGENT,
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS,
                                     0);
    if (!hSession)
        return std::nullopt;

    WinHttpSetTimeouts(hSession, indicatorsTimeoutMs_, 30000, 30000, 30000);

    std::wstring wHost(indicatorsHost_.begin(), indicatorsHost_.end());
    HINTERNET hConnect = WinHttpConnect(hSession,
                                        wHost.c_str(),
                                        static_cast<INTERNET_PORT>(indicatorsPort_),
                                        0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }

    std::wstring wPath(indicatorsPath_.begin(), indicatorsPath_.end());
    LPCWSTR path = wPath.c_str();
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
                                            nullptr, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }

    BOOL bResult = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS,
                                      0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!bResult)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }

    bResult = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResult)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return std::nullopt;
    }

    std::string responseBody;
    DWORD bytesRead = 0;
    char buffer[4096];
    do
    {
        bytesRead = 0;
        bResult = WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead);
        if (bResult && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            responseBody.append(buffer, bytesRead);
        }
    } while (bResult && bytesRead > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (responseBody.empty())
        return std::nullopt;

    return responseBody;
}
