add_executable(gen-app
        main.cpp
        ServerConfig.cpp
        TCPServer.cpp
        SocketManager.cpp
        EpollManager.cpp
        TCPClient.cpp
)

# Привязка библиотек
target_link_libraries(gen-app PRIVATE pthread)

# Включение директорий для заголовочных файлов
target_include_directories(gen-app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
