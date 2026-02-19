cmake_minimum_required(VERSION 3.10)
project(MyProject C)

# 1. 设置导出编译指令，供 nvim clangd 使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 2. 指定公共头文件根目录
# 这样你就可以用 <core/connection.h> 而不是 "connection.h"
include_directories(include)

# 3. 收集各层源文件
file(GLOB_RECURSE CORE_SRCS "src/core/*.c")
file(GLOB_RECURSE TRANS_SRCS "src/transport/*.c")
file(GLOB_RECURSE PROTO_SRCS "src/protocol/*.c")
file(GLOB_RECURSE BUS_SRCS "src/bus/*.c")

# 4. 将各层编译为静态库（解耦）
add_library(core_lib STATIC ${CORE_SRCS})
add_library(trans_lib STATIC ${TRANS_SRCS})
add_library(proto_lib STATIC ${PROTO_SRCS})
add_library(bus_lib STATIC ${BUS_SRCS})

# 5. 生成可执行文件
add_executable(my_app main.c)

# 6. 链接所有模块
target_link_libraries(my_app 
  trans_lib
  bus_lib
  proto_lib
  core_lib
)
