cmake_minimum_required(VERSION 3.20)
project(ToyC_Compiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(src)
include_directories(.)

# 源文件 - 使用手动实现的词法和语法分析器
set(SOURCES
    src/toyc_main.cpp
    src/ast.cpp
    src/symbol_table.cpp
    src/semantic_analyzer.cpp
    src/code_generator.cpp
    src/optimizer.cpp
    src/manual_lexer.cpp
    src/manual_parser.cpp
)

# 创建可执行文件
add_executable(toyc_compiler ${SOURCES})

# 编译选项
target_compile_options(toyc_compiler PRIVATE -Wall -Wextra -O2)

# 创建优化版本的编译器（用于-opt参数）
add_executable(toyc_compiler_opt ${SOURCES})
target_compile_definitions(toyc_compiler_opt PRIVATE ENABLE_OPTIMIZATION=1)
target_compile_options(toyc_compiler_opt PRIVATE -Wall -Wextra -O2)
