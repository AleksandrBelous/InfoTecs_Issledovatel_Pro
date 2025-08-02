# Создание библиотеки для обработки пакетов
add_library(packet_processor_lib STATIC
        PacketProcessor.cpp
        PacketParser.cpp
)

# Включение директорий для заголовочных файлов
target_include_directories(packet_processor_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
) 