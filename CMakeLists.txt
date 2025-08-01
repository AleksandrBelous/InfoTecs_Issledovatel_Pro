cmake_minimum_required(VERSION 3.20)
project(InfoTecs_Issledovatel_Pro LANGUAGES CXX)

# Общие требования к компилятору для всех зависимых программ
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Общие флаги диагностики и оптимизации
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-O2 -Wall -Wextra -pedantic)
endif ()

# Подкаталоги с отдельными программами
add_subdirectory(gen-app)
