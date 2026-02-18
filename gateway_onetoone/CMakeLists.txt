cmake_minimum_required(VERSION 3.10)
project(MyProject C)

# 1. 自动搜集所有的源文件 (.c)
file(GLOB_RECURSE SOURCES 
    "main.c"
    "src/core/*.c"
    "src/transport/*.c"
    "src/protocol/*.c"
)

# 2. 指定头文件搜索路径 (告诉编译器去哪里找 .h)
include_directories(
    src/core
    src/transport
    src/protocol
)

# 3. 生成可执行文件
add_executable(my_app ${SOURCES})
