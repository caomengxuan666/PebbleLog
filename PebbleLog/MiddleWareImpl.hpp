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
        ConsolePrefixMiddleware() : prefix() {}

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

    class DailyLogMiddleware:public LoggerMiddleware<DailyLogMiddleware>{
        public:
        DailyLogMiddleware(){}

        void process(){
            std::lock_guard<std::mutex> lock(PebbleLog::getMutex());
            std::time_t now = std::time(nullptr);
            std::tm localTime;
            localtime_r(&now, &localTime);
            char dateStr[11]; // 格式为 YYYY-MM-DD
            std::strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &localTime);
            std::string logName = PebbleLog::getLogName();
            size_t dotPos = logName.find_last_of('.');
            if (dotPos != std::string::npos) {
                // 在文件名中插入日期
                std::string newLogName = logName.substr(0, dotPos) + "_" + dateStr + logName.substr(dotPos);
                PebbleLog::setLogName(newLogName);
            } else {
                // 如果没有扩展名，直接在文件名后添加日期
                PebbleLog::setLogName(logName + "_" + dateStr);
            }
        }
    };

class TraceMiddleware : public LoggerMiddleware<TraceMiddleware> {
public:
    TraceMiddleware(const std::string& file, int line, const std::string& func, const std::string& args)
        : file(file), line(line), func(func), args(args) {}

    void process() {
        std::lock_guard<std::mutex> lock(PebbleLog::getMutex());
        std::stringstream ss;
        ss << "[Trace] File: " << file << ", Line: " << line << ", Function: " << func << ", Args: " << args;
        PebbleLog::info(ss.str());
    }

private:
    std::string file;
    int line;
    std::string func;
    std::string args;
};

    class ThreadIDMiddleware : public LoggerMiddleware<ThreadIDMiddleware> {
    public:
        ThreadIDMiddleware() {}

        void process() {
            std::lock_guard<std::mutex> lock(PebbleLog::getMutex());
            std::stringstream ss;
            ss << "[ThreadID:" << std::this_thread::get_id() << "] ";
            PebbleLog::setConsolePrefixFormat(ss.str() + PebbleLog::getConsolePrefixFormat());
        }
    };

    class CustomTagMiddleware : public LoggerMiddleware<CustomTagMiddleware> {
    public:
        CustomTagMiddleware(const std::string& tag) : tag(tag) {}

        void process() {
            std::lock_guard<std::mutex> lock(PebbleLog::getMutex());
            PebbleLog::setConsolePrefixFormat("[" + tag + "] " + PebbleLog::getConsolePrefixFormat());
        }

    private:
        std::string tag;
    };
} // namespace utils::Log::MiddleWare