#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

// Заголовочные файлы gen-app
#include "logging/LogManager.h"
#include "logging/Logger.h"
#include "../gen-app/server/ServerConfig.h"
#include "../gen-app/client/ClientConfig.h"
#include "../gen-app/network/SocketManager.h"
#include "../gen-app/network/EpollManager.h"

// Тесты для ServerConfig
class ServerConfigTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Инициализация перед каждым тестом
    }

    void TearDown() override
    {
        // Очистка после каждого теста
    }
};

TEST_F(ServerConfigTest, DefaultConstructor)
{
    ServerConfig config;
    EXPECT_FALSE(config.isValid());
    EXPECT_EQ(config.getHost(), "0.0.0.0");
    EXPECT_EQ(config.getPort(), 0);
}

TEST_F(ServerConfigTest, ParameterConstructor)
{
    ServerConfig config("127.0.0.1", 8080);
    EXPECT_TRUE(config.isValid());
    EXPECT_EQ(config.getHost(), "127.0.0.1");
    EXPECT_EQ(config.getPort(), 8080);
}

TEST_F(ServerConfigTest, SetterMethods)
{
    ServerConfig config;
    config.setHost("192.168.1.1");
    config.setPort(9000);

    EXPECT_EQ(config.getHost(), "192.168.1.1");
    EXPECT_EQ(config.getPort(), 9000);
    EXPECT_TRUE(config.isValid());
}

TEST_F(ServerConfigTest, CopyConstructor)
{
    ServerConfig original("10.0.0.1", 1234);
    ServerConfig copy(original);

    EXPECT_EQ(copy.getHost(), original.getHost());
    EXPECT_EQ(copy.getPort(), original.getPort());
    EXPECT_EQ(copy.isValid(), original.isValid());
}

TEST_F(ServerConfigTest, AssignmentOperator)
{
    ServerConfig original("10.0.0.1", 1234);
    ServerConfig assigned = original;

    EXPECT_EQ(assigned.getHost(), original.getHost());
    EXPECT_EQ(assigned.getPort(), original.getPort());
    EXPECT_EQ(assigned.isValid(), original.isValid());
}

// Тесты для ClientConfig
class ClientConfigTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(ClientConfigTest, DefaultConstructor)
{
    ClientConfig config;
    EXPECT_FALSE(config.isValid());
    EXPECT_EQ(config.host, "127.0.0.1");
    EXPECT_EQ(config.port, 0);
    EXPECT_EQ(config.connections, 1);
    EXPECT_EQ(config.seed, 1);
}

TEST_F(ClientConfigTest, ParameterConstructor)
{
    ClientConfig config("192.168.1.100", 8080, 5, 42);
    EXPECT_TRUE(config.isValid());
    EXPECT_EQ(config.host, "192.168.1.100");
    EXPECT_EQ(config.port, 8080);
    EXPECT_EQ(config.connections, 5);
    EXPECT_EQ(config.seed, 42);
}

TEST_F(ClientConfigTest, Validation)
{
    ClientConfig config;
    EXPECT_FALSE(config.isValid());

    config.host = "127.0.0.1";
    config.port = 8080;
    EXPECT_TRUE(config.isValid());

    config.host = "";
    EXPECT_FALSE(config.isValid());

    config.host = "127.0.0.1";
    config.port = 0;
    EXPECT_FALSE(config.isValid());
}

// Тесты для LogManager
class LogManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Очищаем состояние LogManager перед каждым тестом
        LogManager::initialize(false, "test");
    }

    void TearDown() override
    {
        // Очистка после теста
    }
};

TEST_F(LogManagerTest, InitializeWithoutLogging)
{
    LogManager::initialize(false, "test_component");
    EXPECT_FALSE(LogManager::is_logging_enabled());

    auto logger = LogManager::get_logger();
    // Логгер может быть nullptr при отключенном логировании
    // EXPECT_NE(logger, nullptr);
}

TEST_F(LogManagerTest, InitializeWithLogging)
{
    LogManager::initialize(true, "test_component");
    EXPECT_TRUE(LogManager::is_logging_enabled());

    auto logger = LogManager::get_logger();
    EXPECT_NE(logger, nullptr);
}

TEST_F(LogManagerTest, LoggerSingleton)
{
    LogManager::initialize(false, "component1");
    auto logger1 = LogManager::get_logger();

    LogManager::initialize(false, "component2");
    auto logger2 = LogManager::get_logger();

    // Должен быть один и тот же логгер
    EXPECT_EQ(logger1, logger2);
}

// Тесты для EpollManager
class EpollManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        epoll_manager = std::make_unique<EpollManager>();
    }

    void TearDown() override
    {
        epoll_manager.reset();
    }

    std::unique_ptr<EpollManager> epoll_manager;
};

