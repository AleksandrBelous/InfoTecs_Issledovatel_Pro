# Создание библиотеки для логирования
add_library(logging_lib_sniffer STATIC
        Logger.cpp
        LogManager.cpp
)

# Включение директорий для заголовочных файлов
target_include_directories(logging_lib_sniffer PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
) 