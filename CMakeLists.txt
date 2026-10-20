cmake_minimum_required(VERSION 3.20)
project(MinecraftClone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Автоматическая загрузка raylib
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

# Сбор всех .cpp файлов из папки src
file(GLOB SOURCES "src/*.cpp")

add_executable(MinecraftClone ${SOURCES})

target_include_directories(MinecraftClone PRIVATE src)
target_link_libraries(MinecraftClone PRIVATE raylib)