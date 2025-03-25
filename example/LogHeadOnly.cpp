#include "../PebbleLog_ho.hpp"

void logMessages() {
    using namespace utils::Log;

    // ʹ�� std::this_thread::get_id() ��ȡ�߳� ID
    std::thread::id threadId = std::this_thread::get_id();
    std::string threadIdStr = std::to_string(std::hash<std::thread::id>{}(threadId));// �� thread::id ת��Ϊ�ַ���

    // ��¼��־
    PEBBLETRACE([](std::thread::id threadId) { return threadId; }, threadId);
    PebbleLog::debug("Thread {}: This is a debug message.", threadIdStr);
    PebbleLog::info("Thread {}: I love logging.", threadIdStr);
    PebbleLog::error("Thread {}: An error occurred.", threadIdStr);
}

int exampleFunction(int a, int b) {
    int c = a + b;
    PEBBLETRACE([](int a, int b) { return a + b; }, a, b);// ��¼�����ĳ���κ��ڲ�����
    return c;
}

void stringConcatenationExample(const std::string &str1, const std::string &str2) {
    std::string result = str1 + str2;
    PEBBLETRACE([](const std::string &str1, const std::string &str2) { return str1 + str2; }, str1, str2);
}


void pointerExample() {
    int *ptr = new int(42);
    PEBBLETRACE([](int *ptr) { return *ptr; }, ptr);
    delete ptr;
}

int main() {
    using namespace utils::Log::MiddleWare;

    // ������־
    PebbleLog::setLogLevel(LogLevel::DEBUG);
    PebbleLog::setLogType(LogType::BOTH);
    PebbleLog::setLogPath("./custom_logs");
    PebbleLog::setLogName("app.log");
    // ����м����ֱ�Ӵ���ʵ����
    PebbleLog::middleware() | MiddleWare::LocalTimeStampMiddleware("%Y-%m-%d %H:%M:%S");

    // Ӧ���м��
    PebbleLog::applyMiddlewares();

    // ��������߳̽�����־��¼
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(logMessages);
    }

    // �ȴ������߳����
    for (auto &thread: threads) {
        thread.join();
    }


    exampleFunction(10, 20);

    // �ַ���ƴ��ʾ��
    stringConcatenationExample("Hello, ", "World!");

    pointerExample();

    // ���̼߳�¼��־
    PebbleLog::setLogLevel(LogLevel::FATAL);
    PebbleLog::log() << "This is a FATAL message from the func : " << __func__ << " at Line: " << __LINE__ << " of" << " File :" << __FILE__;

    return 0;
}
