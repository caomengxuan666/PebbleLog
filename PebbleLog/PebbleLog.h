/*
#pragma once

#ifdef _WIN32
#ifdef PEBBLE_LOG_EXPORTS
#define PEBBLE_LOG_API __declspec(dllexport)
#else
#define PEBBLE_LOG_API __declspec(dllimport)
#endif
#else
#define PEBBLE_LOG_API// 非 Windows 平台无需特殊处理
#endif

#include "MiddleWare.hpp"
#include <atomic>
#include <condition_variable>
#include <format>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
using namespace utils::Log;
namespace utils::Log {
    using namespace Log::MiddleWare;

    enum class LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        TRACE
    };

    enum class LogType {
        CONSOLE,
        FILE,
        BOTH
    };

    class PebbleLog {
        friend class MiddlewareChain;// 允许中间件访问私有成员
    public:
        // 日志记录方法
        template<typename... Args>
        static void info(const std::string_view formatStr, Args &&...args) {
            log(LogLevel::INFO, std::vformat(formatStr, std::make_format_args(args...)));
        }
        template<typename... Args>
        static void debug(const std::string_view formatStr, Args &&...args) {
            log(LogLevel::DEBUG, std::vformat(formatStr, std::make_format_args(args...)));
        }
        template<typename... Args>
        static void warn(const std::string_view formatStr, Args &&...args) {
            log(LogLevel::WARN, std::vformat(formatStr, std::make_format_args(args...)));
        }
        template<typename... Args>
        static void error(const std::string_view formatStr, Args &&...args) {
            log(LogLevel::ERROR, std::vformat(formatStr, std::make_format_args(args...)));
        }
        template<typename... Args>
        static void fatal(const std::string_view formatStr, Args &&...args) {
            log(LogLevel::FATAL, std::vformat(formatStr, std::make_format_args(args...)));
        }
        template<typename... Args>
        static void trace(const std::string_view formatStr, Args &&...args) {
            log(LogLevel::TRACE, std::vformat(formatStr, std::make_format_args(args...)));
        }

        // 核心日志方法
        static void log(LogLevel level, std::string_view message);

        // 配置方法
        static void setLogLevel(LogLevel level);
        static void setLogType(LogType type);
        static void setMaxFileSize(size_t size);
        static void setMaxFileCount(size_t count);
        static void setLogPath(const std::string &path);
        static void setLogName(const std::string &name);
        static void setTimeFormat(const std::string &format);
        static void setConsolePrefixFormat(const std::string &prefix);
        static void setFilePrefixFormat(const std::string &format);

        static const std::string &getLogName();
        static const std::string &getConsolePrefixFormat();

        // 新增函数，用于记录函数的出入参和内部变量
        template<typename Func, typename... Args>
        static void traceFunction(const char *file, int line, const char *function, Func &&func, Args &&...args) {
            std::ostringstream oss;
            oss << "File: " << file << ", Line: " << line << ", Function: " << function << " | ";
            oss << "Args: ";
            ((oss << args << " (" << typeid(args).name() << "), "), ...);
            std::string logMessage = oss.str();
            if (!logMessage.empty() && logMessage.back() == ' ') {
                logMessage.pop_back(); // 移除末尾多余的空格
            }
            if (!logMessage.empty() && logMessage.back() == ',') {
                logMessage.pop_back(); // 移除末尾多余的逗号
            }
            log(LogLevel::TRACE, logMessage);

            // 执行函数并记录返回值
            auto result = func(std::forward<Args>(args)...);
            oss.str("");
            oss << "Return: " << result << " (" << typeid(result).name() << ")";
            log(LogLevel::TRACE, oss.str());
        }

        // 中间件的代理类
        class MiddlewareProxy {
        public:
            // 直接接受中间件实例（支持临时对象）
            template<typename MiddlewareType>
            MiddlewareProxy &operator|(MiddlewareType &&middleware) {
                using DecayedType = std::decay_t<MiddlewareType>;
                getInstance().middlewareChain.addMiddleware<DecayedType>(std::forward<MiddlewareType>(middleware));
                return *this;
            }
        };

        class LogStream {
        public:
            // 左移运算符重载
            template<typename T>
            LogStream &operator<<(const T &message) {
                if (!stream_) {
                    stream_ = std::make_unique<std::ostringstream>();
                }
                (*stream_) << message;// 将消息写入缓冲区
                return *this;
            }

            // 析构函数中触发日志记录
            ~LogStream() {
                if (stream_ && stream_->tellp() > 0) {                // 如果有内容
                    PebbleLog::log(logProperty.level, stream_->str());// 调用核心日志方法
                }
            }

        private:
            std::unique_ptr<std::ostringstream> stream_;// 缓冲区，用于拼接消息
        };
        //静态方法，返回 LogStream 对象
        static LogStream log() {
            return LogStream();
        }

        // 静态方法获取代理对象
        static MiddlewareProxy middleware() { return {}; }

        // 中间件相关方法
        template<typename MiddlewareType, typename... Args>
        static void addMiddleware(Args &&...args) {
            getInstance().middlewareChain.addMiddleware<MiddlewareType>(
                    std::forward<Args>(args)...);
        }

        template<typename MiddlewareType, typename... Args>
        PebbleLog &operator|(Args &&...args) {
            addMiddleware<MiddlewareType>(std::forward<Args>(args)...);
            return *this;
        }

        static void applyMiddlewares() {
            getInstance().middlewareChain.process();
        }

        // 获取互斥锁
        static std::mutex &getMutex() { return logMutex; }

    private:
        struct LogProperty {
            LogLevel level = LogLevel::DEBUG;
            LogType type = LogType::CONSOLE;
            size_t maxFileSize = 10 * 1024 * 1024;// 默认 10MB
            size_t maxFileCount = 5;
            std::string logPath = "./logs";
            std::string logName = "app.log";
            std::string logFullPathName;
        };

        static PebbleLog &getInstance() {
            static PebbleLog instance;
            return instance;
        }

        PebbleLog();
        ~PebbleLog();

        static void formatLogMessage(LogLevel level, const std::string &message, std::string &formattedMessage);
        static void writeLogToFile(const std::string &message);
        static void writeLogToConsole(LogLevel level, const std::string &message);

        static LogProperty logProperty;
        static std::mutex logMutex;
        MiddlewareChain middlewareChain;// 内嵌中间件链

        // 异步日志处理相关
        static std::queue<std::pair<LogLevel, std::string>> logQueue;
        static std::mutex queueMutex;
        static std::condition_variable queueCond;
        std::atomic<bool> stopFlag;
        std::thread logThread;

        void processLogs();
    };
}// namespace utils::Log

#define PEBBLETRACE(func, ...) \
    PebbleLog::traceFunction(__FILE__, __LINE__, __func__, func, ##__VA_ARGS__); \


    */