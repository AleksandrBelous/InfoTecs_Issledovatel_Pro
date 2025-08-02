#include "TCPServer.h"
#include "../network/SocketManager.h"
#include "../network/EpollManager.h"
#include "../logging/LogMacros.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <csignal>
#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <utility>

// Глобальный указатель на сервер для обработчика сигналов
static TCPServer* g_server_instance = nullptr;

TCPServer::TCPServer(ServerConfig config)
    : config_(std::move(config))
      , epoll_manager_(std::make_unique<EpollManager>())
{
    LOG_FUNCTION();
}

TCPServer::~TCPServer()
{
    LOG_FUNCTION();
    if(!running_)
    {
        LOG_MESSAGE("Already shutting down, skipping destructor cleanup");
        return; // Уже завершается
    }
    shutdown();
}

bool TCPServer::initialize()
{
    LOG_FUNCTION();
    // Создаем серверный сокет
    server_fd_ = SocketManager::createServerSocket(config_.getHost(), config_.getPort());
    if(server_fd_ == -1)
    {
        LOG_MESSAGE("Failed to create server socket");
        return false;
    }

    // Инициализируем epoll
    if(!epoll_manager_->initialize())
    {
        LOG_MESSAGE("Failed to initialize epoll");
        SocketManager::closeSocket(server_fd_);
        server_fd_ = -1;
        return false;
    }

    // Добавляем серверный сокет в epoll для ожидания новых подключений
    if(!epoll_manager_->addFileDescriptor(server_fd_, EPOLLIN))
    {
        LOG_MESSAGE("Failed to add server socket to epoll");
        SocketManager::closeSocket(server_fd_);
        server_fd_ = -1;
        return false;
    }

    // Устанавливаем обработчик сигнала SIGINT
    g_server_instance = this;
    signal(SIGINT, signalHandler);

    LOG_MESSAGE("Server initialized successfully");
    std::cout << "[server] Ожидаю подключения... (Ctrl-C для завершения)\n";
    return true;
}

void TCPServer::run()
{
    LOG_FUNCTION();
    if(!isRunning())
    {
        LOG_MESSAGE("Server not initialized, cannot run");
        std::cerr << "[error] Сервер не инициализирован\n";
        return;
    }

    // Главный цикл сервера
    LOG_MESSAGE("Starting main loop");
    while(running_)
    {
        handleEpollEvents();
    }

    LOG_MESSAGE("Main loop ended, calling shutdown");
    shutdown();
}

void TCPServer::shutdown()
{
    LOG_FUNCTION();

    running_ = 0;
    std::cout << "[server] Завершение работы сервера...\n";

    // Закрываем все клиентские соединения
    LOG_MESSAGE("Closing " + std::to_string(client_fds_.size()) + " client connections");
    for(int client_fd : client_fds_)
    {
        std::cout << "[server] Закрываю клиентское соединение (fd=" << client_fd << ")\n";
        (void)epoll_manager_->removeFileDescriptor(client_fd);
        SocketManager::closeSocket(client_fd);
    }
    client_fds_.clear();

    // Закрываем epoll и серверный сокет
    if(server_fd_ >= 0)
    {
        LOG_MESSAGE("Closing server socket fd=" + std::to_string(server_fd_));
        (void)epoll_manager_->removeFileDescriptor(server_fd_);
        SocketManager::closeSocket(server_fd_);
        server_fd_ = -1;
    }

    LOG_MESSAGE("Shutdown completed");
    std::cout << "[server] Сервер остановлен\n";
}

size_t TCPServer::getActiveConnections() const noexcept
{
    return client_fds_.size();
}

void TCPServer::signalHandler(int signum)
{
    LOG_FUNCTION_START();
    (void)signum; // Подавляем предупреждение о неиспользуемом параметре

    if(g_server_instance)
    {
        LOG_MESSAGE("Setting running_ = 0 in signal handler");
        g_server_instance->running_ = 0;
        std::cout << "\n[server] Получен сигнал завершения, закрываю соединения...\n";
        // Принудительно завершаем работу
        // g_server_instance->shutdown();
    }
    else
    {
        LOG_MESSAGE("g_server_instance is null in signal handler");
    }
    LOG_FUNCTION_STOP();
}

