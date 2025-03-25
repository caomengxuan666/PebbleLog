#include"../PebbleLog_ho.hpp"
#include <thread>
void logMessages() {
    using namespace utils::Log;

    // 使用 std::this_thread::get_id() 获取线程 ID
    std::thread::id threadId = std::this_thread::get_id();
    std::string threadIdStr = std::to_string(std::hash<std::thread::id>{}(threadId)); // 将 thread::id 转换为字符串

    // 记录日志
    PEBBLETRACE([](std::thread::id threadId) { return threadId; }, threadId);
    PebbleLog::debug("Thread {}: This is a debug message.", threadIdStr); 
    PebbleLog::info("Thread {}: I love logging.", threadIdStr); 
    PebbleLog::error("Thread {}: An error occurred.", threadIdStr);
}

int exampleFunction(int a, int b) {
    int c = a + b;
    PEBBLETRACE([](int a, int b) { return a + b; }, a, b); // 记录函数的出入参和内部变量
    return c;
}

void stringConcatenationExample(const std::string& str1, const std::string& str2) {
    std::string result = str1 + str2;
    PEBBLETRACE([](const std::string& str1, const std::string& str2) { return str1 + str2; }, str1, str2);
}


void pointerExample() {
    int* ptr = new int(42);
    PEBBLETRACE([](int* ptr) { return *ptr; }, ptr);
    delete ptr;
}

int main() {
    using namespace utils::Log::MiddleWare;

    // 配置日志
    PebbleLog::setLogLevel(LogLevel::DEBUG);
    PebbleLog::setLogType(LogType::BOTH);
    PebbleLog::setLogPath("./custom_logs");
    PebbleLog::setLogName("app.log");
    // 添加中间件（直接传递实例）
    PebbleLog::middleware() | MiddleWare::LocalTimeStampMiddleware("%Y-%m-%d %H:%M:%S");           

    // 应用中间件
    PebbleLog::applyMiddlewares();

    // 创建多个线程进行日志记录
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(logMessages);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }


    exampleFunction(10, 20);

    // 字符串拼接示例
    stringConcatenationExample("Hello, ", "World!");

    pointerExample();

    // 主线程记录日志
    PebbleLog::setLogLevel(LogLevel::FATAL);
    PebbleLog::log() << "This is a FATAL message from the func : "<<__func__<<" at Line: "<<__LINE__<<" of"<<" File :"<<__FILE__;

    return 0;
}