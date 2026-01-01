cmake_minimum_required(VERSION 3.16)
project(toyc_compiler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 编译器警告选项
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()


add_executable(toyc
    main.cpp
    lexer/lexer.cpp
    parser/parser.cpp
    parser/ast.cpp
    semantic/semantic.cpp
    ir/irgen.cpp
    codegen/codegen.cpp
)

# 设置头文件搜索路径
target_include_directories(toyc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/semantic
    ${CMAKE_CURRENT_SOURCE_DIR}/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen
)

# 如果irgen.cpp很大，可能需要调整编译设置来避免过长的编译时间
# 可以尝试启用多进程编译
set(CMAKE_BUILD_PARALLEL_LEVEL 4)

# 根据操作系统调整设置
if(UNIX AND NOT APPLE)
    # Linux特定设置
    target_link_libraries(toyc)
elseif(APPLE)
    # macOS特定设置
    target_link_libraries(toyc)
elseif(WIN32)
    # Windows特定设置
    target_link_libraries(toyc)
endif()

# 安装目标（可选）
install(TARGETS toyc DESTINATION bin)