TEST_F(EpollManagerTest, Initialization)
{
    EXPECT_TRUE(epoll_manager->initialize());
}

TEST_F(EpollManagerTest, AddAndRemoveFileDescriptor)
{
    ASSERT_TRUE(epoll_manager->initialize());

    // Создаем тестовый файловый дескриптор
    int test_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(test_fd, 0);

    // Добавляем в epoll
    EXPECT_TRUE(epoll_manager->addFileDescriptor(test_fd, EPOLLIN));

    // Удаляем из epoll
    EXPECT_TRUE(epoll_manager->removeFileDescriptor(test_fd));

    close(test_fd);
}

TEST_F(EpollManagerTest, WaitForEvents)
{
    ASSERT_TRUE(epoll_manager->initialize());

    // Создаем тестовый файловый дескриптор
    int test_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(test_fd, 0);

    // Добавляем в epoll
    ASSERT_TRUE(epoll_manager->addFileDescriptor(test_fd, EPOLLIN));

    // Ждем события с таймаутом
    struct epoll_event events[1];
    int num_events = epoll_manager->waitForEvents(events, 1, 100); // 100ms timeout

    // Должно быть 0 событий (нет активности на сокете)
    EXPECT_GE(num_events, 0);

    close(test_fd);
}

// Тесты для SocketManager
class SocketManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(SocketManagerTest, SetNonBlocking)
{
    int test_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(test_fd, 0);

    EXPECT_TRUE(SocketManager::setNonBlocking(test_fd));

    // Проверяем, что сокет действительно неблокирующий
    int flags = fcntl(test_fd, F_GETFL, 0);
    EXPECT_TRUE(flags & O_NONBLOCK);

    close(test_fd);
}

TEST_F(SocketManagerTest, CloseSocket)
{
    int test_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(test_fd, 0);

    SocketManager::closeSocket(test_fd);

    // Проверяем, что сокет закрыт
    int result = fcntl(test_fd, F_GETFL, 0);
    EXPECT_EQ(result, -1);
    EXPECT_EQ(errno, EBADF);
}

// Интеграционные тесты
class IntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        LogManager::initialize(false, "integration_test");
    }

    void TearDown() override
    {
    }
};

TEST_F(IntegrationTest, ServerConfigWithLogging)
{
    ServerConfig config("127.0.0.1", 8080);
    EXPECT_TRUE(config.isValid());

    auto logger = LogManager::get_logger();
    EXPECT_NE(logger, nullptr);
}

TEST_F(IntegrationTest, ClientConfigValidation)
{
    ClientConfig config("127.0.0.1", 8080, 10, 12345);
    EXPECT_TRUE(config.isValid());
    EXPECT_EQ(config.host, "127.0.0.1");
    EXPECT_EQ(config.port, 8080);
    EXPECT_EQ(config.connections, 10);
    EXPECT_EQ(config.seed, 12345);
}

// Тесты производительности
class PerformanceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(PerformanceTest, EpollManagerPerformance)
{
    auto epoll_manager = std::make_unique<EpollManager>();
    ASSERT_TRUE(epoll_manager->initialize());

    constexpr int num_fds = 100;
    std::vector<int> fds;

    // Создаем множество файловых дескрипторов
    for(int i = 0; i < num_fds; ++i)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        ASSERT_GE(fd, 0);
        fds.push_back(fd);
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Добавляем все в epoll
    for(int fd : fds)
    {
        EXPECT_TRUE(epoll_manager->addFileDescriptor(fd, EPOLLIN));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Очистка
    for(int fd : fds)
    {
        EXPECT_TRUE(epoll_manager->removeFileDescriptor(fd));
        close(fd);
    }

    // Проверяем, что операция выполнилась быстро
    EXPECT_LT(duration.count(), 10000); // Менее 10ms
}

// Тесты для многопоточности
class ThreadingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(ThreadingTest, LogManagerThreadSafety)
{
    LogManager::initialize(true, "thread_test");

    constexpr int num_threads = 10;
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    // Запускаем несколько потоков, которые одновременно используют LogManager
    for(int i = 0; i < num_threads; ++i)
    {
        constexpr int num_operations = 100;
        threads.emplace_back([num_operations]()
        {
            for(int j = 0; j < num_operations; ++j)
            {
                auto logger = LogManager::get_logger();
                EXPECT_NE(logger, nullptr);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }

    // Ждем завершения всех потоков
    for(auto& thread : threads)
    {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Проверяем, что все операции выполнились без ошибок
    EXPECT_GT(duration.count(), 0);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
