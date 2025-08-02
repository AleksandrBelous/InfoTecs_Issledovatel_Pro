# Создание библиотеки для отслеживания потоков
add_library(flow_tracker_lib STATIC
        FlowTracker.cpp
        FlowStats.cpp
)

# Включение директорий для заголовочных файлов
target_include_directories(flow_tracker_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
) 