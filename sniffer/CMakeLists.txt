# Добавляем поддиректории
add_subdirectory(packet_processor)
add_subdirectory(flow_tracker)
add_subdirectory(statistics)
add_subdirectory(logging)

# Основной исполняемый файл
add_executable(sniffer main.cpp)

# Привязка библиотек
target_link_libraries(sniffer PRIVATE
        pthread
        pcap
        packet_processor_lib
        flow_tracker_lib
        statistics_lib
        logging_lib_sniffer
)

# Включение директорий для заголовочных файлов
target_include_directories(sniffer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/packet_processor
        ${CMAKE_CURRENT_SOURCE_DIR}/flow_tracker
        ${CMAKE_CURRENT_SOURCE_DIR}/statistics
        ${CMAKE_CURRENT_SOURCE_DIR}/logging
) 