# Добавляем поддиректории
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(network)
add_subdirectory(logging)

# Основной исполняемый файл
add_executable(gen-app main.cpp)

# Привязка библиотек
target_link_libraries(gen-app PRIVATE
        pthread
        server_lib
        client_lib
        network_lib
        logging_lib_gen
)

# Включение директорий для заголовочных файлов
target_include_directories(gen-app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/server
        ${CMAKE_CURRENT_SOURCE_DIR}/client
        ${CMAKE_CURRENT_SOURCE_DIR}/network
        ${CMAKE_CURRENT_SOURCE_DIR}/logging
)