void TCPServer::handleNewConnections()
{
    LOG_FUNCTION();
    while(true)
    {
        struct sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = SocketManager::acceptConnection(server_fd_, &client_addr, &client_addr_len);
        if(client_fd == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // Больше нет ожидающих подключений
                LOG_MESSAGE("No more pending connections");
                break;
            }
            std::perror("accept");
            LOG_MESSAGE("accept() failed");
            break;
        }

        // Переводим клиентский сокет в неблокирующий режим
        if(!SocketManager::setNonBlocking(client_fd))
        {
            LOG_MESSAGE("Failed to set non-blocking mode for fd=" + std::to_string(client_fd));
            SocketManager::closeSocket(client_fd);
            continue;
        }

        // Добавляем клиентский сокет в epoll для чтения данных
        // EPOLLRDHUP - для обнаружения закрытия соединения клиентом
        if(!epoll_manager_->addFileDescriptor(client_fd, EPOLLIN | EPOLLRDHUP))
        {
            LOG_MESSAGE("Failed to add client fd=" + std::to_string(client_fd) + " to epoll");
            SocketManager::closeSocket(client_fd);
            continue;
        }

        client_fds_.insert(client_fd);
        LOG_MESSAGE("Added client fd=" + std::to_string(client_fd) + " to client_fds_");

        // Выводим информацию о новом подключении
        std::string client_ip = SocketManager::getClientIP(client_addr);
        uint16_t client_port = SocketManager::getClientPort(client_addr);
        std::cout << "[server] Новое подключение от " << client_ip
            << ":" << client_port
            << " (fd=" << client_fd << ")\n";
    }
}

void TCPServer::handleClientData(int client_fd)
{
    LOG_FUNCTION();
    char buffer[4096];
    bool should_close = false;

    // Читаем все доступные данные от клиента
    while(true)
    {
        ssize_t bytes_received = SocketManager::receiveData(client_fd, buffer, sizeof(buffer));

        if(bytes_received > 0)
        {
            // Данные получены - согласно заданию, просто читаем их
            // (не требуется обработка данных)
            LOG_MESSAGE("Received " + std::to_string(bytes_received) + " bytes from fd=" + std::to_string(client_fd));
            continue;
        }
        else if(bytes_received == 0)
        {
            // Клиент закрыл соединение (EOF)
            LOG_MESSAGE("Client closed connection (EOF) for fd=" + std::to_string(client_fd));
            should_close = true;
            break;
        }
        else
        {
            // bytes_received == -1
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // Больше нет данных для чтения
                LOG_MESSAGE("No more data to read from fd=" + std::to_string(client_fd));
                break;
            }
            std::perror("recv");
            LOG_MESSAGE("recv() failed for fd=" + std::to_string(client_fd));
            should_close = true;
            break;
        }
    }

    // Проверяем, нужно ли закрыть соединение
    if(should_close)
    {
        LOG_MESSAGE("Closing connection for fd=" + std::to_string(client_fd));
        closeClientConnection(client_fd);
    }
}

void TCPServer::handleEpollEvents()
{
    LOG_FUNCTION();
    constexpr int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];

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
                return; // Выходим из цикла, возвращаемся в run() и переходим к shutdown()
            }
            LOG_MESSAGE("running_ is still true, continuing");
            return; // Продолжаем работу, если running_ еще установлен
        }
        std::perror("epoll_wait");
        LOG_MESSAGE("epoll_wait failed");
        return;
    }

    LOG_MESSAGE("Got " + std::to_string(num_events) + " events from epoll");
    for(int i = 0; i < num_events; ++i)
    {
        int fd = events[i].data.fd;
        uint32_t event_flags = events[i].events;

        LOG_MESSAGE("Processing event for fd=" + std::to_string(fd) + " with flags=" + std::to_string(event_flags));

        if(fd == server_fd_)
        {
            // Событие от серверного сокета - новое подключение
            LOG_MESSAGE("Server socket event, handling new connections");
            handleNewConnections();
        }
        else
        {
            // Событие от клиентского сокета
            if(event_flags & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                // Соединение закрыто или произошла ошибка
                LOG_MESSAGE("Client connection error/close for fd=" + std::to_string(fd));
                closeClientConnection(fd);
            }
            else if(event_flags & EPOLLIN)
            {
                // Есть данные для чтения
                LOG_MESSAGE("Client data available for fd=" + std::to_string(fd));
                handleClientData(fd);
            }
        }
    }
}

void TCPServer::closeClientConnection(int client_fd)
{
    LOG_FUNCTION();
    client_fds_.erase(client_fd);
    std::cout << "[server] Соединение закрыто (fd=" << client_fd << ")\n";
    (void)epoll_manager_->removeFileDescriptor(client_fd);
    SocketManager::closeSocket(client_fd);
    LOG_MESSAGE("Client connection fd=" + std::to_string(client_fd) + " closed");
}
