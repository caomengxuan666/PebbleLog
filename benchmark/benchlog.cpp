#include"../PebbleLog_ho.hpp"
#include <string>
#include<benchmark/benchmark.h>

// 初始化日志库
static void initLogger() {
    using namespace utils::Log;

    // 配置日志
    PebbleLog::setLogLevel(LogLevel::DEBUG);
    PebbleLog::setLogType(LogType::CONSOLE);
    PebbleLog::setLogPath("./bench_logs");
    PebbleLog::setLogName("bench.log");
    PebbleLog::setMaxFileSize(10 * 1024 * 1024); // 10MB
    PebbleLog::setMaxFileCount(5);
}

// 测试 info 级别的日志记录
static void BM_LogInfo(benchmark::State& state) {
    using namespace utils::Log;

    initLogger();

    // Warm-up 阶段
    for (int i = 0; i < state.range(0); ++i) {
        PebbleLog::info("This is an info message.");
    }

    // 实际测试阶段
    for (auto _ : state) {
        PebbleLog::info("This is an info message.");
    }
}

// 测试 debug 级别的日志记录
static void BM_LogDebug(benchmark::State& state) {
    using namespace utils::Log;

    initLogger();

    // Warm-up 阶段
    for (int i = 0; i < state.range(0); ++i) {
        PebbleLog::debug("This is a debug message.");
    }

    // 实际测试阶段
    for (auto _ : state) {
        PebbleLog::debug("This is a debug message.");
    }
}

// 测试 warn 级别的日志记录
static void BM_LogWarn(benchmark::State& state) {
    using namespace utils::Log;

    initLogger();

    // Warm-up 阶段
    for (int i = 0; i < state.range(0); ++i) {
        PebbleLog::warn("This is a warning message.");
    }

    // 实际测试阶段
    for (auto _ : state) {
        PebbleLog::warn("This is a warning message.");
    }
}

// 测试 error 级别的日志记录
static void BM_LogError(benchmark::State& state) {
    using namespace utils::Log;

    initLogger();

    // Warm-up 阶段
    for (int i = 0; i < state.range(0); ++i) {
        PebbleLog::error("This is an error message.");
    }

    // 实际测试阶段
    for (auto _ : state) {
        PebbleLog::error("This is an error message.");
    }
}

// 测试 fatal 级别的日志记录
static void BM_LogFatal(benchmark::State& state) {
    using namespace utils::Log;

    initLogger();

    // Warm-up 阶段
    for (int i = 0; i < state.range(0); ++i) {
        PebbleLog::fatal("This is a fatal message.");
    }

    // 实际测试阶段
    for (auto _ : state) {
        PebbleLog::fatal("This is a fatal message.");
    }
}

// 注册基准测试
BENCHMARK(BM_LogInfo)->Arg(1000);
BENCHMARK(BM_LogDebug)->Arg(1000);
BENCHMARK(BM_LogWarn)->Arg(1000);
BENCHMARK(BM_LogError)->Arg(1000);
BENCHMARK(BM_LogFatal)->Arg(1000);

// 运行所有注册的基准测试
BENCHMARK_MAIN();