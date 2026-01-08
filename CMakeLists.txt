cmake_minimum_required(VERSION 3.25)
project(tiny_js)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

include(FetchContent)

FetchContent_Declare(
        asmjit
        GIT_REPOSITORY https://github.com/asmjit/asmjit.git
        GIT_TAG master
)
FetchContent_MakeAvailable(asmjit)

# 不需要手动设置 arch 定义，asmjit 库会自动检测


file(GLOB SOURCES "src/*.cpp" "src/native/*.cpp")

add_executable(tiny_js main.cpp ${SOURCES})

target_link_libraries(tiny_js PRIVATE asmjit::asmjit)

add_subdirectory(tests)

# 设置二进制文件输出目录
set_target_properties(tiny_js PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/scripts)
