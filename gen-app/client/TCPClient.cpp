#include "TCPClient.h"
#include "../network/SocketManager.h"
#include "../network/EpollManager.h"
#include "../logging/LogMacros.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <ranges>

// Глобальный указатель на клиент для обработчика сигналов
static TCPClient* g_client_instance = nullptr;

TCPClient::TCPClient(ClientConfig config)
    : config_(std::move(config))
      , epoll_manager_(std::make_unique<EpollManager>())
      , rng_(config_.seed)
{
    LOG_FUNCTION();
}

TCPClient::~TCPClient()
{
    LOG_FUNCTION();
    if(running_)
    {
        LOG_MESSAGE("Still running, calling shutdown from destructor");
        shutdown();
    }
}

bool TCPClient::initialize()
{
    LOG_FUNCTION();
    // Инициализируем epoll
    if(!epoll_manager_->initialize())
    {
        LOG_MESSAGE("Failed to initialize epoll");
        std::cerr << "[error] Не удалось инициализировать epoll\n";
        return false;
    }

    // Устанавливаем обработчик сигнала SIGINT
    g_client_instance = this;
    signal(SIGINT, signalHandler);

    // Проверяем доступность сервера перед созданием соединений
    LOG_MESSAGE("Checking server availability at " + config_.host + ":" + std::to_string(config_.port));
    int test_fd = SocketManager::createClientSocket(config_.host, config_.port);
    if(test_fd == -1)
    {
        LOG_MESSAGE("Server unavailable: cannot create test socket");
        std::cerr << "[error] Сервер недоступен: " << config_.host << ":" << config_.port << "\n";
        return false;
    }

    // Проверяем статус подключения
    int err = 0;
    socklen_t len = sizeof(err);
    if(getsockopt(test_fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err != 0)
    {
        if(err == ECONNREFUSED)
        {
            LOG_MESSAGE("Server refused connection");
            std::cerr << "[error] Сервер недоступен: " << config_.host << ":" << config_.port << "\n";
            SocketManager::closeSocket(test_fd);
            return false;
        }
    }
    SocketManager::closeSocket(test_fd);
    LOG_MESSAGE("Server availability check passed");

    // Создаем начальные соединения
    LOG_MESSAGE("Creating " + std::to_string(config_.connections) + " initial connections");
    for(size_t i = 0; i < config_.connections; ++i)
    {
        Connection conn;
        if(!startConnection(conn))
        {
            LOG_MESSAGE("Failed to create connection " + std::to_string(i + 1));
            std::cerr << "[error] Не удалось создать соединение " << (i + 1) << "\n";
            return false;
        }
        connections_.emplace(conn.fd, conn);
    }

    checkConnectionCount();

    LOG_MESSAGE("Client initialized successfully");
    std::cout << "[client] Инициализирован клиент: " << config_.toString() << "\n";
    std::cout << "[client] Создано соединений: " << connections_.size()
        << " к " << config_.host << ':' << config_.port
        << " (Ctrl-C для завершения)" << std::endl;
    return true;
}

void TCPClient::run()
{
    LOG_FUNCTION();
    if(!isRunning())
    {
        LOG_MESSAGE("Client not initialized, cannot run");
        std::cerr << "[error] Клиент не инициализирован\n";
        return;
    }

    // Главный цикл клиента
    LOG_MESSAGE("Starting main loop");
    while(running_)
    {
        handleEpollEvents();
    }

    LOG_MESSAGE("Main loop ended, calling shutdown");
    shutdown();
}

void TCPClient::shutdown()
{
    LOG_FUNCTION();
    if(!running_)
    {
        LOG_MESSAGE("Already shutting down, skipping");
        return; // Уже завершается
    }

    running_ = 0;
    std::cout << "[client] Завершение работы клиента...\n";

    // Закрываем все соединения
    LOG_MESSAGE("Closing " + std::to_string(connections_.size()) + " connections");
    for(const auto& fd : connections_ | std::views::keys)
    {
        std::cout << "[client] Закрываю соединение (fd=" << fd << ")\n";
        (void)epoll_manager_->removeFileDescriptor(fd);
        SocketManager::closeSocket(fd);
    }
    connections_.clear();

    LOG_MESSAGE("Shutdown completed");
    std::cout << "[client] Клиент остановлен\n";
}

std::string TCPClient::getStats() const
{
    size_t total_bytes_to_send = 0;
    size_t total_bytes_sent = 0;

    for(const auto& [fd, conn] : connections_)
    {
        total_bytes_to_send += conn.total_bytes;
        total_bytes_sent += conn.bytes_sent;
    }

    return "Активных соединений: " + std::to_string(connections_.size()) +
        "/" + std::to_string(config_.connections) +
        ", Отправлено байт: " + std::to_string(total_bytes_sent) +
        "/" + std::to_string(total_bytes_to_send);
}

void TCPClient::signalHandler(int signum)
{
    LOG_FUNCTION_START();
    (void)signum; // Подавляем предупреждение о неиспользуемом параметре

    if(g_client_instance)
    {
        LOG_MESSAGE("Setting running_ = 0 in signal handler");
        g_client_instance->running_ = 0;
        std::cout << "\n[client] Получен сигнал завершения, закрываю соединения...\n";
        // Принудительно завершаем работу
        g_client_instance->shutdown();
    }
    else
    {
        LOG_MESSAGE("g_client_instance is null in signal handler");
    }
    LOG_FUNCTION_STOP();
}

bool TCPClient::startConnection(Connection& conn)
{
    LOG_FUNCTION();
    // Создаем клиентский сокет и инициируем подключение
    int fd = SocketManager::createClientSocket(config_.host, config_.port);
    if(fd == -1)
    {
        LOG_MESSAGE("Failed to create client socket");
        std::cerr << "[error] Не удалось создать сокет для подключения к "
            << config_.host << ":" << config_.port << "\n";
        return false;
    }

    // Если не удается создать соединение, проверяем доступность сервера
    // Проверяем статус подключения
    int err = 0;
    socklen_t len = sizeof(err);
    if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err != 0)
    {
        if(err == ECONNREFUSED)
        {
            LOG_MESSAGE("Server refused connection during startConnection");
            std::cerr << "[error] Сервер недоступен. Завершение работы клиента.\n";
            SocketManager::closeSocket(fd);
            running_ = 0;
            return false;
        }
    }

    // Генерируем случайное количество байт для отправки (32-1024 байта)
    std::uniform_int_distribution<size_t> dist(32, 1024);
    conn.fd = fd;
    conn.total_bytes = dist(rng_);
    conn.bytes_sent = 0;
    conn.is_connecting = true;

    // Добавляем сокет в epoll для отслеживания событий подключения и записи
    if(!epoll_manager_->addFileDescriptor(fd, EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    {
        LOG_MESSAGE("Failed to add socket fd=" + std::to_string(fd) + " to epoll");
        std::cerr << "[error] Не удалось добавить сокет в epoll: fd=" << fd << "\n";
        SocketManager::closeSocket(fd);
        return false;
    }

    LOG_MESSAGE(
        "Connection started: fd=" + std::to_string(fd) + " (will send " + std::to_string(conn.total_bytes) + " bytes)");
    std::cout << "[client] Открыто соединение: fd=" << fd
        << " (будет отправлено " << conn.total_bytes << " байт)\n";
    return true;
}

void TCPClient::checkConnectionCount()
{
    // Выводим информацию об изменении количества соединений
    if(last_connection_count_ != connections_.size())
    {
        LOG_MESSAGE(
            "Connection count changed: " + std::to_string(last_connection_count_) + " -> " + std::to_string(connections_
                .size()));
        last_connection_count_ = connections_.size();
        std::cout << "[client] Активных соединений: " << last_connection_count_
            << "/" << config_.connections << std::endl;
    }
}

void TCPClient::restartConnection(int fd)
{
    LOG_FUNCTION();
    auto it = connections_.find(fd);
    if(it == connections_.end())
    {
        LOG_MESSAGE("Connection fd=" + std::to_string(fd) + " not found in connections_");
        return;
    }

    std::cout << "[client] Закрыто соединение: fd=" << fd << '\n';
    (void)epoll_manager_->removeFileDescriptor(fd);
    SocketManager::closeSocket(fd);
    connections_.erase(it);

    // Создаем новое соединение для поддержания постоянного количества
    Connection conn;
    if(startConnection(conn))
    {
        connections_.emplace(conn.fd, conn);
        LOG_MESSAGE("Successfully recreated connection: fd=" + std::to_string(conn.fd));
        checkConnectionCount();
    }
    else
    {
        LOG_MESSAGE("Failed to recreate connection");
        std::cerr << "[error] Не удалось пересоздать соединение\n";
        // Если не удается создать соединение, проверяем доступность сервера
        int test_fd = SocketManager::createClientSocket(config_.host, config_.port);
        if(test_fd != -1)
        {
            // Проверяем статус подключения
            int err = 0;
            socklen_t len = sizeof(err);
            if(getsockopt(test_fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err != 0)
            {
                if(err == ECONNREFUSED)
                {
                    LOG_MESSAGE("Server unavailable during restart, shutting down");
                    std::cerr << "[error] Сервер недоступен. Завершение работы клиента.\n";
                    SocketManager::closeSocket(test_fd);
                    running_ = 0;
                    return;
                }
            }
            SocketManager::closeSocket(test_fd);
        }

        // Проверяем, есть ли еще активные соединения
        if(connections_.empty())
        {
            LOG_MESSAGE("No active connections, server may be unavailable");
            std::cerr << "[error] Нет активных соединений. Сервер может быть недоступен.\n";
            std::cerr << "[error] Завершение работы клиента.\n";
            running_ = 0;
        }
    }
}

void TCPClient::handleEpollEvents()
{
    LOG_FUNCTION();
    constexpr int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];

    // Ожидаем события от epoll
    int num_events = epoll_manager_->waitForEvents(events, MAX_EVENTS, -1);
    if(num_events == -1)
    {
        if(errno == EINTR)
        {
            // Прервано сигналом - проверяем, нужно ли завершить работу
            LOG_MESSAGE("epoll_wait interrupted by signal");
            if(!running_)
            {
                LOG_MESSAGE("running_ is false, exiting handleEpollEvents");
                return; // Выходим из цикла, shutdown() будет вызван в run()
            }
            LOG_MESSAGE("running_ is still true, continuing");
            return; // Продолжаем работу, если running_ еще установлен
        }
        std::perror("epoll_wait");
        LOG_MESSAGE("epoll_wait failed");
        running_ = 0;
        return;
    }

    LOG_MESSAGE("Got " + std::to_string(num_events) + " events from epoll");
    // Обрабатываем все полученные события
    for(int i = 0; i < num_events; ++i)
    {
        handleConnectionEvent(events[i].data.fd, events[i].events);
    }

    checkConnectionCount();
}

void TCPClient::handleConnectionEvent(int fd, uint32_t events)
{
    LOG_FUNCTION();
    auto it = connections_.find(fd);
    if(it == connections_.end())
    {
        LOG_MESSAGE("Connection fd=" + std::to_string(fd) + " not found in connections_");
        return;
    }

    Connection& conn = it->second;

    // Проверяем ошибки соединения
    if(events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    {
        LOG_MESSAGE("Connection error/close for fd=" + std::to_string(fd));
        std::cout << "[client] Соединение разорвано сервером: fd=" << fd << "\n";
        restartConnection(fd);
        return;
    }

    // Если соединение еще подключается, проверяем статус подключения
    if(conn.is_connecting)
    {
        LOG_MESSAGE("Checking connection status for fd=" + std::to_string(fd));
        int err = 0;
        socklen_t len = sizeof(err);
        if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err != 0)
        {
            LOG_MESSAGE("Connection error for fd=" + std::to_string(fd) + ": " + strerror(err));
            std::cerr << "[error] Ошибка подключения (fd=" << fd << "): " << strerror(err) << "\n";
            if(err == ECONNREFUSED)
            {
                LOG_MESSAGE("Server refused connection, shutting down");
                std::cerr << "[error] Сервер недоступен. Завершение работы клиента.\n";
                running_ = 0;
                return;
            }
            restartConnection(fd);
            return;
        }
        conn.is_connecting = false;
        LOG_MESSAGE("Connection established for fd=" + std::to_string(fd));
        std::cout << "[client] Соединение установлено: fd=" << fd << "\n";
    }

    // Если сокет готов к записи, отправляем данные
    if(events & EPOLLOUT)
    {
        // Дополнительная проверка состояния соединения перед отправкой
        if(conn.is_connecting)
        {
            LOG_MESSAGE("Connection still connecting, skipping data send for fd=" + std::to_string(fd));
            // Соединение еще не установлено, пропускаем отправку
            return;
        }

        LOG_MESSAGE(
            "Sending data for fd=" + std::to_string(fd) + " (" + std::to_string(conn.bytes_sent) + "/" + std::to_string(
                conn.total_bytes) + " bytes)");
        // Статический буфер с нулями для отправки
        static constexpr char data[1024] = {};

        // Отправляем данные порциями
        while(conn.bytes_sent < conn.total_bytes)
        {
            size_t to_send = std::min(sizeof(data), conn.total_bytes - conn.bytes_sent);
            ssize_t bytes_sent = SocketManager::sendData(fd, data, to_send);

            if(bytes_sent > 0)
            {
                conn.bytes_sent += static_cast<size_t>(bytes_sent);
                LOG_MESSAGE("Sent " + std::to_string(bytes_sent) + " bytes for fd=" + std::to_string(fd));
            }
            else if(bytes_sent == -1)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // Буфер отправки заполнен, попробуем позже
                    LOG_MESSAGE("Send buffer full for fd=" + std::to_string(fd) + ", will try later");
                    break;
                }
                else if(errno == EPIPE || errno == ECONNRESET)
                {
                    // Соединение разорвано сервером
                    LOG_MESSAGE("Connection broken by server during send for fd=" + std::to_string(fd));
                    std::cout << "[client] Соединение разорвано сервером при отправке: fd=" << fd << "\n";
                    restartConnection(fd);
                    return;
                }
                LOG_MESSAGE("Send failed for fd=" + std::to_string(fd) + ": " + strerror(errno));
                std::perror("send");
                restartConnection(fd);
                return;
            }
        }

        // Если все данные отправлены, закрываем соединение
        if(conn.bytes_sent >= conn.total_bytes)
        {
            LOG_MESSAGE("All data sent for fd=" + std::to_string(fd));
            std::cout << "[client] Данные отправлены: fd=" << fd
                << " (" << conn.bytes_sent << "/" << conn.total_bytes << " байт)\n";
            restartConnection(fd);
        }
    }
}
