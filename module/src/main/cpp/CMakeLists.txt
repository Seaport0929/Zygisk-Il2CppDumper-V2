cmake_minimum_required(VERSION 3.18.1)

# 直接设置模块名，避免在 Gradle 中传递变量导致路径混乱
project("il2cppdumper")

message("Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 20)

# x18 寄存器锁定对某些 Android 系统的兼容性有帮助
set(LINKER_FLAGS "-ffixed-x18 -Wl,--hash-style=both")
set(C_FLAGS "-Werror=format -fdata-sections -ffunction-sections")
set(CXX_FLAGS "${CXX_FLAGS} -fno-exceptions -fno-rtti")

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    # 隐藏符号，减小体积，增加逆向难度
    set(C_FLAGS "${C_FLAGS} -O2 -fvisibility=hidden -fvisibility-inlines-hidden")
    set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,-exclude-libs,ALL -Wl,--gc-sections -Wl,--strip-all")
else ()
    set(C_FLAGS "${C_FLAGS} -O0")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS} ${CXX_FLAGS}")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LINKER_FLAGS}")

# 核心：只编译我们重构的三个核心文件，移除 xdl 源码
add_library(il2cppdumper SHARED
        main.cpp
        hack.cpp
        il2cpp_dump.cpp)

# 链接 Android 日志库
target_link_libraries(il2cppdumper log)

# 后处理：剥离调试符号，进一步优化体积
if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET il2cppdumper POST_BUILD
            COMMAND ${CMAKE_STRIP} --strip-all --remove-section=.comment $<TARGET_FILE:il2cppdumper>)
endif ()
