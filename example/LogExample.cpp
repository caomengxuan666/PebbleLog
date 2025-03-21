#include <MiddleWareImpl.hpp>
#include <PebbleLog.h>

int main() {
    using namespace utils::Log;

    // 配置日志
    PebbleLog::setLogLevel(LogLevel::DEBUG);
    PebbleLog::setLogType(LogType::BOTH);
    PebbleLog::setLogPath("./custom_logs");
    PebbleLog::setLogName("app.log");

    // 添加中间件（直接传递实例）
    PebbleLog::middleware() | MiddleWare::LocalTimeStampMiddleware("%Y-%m-%d %H:%M:%S")
            | MiddleWare::FileNamePrefixMiddleware("DEBUG_")                           
            | MiddleWare::ConsolePrefixMiddleware("[cmx]");                            

    // 应用中间件
    PebbleLog::applyMiddlewares();

    // 记录日志
    PebbleLog::debug("This is a {} message.", "debug");
    PebbleLog::error("This is an error message.");

    std::string str = "wyw";
    PebbleLog::info("I love {}", str);

    // 类似QDebug的功能
    PebbleLog::setLogLevel(LogLevel::FATAL);
    PebbleLog::log() << "This is a FATAL message.";


    return 0;
}