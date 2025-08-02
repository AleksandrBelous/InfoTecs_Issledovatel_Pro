# Библиотека логирования
add_library(logging_lib
        Logger.cpp
        LogManager.cpp
)

# Включение директорий для заголовочных файлов
target_include_directories(logging_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
) 