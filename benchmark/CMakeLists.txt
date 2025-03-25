# 添加 Google Benchmark 库
find_package(benchmark REQUIRED)
add_executable(benchLog benchlog.cpp)
target_link_libraries(benchLog PRIVATE benchmark::benchmark benchmark::benchmark_main)
