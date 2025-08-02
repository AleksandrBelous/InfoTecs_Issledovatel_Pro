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

# Настройка путей для выходных файлов
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Подкаталоги с отдельными программами
add_subdirectory(gen-app)
add_subdirectory(sniffer)

# Подкаталог с тестами (опционально)
option(BUILD_TESTS "Build unit tests" ON)
if (BUILD_TESTS)
    add_subdirectory(tests)
    message(STATUS "  - tests (Unit tests with Google Test)")
endif ()

# Вывод информации о проектах
message(STATUS "Building projects:")
message(STATUS "  - gen-app (TCP client/server application)")
message(STATUS "  - sniffer (Network packet analyzer)")
if (BUILD_TESTS)
    message(STATUS "  - tests (Unit tests)")
endif ()
message(STATUS "Output directory: ${CMAKE_BINARY_DIR}/bin")
