#pragma once
/*
 * @brief header_only file for pebble log
 */

#include <memory>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <format>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>

namespace utils::Log::MiddleWare {
    class MiddlewareBase {
    public:
        virtual ~MiddlewareBase() = default;
        virtual void sink() = 0;
    };

    template<class MiddlewareType>
    class LoggerMiddleware : public MiddlewareBase {
    public:
        void sink() override {
            static_cast<MiddlewareType *>(this)->process();
        }
    };

    class MiddlewareChain {
        std::vector<std::shared_ptr<MiddlewareBase>> middlewares;

    public:
        template<typename MiddlewareType, typename... Args>
        void addMiddleware(Args &&...args) {
            middlewares.emplace_back(
                    std::make_shared<MiddlewareType>(std::forward<Args>(args)...));
        }

        void process() {
            for (const auto &middleware: middlewares) {
                middleware->sink();
            }
        }
    };
}// namespace utils::Log::MiddleWare

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
                logMessage.pop_back();// 移除末尾多余的空格
            }
            if (!logMessage.empty() && logMessage.back() == ',') {
                logMessage.pop_back();// 移除末尾多余的逗号
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
    PebbleLog::traceFunction(__FILE__, __LINE__, __func__, func, ##__VA_ARGS__);

#include <chrono>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <type_traits>

namespace utils::Log::MiddleWare {

    // 使用 chrono 获取本地时间并格式化为字符串
    std::string getLocalTimeString(const std::string &format = "%Y-%m-%d %H:%M:%S") {
        // 获取当前时间点
        auto now = std::chrono::system_clock::now();
        // 转换为 time_t
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
        // 转换为本地时间
        std::tm localTime;
#ifdef _WIN32
        localtime_s(&localTime, &now_time_t);// Windows 使用 localtime_s
#else
        localtime_r(&now_time_t, &localTime);// Linux 使用 localtime_r
#endif

        // 格式化时间
        char timeStr[20];
        std::strftime(timeStr, sizeof(timeStr), format.c_str(), &localTime);
        return std::string(timeStr);
    }

    class LocalTimeStampMiddleware : public LoggerMiddleware<LocalTimeStampMiddleware> {
    public:
        LocalTimeStampMiddleware() : formatStr("%Y-%m-%d %H:%M:%S") {}

        template<typename T>
            requires std::is_constructible_v<std::string, T>// 编译期约束：确保参数可转换为 std::string
        LocalTimeStampMiddleware(T &&format) : formatStr(std::forward<T>(format)) {
            static_assert(!std::is_same_v<std::decay_t<T>, std::string> || !formatStr.empty(),
                          "Time format string must not be empty.");
            static_assert(std::is_constructible_v<std::string, T>,
                          "Template parameter must be convertible to std::string.");
        }

        void process() {
            std::lock_guard<std::mutex> lock(PebbleLog::getMutex());
            std::string timeStr = getLocalTimeString(formatStr);
            PebbleLog::setLogName(PebbleLog::getLogName() + "[" + timeStr + "]");
            PebbleLog::setTimeFormat(formatStr);
        }

    private:
        std::string formatStr;
    };

    class DailyLogMiddleware : public LoggerMiddleware<DailyLogMiddleware> {
    public:
        DailyLogMiddleware() {}

        void process() {
            std::lock_guard<std::mutex> lock(PebbleLog::getMutex());
            std::string dateStr = getLocalTimeString("%Y-%m-%d");
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
        TraceMiddleware(const std::string &file, int line, const std::string &func, const std::string &args)
            : file(file), line(line), func(func), args(args) {}

        void process() {
            std::lock_guard<std::mutex> lock(PebbleLog::getMutex());
            std::stringstream ss;
            ss << "[Trace] File: " << file << ", Line: " << line << ", Function: " << func << ", Args: " << args;
            PebbleLog::info(ss.str());// 使用日志库输出
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
        CustomTagMiddleware(const std::string &tag) : tag(tag) {}

        void process() {
            std::lock_guard<std::mutex> lock(PebbleLog::getMutex());
            PebbleLog::setConsolePrefixFormat("[" + tag + "] " + PebbleLog::getConsolePrefixFormat());
        }

    private:
        std::string tag;
    };
}// namespace utils::Log::MiddleWare

#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>// 用于 open
#include <filesystem>
#include <fstream>
#ifdef _WIN32
#include <io.h>// for _open, _close
#include <windows.h>
#undef ERROR// 取消宏定义
#else
#include <unistd.h>
#endif

namespace utils::Log {
    namespace defalut {
        // 控制台的默认时间格式
        static std::string timeFormat = "%Y-%m-%d %H:%M:%S";
        static std::string filePrefixFormat = "";
    };// namespace defalut

    // 初始化静态成员
    PebbleLog::LogProperty PebbleLog::logProperty;
    std::mutex PebbleLog::logMutex;
    std::queue<std::pair<LogLevel, std::string>> PebbleLog::logQueue;// 定义静态成员变量 logQueue
    std::mutex PebbleLog::queueMutex;                                // 定义静态成员变量
    std::condition_variable PebbleLog::queueCond;                    // 定义静态成员变量
    static bool skipDebug = false;

    // 在 PebbleLog 构造函数中初始化控制台模式
    PebbleLog::PebbleLog() : stopFlag(false) {
#ifdef _WIN32
        // 启用虚拟终端支持
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
#endif
        logThread = std::thread(&PebbleLog::processLogs, this);
    }

    PebbleLog::~PebbleLog() {
        stopFlag = true;
        queueCond.notify_all();
        if (logThread.joinable()) {
            logThread.join();
        }
    }

    void PebbleLog::processLogs() {
        while (!stopFlag) {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCond.wait(lock, [this] { return !logQueue.empty() || stopFlag; });

            while (!logQueue.empty()) {
                auto entry = logQueue.front();
                logQueue.pop();
                lock.unlock();

                if (logProperty.type == LogType::CONSOLE) {
                    writeLogToConsole(entry.first, entry.second);
                } else if (logProperty.type == LogType::FILE) {
                    writeLogToFile(entry.second);
                } else {
                    writeLogToConsole(entry.first, entry.second);
                    writeLogToFile(entry.second);
                }

                lock.lock();
            }
        }
    }

    // 配置方法
    void PebbleLog::setLogLevel(LogLevel level) { logProperty.level = level; }
    void PebbleLog::setLogType(LogType type) { logProperty.type = type; }
    void PebbleLog::setMaxFileSize(size_t size) { logProperty.maxFileSize = size; }
    void PebbleLog::setMaxFileCount(size_t count) { logProperty.maxFileCount = count; }
    void PebbleLog::setLogPath(const std::string &path) { logProperty.logPath = path; }
    void PebbleLog::setLogName(const std::string &name) { logProperty.logName = name; }

    void PebbleLog::setTimeFormat(const std::string &format) { defalut::timeFormat = format; }

    void PebbleLog::setConsolePrefixFormat(const std::string &format) { defalut::filePrefixFormat = format; }

    void PebbleLog::setFilePrefixFormat(const std::string &format) {
        defalut::filePrefixFormat = format;
    }

    const std::string &PebbleLog::getLogName() { return logProperty.logName; }

    const std::string &PebbleLog::getConsolePrefixFormat() {
        return defalut::filePrefixFormat;
    }

    // 核心日志方法
    void PebbleLog::log(LogLevel level, std::string_view message) {
        if (level < logProperty.level) return;

        std::string formattedMessage;
        formatLogMessage(level, std::string(message), formattedMessage);

        std::lock_guard<std::mutex> lock(getInstance().queueMutex);
        getInstance().logQueue.push({level, formattedMessage});
        getInstance().queueCond.notify_one();
    }

    void PebbleLog::formatLogMessage(LogLevel level, const std::string &message, std::string &formattedMessage) {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);

        // 使用 std::strftime 替代 std::format 进行时间格式化
        char timeBuffer[20];
        std::strftime(timeBuffer, sizeof(timeBuffer), defalut::timeFormat.c_str(), &tm);
        std::string timeStr(timeBuffer);

        formattedMessage = std::format("[{}] {}[{}] {}", timeStr, defalut::filePrefixFormat.empty() ? "" : defalut::filePrefixFormat + " ", [&]() -> std::string {
                                   switch (level) {
                                       case LogLevel::INFO: return "INFO";
                                       case LogLevel::DEBUG: return "DEBUG";
                                       case LogLevel::WARN: return "WARN";
                                       case LogLevel::ERROR: return "ERROR";
                                       case LogLevel::FATAL: return "FATAL";
                                       case LogLevel::TRACE: return "TRACE";
                                       default: return "UNKNOWN";
                                   } }(), message);
    }

#ifdef _WIN32
    void PebbleLog::writeLogToConsole(LogLevel level, const std::string &message) {
        static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole == INVALID_HANDLE_VALUE) return;

        // 设置颜色
        WORD colorCode = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;// 默认白色
        switch (level) {
            case LogLevel::INFO:
                colorCode = FOREGROUND_GREEN;
                break;
            case LogLevel::DEBUG:
                colorCode = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                break;
            case LogLevel::WARN:
                colorCode = FOREGROUND_RED | FOREGROUND_GREEN;
                break;// 黄色
            case LogLevel::ERROR:
                colorCode = FOREGROUND_RED;
                break;
            case LogLevel::FATAL:
                colorCode = FOREGROUND_RED | FOREGROUND_BLUE;
                break;// 洋红
            case LogLevel::TRACE:
                colorCode = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                break;
            default:
                break;
        }
        SetConsoleTextAttribute(hConsole, colorCode);

        // 写入消息
        DWORD written;
        WriteConsoleA(hConsole, message.c_str(), static_cast<DWORD>(message.size()), &written, nullptr);
        WriteConsoleA(hConsole, "\n", 1, &written, nullptr);// 换行符

        // 恢复默认颜色
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
#endif

#ifndef _WIN32
    void PebbleLog::writeLogToConsole(LogLevel level, const std::string &message) {
        // 使用 ANSI 转义序列
        const char *colorCode = "";
        switch (level) {
            case LogLevel::INFO:
                colorCode = "\033[32m";
                break;
            case LogLevel::DEBUG:
                colorCode = "\033[36m";
                break;
            case LogLevel::WARN:
                colorCode = "\033[33m";
                break;
            case LogLevel::ERROR:
                colorCode = "\033[31m";
                break;
            case LogLevel::FATAL:
                colorCode = "\033[35m";
                break;
            case LogLevel::TRACE:
                colorCode = "\033[34m";
                break;
            default:
                colorCode = "\033[0m";
                break;
        }

        // 系统调用
        std::string output = colorCode + message + "\033[0m\n";
        ssize_t result = write(STDOUT_FILENO, output.c_str(), output.size());
        if (result == -1) {
            // 备用方案：使用 cerr（避免递归调用）
            std::cerr << "Console write failed: " << strerror(errno) << std::endl;
        }
    }
#endif

    // 输出到文件（支持轮转）
    void PebbleLog::writeLogToFile(const std::string &message) {
        std::filesystem::create_directories(logProperty.logPath);
        std::string fullPath = logProperty.logPath + "/" + logProperty.logName;

        // 检查文件大小
        std::ifstream inFile(fullPath, std::ios::ate);
        if (inFile.is_open() && inFile.tellg() >= static_cast<std::streampos>(logProperty.maxFileSize)) {
            inFile.close();
            // 从 maxFileCount - 1 到 1 重命名文件
            for (int i = logProperty.maxFileCount - 1; i > 0; --i) {
                std::string oldName = fullPath + "." + std::to_string(i - 1);
                std::string newName = fullPath + "." + std::to_string(i);
                if (std::filesystem::exists(oldName)) {
                    try {
                        std::filesystem::rename(oldName, newName);
                    } catch (const std::filesystem::filesystem_error &e) {
                        error("Filesystem error: " + std::string(e.what()));
                    } catch (const std::exception &e) {
                        error("General error: " + std::string(e.what()));
                    }
                }
            }
            // 将当前文件重命名为 fullPath.1
            std::string newName = fullPath + ".1";
            if (std::filesystem::exists(fullPath)) {
                try {
                    std::filesystem::rename(fullPath, newName);
                } catch (const std::filesystem::filesystem_error &e) {
                    error("Filesystem error: " + std::string(e.what()));
                } catch (const std::exception &e) {
                    error("General error: " + std::string(e.what()));
                }
            }
        }

        std::ofstream outFile(fullPath, std::ios::app);
        if (outFile.is_open()) {
            outFile << message << std::endl;
            outFile.close();
        } else {
            error("Failed to open log file: " + fullPath);
        }
    }
}// namespace utils::Log
