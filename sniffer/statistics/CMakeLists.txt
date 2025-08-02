# Создание библиотеки для статистики
add_library(statistics_lib STATIC
        StatisticsManager.cpp
)

# Включение директорий для заголовочных файлов
target_include_directories(statistics_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
) 