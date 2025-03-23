# PebbleLog

PebbleLog 是一个轻量级的C++异步日志库，支持多种日志级别、日志类型（控制台、文件、两者皆有）以及中间件扩展。它提供了灵活的日志配置选项，如日志文件大小限制、文件数量限制、日志路径和名称等。

## 特性

- **日志级别**：DEBUG, INFO, WARN, ERROR, FATAL
- **日志类型**：控制台输出、文件输出、两者皆有
- **中间件支持**：允许用户自定义中间件来扩展日志功能
- **配置选项**：
  - 设置日志文件大小限制
  - 设置日志文件数量限制
  - 自定义日志路径和名称
  - 支持时间格式化和前缀格式化
- **异步日志处理**：通过异步队列实现高效的日志写入，避免阻塞主线程
- **日志轮转**：支持按文件大小自动轮转日志文件
- **格式化日志**：支持使用占位符 `{}` 进行动态参数替换，类似 `std::format` 的语法

## 安装

1. 将 PebbleLog 项目克隆到您的本地仓库：
   ```bash
   git clone https://github.com/your-repo/PebbleLog.git
   ```

2. 将 PebbleLog 文件夹添加到您的项目中，并确保包含以下头文件：
    ```cpp
    #include "PebbleLog/PebbleLog.h"
    ```

3.配置 CMake 或其他构建工具以包含 PebbleLog 的源文件。

## 使用示例

### 基本用法
```cpp

#include "PebbleLog/PebbleLog.h"
int main() {
using namespace utils::Log;

    // 配置日志
    PebbleLog::setLogLevel(LogLevel::DEBUG);
    PebbleLog::setLogType(LogType::BOTH); // 控制台和文件同时输出
    PebbleLog::setLogPath("./logs");
    PebbleLog::setLogName("app.log");

    // 记录日志
    PebbleLog::info("This is an info message.");
    PebbleLog::debug("This is a debug message.");
    PebbleLog::error("Error code: {}, message: {}", 404, "Not Found");


}
```

### 中间件扩展
**通过中间件可以扩展日志功能，例如添加时间戳、文件名前缀或控制台前缀：**
```cpp

#include "PebbleLog/PebbleLog.h"
#include "PebbleLog/MiddleWareImpl.hpp"
int main() {
    using namespace utils::Log;

    // 添加中间件
    PebbleLog::middleware()
        | MiddleWare::LocalTimeStampMiddleware("%Y-%m-%d %H:%M:%S")
        | MiddleWare::FileNamePrefixMiddleware("DEBUG_")
        | MiddleWare::ConsolePrefixMiddleware("[cmx]");

    // 应用中间件
    PebbleLog::applyMiddlewares();

    // 记录日志
    PebbleLog::info("This is an info message with middleware support.");

    return 0;
}
```

### 格式化日志
PebbleLog 支持类似 std::format 的语法，允许使用占位符 {} 动态插入参数：

```cpp
PebbleLog::info("This is an {} message.", "INFO");
PebbleLog::error("Error code: {}, message: {}", 404, "Not Found");
```

### 左移运算符
PebbleLog 支持左移运算符 << 来快速记录日志：
这种情况下以全局日志级别决定日志级别输出

```cpp
PebbleLog::log() << "This is an INFO message.";
```

## 配置选项

以下是可用的配置方法：

| 方法                                      | 描述                                   |
|-------------------------------------------|----------------------------------------|
| `setLogLevel(LogLevel level)`             | 设置日志记录的最低级别                 |
| `setLogType(LogType type)`                | 设置日志输出类型（控制台、文件、两者） |
| `setMaxFileSize(size_t size)`             | 设置单个日志文件的最大大小（字节）     |
| `setMaxFileCount(size_t count)`           | 设置保留的日志文件最大数量             |
| `setLogPath(const std::string &path)`     | 设置日志文件存储路径                   |
| `setLogName(const std::string &name)`     | 设置日志文件名称                       |
| `setTimeFormat(const std::string &format)`| 设置时间格式                           |
| `setConsolePrefixFormat(const std::string &prefix)` | 设置控制台日志前缀             |
| `setFilePrefixFormat(const std::string &prefix)`   | 设置文件日志前缀               |

---

## 日志轮转

PebbleLog 支持基于文件大小的日志轮转功能。当日志文件达到指定大小时，会自动创建新的日志文件，并将旧文件重命名为带有编号的备份文件（如 `app.log.1`, `app.log.2` 等）。可以通过以下方法配置轮转策略：

- **`setMaxFileSize`**：设置单个日志文件的最大大小。
- **`setMaxFileCount`**：设置保留的日志文件最大数量。

---

## 性能优化

- **异步日志处理**：所有日志消息都会被推送到一个异步队列中，由后台线程负责写入，避免阻塞主线程。
- **线程安全**：通过互斥锁保护日志队列和配置操作，确保多线程环境下的安全性。

---

## 贡献

欢迎为 PebbleLog 贡献代码！如果您发现了问题或希望添加新功能，请提交 [Issue](https://github.com/caomengxuan666/PebbleLog/issues) 或 [Pull Request](https://github.com/caomengxuan666/PebbleLog/pulls)。

---

## 许可证

PebbleLog 使用 [MIT 许可证](LICENSE) 开源。