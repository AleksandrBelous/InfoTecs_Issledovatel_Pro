# Библиотека сервера
add_library(server_lib
        TCPServer.cpp
        ServerConfig.cpp
)

# Привязка зависимостей
target_link_libraries(server_lib PRIVATE
        network_lib
        logging_lib
)

# Включение директорий для заголовочных файлов
target_include_directories(server_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
) 