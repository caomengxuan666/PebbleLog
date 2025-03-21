#pragma once
#include "PebbleLog.h"
#include "MiddleWare.hpp"
#include <type_traits>

namespace utils::Log::MiddleWare {

    class LocalTimeStampMiddleware : public LoggerMiddleware<LocalTimeStampMiddleware> {
    public:
        LocalTimeStampMiddleware() : formatStr("%Y-%m-%d %H:%M:%S") {}

        template <typename T>
        requires std::is_constructible_v<std::string, T> // 编译期约束：确保参数可转换为 std::string
        LocalTimeStampMiddleware(T&& format) : formatStr(std::forward<T>(format)) {
            static_assert(!std::is_same_v<std::decay_t<T>, std::string> || !formatStr.empty(), 
                          "Time format string must not be empty.");
            static_assert(std::is_constructible_v<std::string, T>, 
                          "Template parameter must be convertible to std::string.");
        }

        void process() {
            std::lock_guard<std::mutex> lock(PebbleLog::getMutex());
            std::time_t now = std::time(nullptr);
            std::tm localTime;
            localtime_r(&now, &localTime);
            char timeStr[20];
            std::strftime(timeStr, sizeof(timeStr), formatStr.c_str(), &localTime);
            PebbleLog::setLogName(PebbleLog::getLogName() + "[" + std::string(timeStr) + "]");
            PebbleLog::setTimeFormat(formatStr);
        }

    private:
        std::string formatStr;
    };

    class FileNamePrefixMiddleware : public LoggerMiddleware<FileNamePrefixMiddleware> {
    public:
        FileNamePrefixMiddleware() : prefix("PREFIX_") {}

        template <typename T>
        requires std::is_constructible_v<std::string, T> // 编译期约束：确保参数可转换为 std::string
        FileNamePrefixMiddleware(T&& customPrefix) : prefix(std::forward<T>(customPrefix)) {
            static_assert(!std::is_same_v<std::decay_t<T>, std::string> || !prefix.empty(), 
                          "File name prefix must not be empty.");
            static_assert(std::is_constructible_v<std::string, T>, 
                          "Template parameter must be convertible to std::string.");
        }

        void process() {
            std::lock_guard<std::mutex> lock(PebbleLog::getMutex());
            PebbleLog::setLogName(prefix + PebbleLog::getLogName());
        }

    private:
        std::string prefix;
    };

    class ConsolePrefixMiddleware : public LoggerMiddleware<ConsolePrefixMiddleware> {
    public:
        ConsolePrefixMiddleware() : prefix("[cmx]") {}

        template <typename T>
        requires std::is_constructible_v<std::string, T> // 编译期约束：确保参数可转换为 std::string
        ConsolePrefixMiddleware(T&& customPrefix) : prefix(std::forward<T>(customPrefix)) {
            static_assert(!std::is_same_v<std::decay_t<T>, std::string> || !prefix.empty(), 
                          "Console prefix must not be empty.");
            static_assert(std::is_constructible_v<std::string, T>, 
                          "Template parameter must be convertible to std::string.");
        }

        void process() {
            std::lock_guard<std::mutex> lock(PebbleLog::getMutex());
            PebbleLog::setConsolePrefixFormat(prefix);
        }

    private:
        std::string prefix;
    };
} // namespace utils::Log::MiddleWare