cmake_minimum_required(VERSION 3.31)

project(InfoTecs_Issledovatel_Pro LANGUAGES CXX)

add_executable(InfoTecs_Issledovatel_Pro main.cpp)

set_target_properties(InfoTecs_Issledovatel_Pro PROPERTIES
        CXX_STANDARD          20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS        NO
)

target_compile_options(InfoTecs_Issledovatel_Pro PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-O2 -Wall -Wextra -pedantic>
)
