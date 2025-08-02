# Библиотека клиента
add_library(client_lib
        TCPClient.cpp
)

# Привязка зависимостей
target_link_libraries(client_lib PRIVATE
        network_lib
        logging_lib
)

# Включение директорий для заголовочных файлов
target_include_directories(client_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
) 