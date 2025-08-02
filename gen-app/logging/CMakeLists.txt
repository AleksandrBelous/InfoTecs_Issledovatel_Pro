# Библиотека логирования
add_library(logging_lib_gen STATIC
        Logger.cpp
        LogManager.cpp
)

# Включение директорий для заголовочных файлов
target_include_directories(logging_lib_gen PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
) 