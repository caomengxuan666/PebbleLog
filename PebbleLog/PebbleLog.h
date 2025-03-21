#pragma once

#include "MiddleWare.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>

#include <format>

using namespace utils::Log;
namespace utils::Log {
    using namespace Log::MiddleWare;
    enum class LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
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
        template <typename... Args>
        static void info(const std::string_view formatStr, Args&&... args) {
            log(LogLevel::INFO, std::vformat(formatStr, std::make_format_args(args...)));
        }
        template <typename... Args>
        static void debug(const std::string_view formatStr, Args&&... args) {
            log(LogLevel::DEBUG, std::vformat(formatStr, std::make_format_args(args...)));
        }
        template <typename... Args>
        static void warn(const std::string_view formatStr, Args&&... args) {
            log(LogLevel::WARN, std::vformat(formatStr, std::make_format_args(args...)));
        }
        template <typename... Args>
        static void error(const std::string_view formatStr, Args&&... args) {
            log(LogLevel::ERROR, std::vformat(formatStr, std::make_format_args(args...)));
        }
    
        template <typename... Args>
        static void fatal(const std::string_view formatStr, Args&&... args) {
            log(LogLevel::FATAL, std::vformat(formatStr, std::make_format_args(args...)));
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
        static void setFilePrefixFormat(const std::string &prefix);

        static const std::string &getLogName();

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

        void operator()() {
            applyMiddlewares();
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