add_executable(LogExample LogExample.cpp "LogHeadOnly.cpp")

target_link_libraries(LogExample PRIVATE PebbleLog_STATIC) 

add_executable(LogHeadOnly LogHeadOnly.cpp)