#include "PebbleLog.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>// 用于 open
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>// 用于系统调用

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


    PebbleLog::PebbleLog() : stopFlag(false) {
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

    // 输出到控制台
    void PebbleLog::writeLogToConsole(LogLevel level, const std::string &message) {
        const char *colorCode = "";
        switch (level) {
            case LogLevel::INFO:
                colorCode = "\033[32m";// 绿色
                break;
            case LogLevel::DEBUG:
                colorCode = "\033[36m";// 青色
                break;
            case LogLevel::WARN:
                colorCode = "\033[33m";// 黄色
                break;
            case LogLevel::ERROR:
                colorCode = "\033[31m";// 红色
                break;
            case LogLevel::FATAL:
                colorCode = "\033[35m";// 洋红色
                break;
            case LogLevel::TRACE:
                colorCode = "\033[34m";// 蓝色
                break;
            default:
                colorCode = "\033[0m";// 默认颜色
                break;
        }

        std::string coloredMessage = colorCode + message + "\033[0m\n";// 确保每条日志消息都带有换行符
        ssize_t result = write(STDOUT_FILENO, coloredMessage.c_str(), coloredMessage.size());
        if (result == -1) {
            std::cerr << "Failed to write to console: " << strerror(errno) << std::endl;
        }
    }

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
                        std::cerr << "Filesystem error: " << e.what() << std::endl;
                    } catch (const std::exception &e) {
                        std::cerr << "General error: " << e.what() << std::endl;
                    }
                }
            }
            // 将当前文件重命名为 fullPath.1
            std::string newName = fullPath + ".1";
            if (std::filesystem::exists(fullPath)) {
                try {
                    std::filesystem::rename(fullPath, newName);
                } catch (const std::filesystem::filesystem_error &e) {
                    std::cerr << "Filesystem error: " << e.what() << std::endl;
                } catch (const std::exception &e) {
                    std::cerr << "General error: " << e.what() << std::endl;
                }
            }
        }

        std::ofstream outFile(fullPath, std::ios::app);
        if (outFile.is_open()) {
            outFile << message << std::endl;
            outFile.close();
        } else {
            std::cerr << "Failed to open log file: " << fullPath << std::endl;
        }
    }
}// namespace utils::Log