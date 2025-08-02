# CMakeLists.txt для тестов

# Проверка наличия Google Test
find_package(GTest REQUIRED)

# Тесты для gen-app
add_executable(gen_app_tests
        gen_app_tests.cpp
        ../gen-app/logging/Logger.cpp
        ../gen-app/logging/LogManager.cpp
        ../gen-app/network/SocketManager.cpp
        ../gen-app/network/EpollManager.cpp
        ../gen-app/server/ServerConfig.cpp
)

# Тесты для sniffer
add_executable(sniffer_tests
        sniffer_tests.cpp
        ../sniffer/logging/Logger.cpp
        ../sniffer/logging/LogManager.cpp
        ../sniffer/flow_tracker/FlowStats.cpp
        ../sniffer/flow_tracker/FlowTracker.cpp
        ../sniffer/statistics/StatisticsManager.cpp
        ../sniffer/packet_processor/PacketParser.cpp
)

# Привязка библиотек для gen_app_tests
target_link_libraries(gen_app_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
)

# Привязка библиотек для sniffer_tests
target_link_libraries(sniffer_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        pthread
)

# Директории для заголовочных файлов
target_include_directories(gen_app_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../gen-app
        ${CMAKE_CURRENT_SOURCE_DIR}/../gen-app/logging
        ${CMAKE_CURRENT_SOURCE_DIR}/../gen-app/network
        ${CMAKE_CURRENT_SOURCE_DIR}/../gen-app/server
        ${CMAKE_CURRENT_SOURCE_DIR}/../gen-app/client
)

target_include_directories(sniffer_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../sniffer
        ${CMAKE_CURRENT_SOURCE_DIR}/../sniffer/logging
        ${CMAKE_CURRENT_SOURCE_DIR}/../sniffer/flow_tracker
        ${CMAKE_CURRENT_SOURCE_DIR}/../sniffer/statistics
        ${CMAKE_CURRENT_SOURCE_DIR}/../sniffer/packet_processor
)

# Добавление тестов в CTest
enable_testing()
add_test(NAME GenAppTests COMMAND gen_app_tests)
add_test(NAME SnifferTests COMMAND sniffer_tests) 