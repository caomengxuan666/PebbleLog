# PebbleLog/CMakeLists.txt
add_library(PebbleLog_STATIC STATIC
    PebbleLog.cpp
    MiddleWareImpl.hpp
    MiddleWare.hpp
    PebbleLog.h
)

add_library(PebbleLog_SHARED SHARED
    PebbleLog.cpp
    MiddleWareImpl.hpp
    MiddleWare.hpp
    PebbleLog.h
)