# Библиотека сетевого слоя
add_library(network_lib
        SocketManager.cpp
        EpollManager.cpp
)

# Привязка зависимостей
target_link_libraries(network_lib PRIVATE
        pthread
)

# Включение директорий для заголовочных файлов
target_include_directories(network_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
) 