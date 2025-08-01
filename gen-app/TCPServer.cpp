#include "TCPServer.h"
#include "SocketManager.h"
#include "EpollManager.h"
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
      , running_(1)
{
}

TCPServer::~TCPServer()
{
    shutdown();
}

bool TCPServer::initialize()
{
    // Создаем серверный сокет
    server_fd_ = SocketManager::createServerSocket(config_.getHost(), config_.getPort());
    if(server_fd_ == -1)
    {
        return false;
    }

    // Инициализируем epoll
    if(!epoll_manager_->initialize())
    {
        SocketManager::closeSocket(server_fd_);
        server_fd_ = -1;
        return false;
    }

    // Добавляем серверный сокет в epoll для ожидания новых подключений
    if(!epoll_manager_->addFileDescriptor(server_fd_, EPOLLIN))
    {
        SocketManager::closeSocket(server_fd_);
        server_fd_ = -1;
        return false;
    }

    // Устанавливаем обработчик сигнала SIGINT
    g_server_instance = this;
    signal(SIGINT, signalHandler);

    std::cout << "[server] Ожидаю подключения... (Ctrl-C для завершения)\n";
    return true;
}

void TCPServer::run()
{
    if(!isRunning())
    {
        std::cerr << "[error] Сервер не инициализирован\n";
        return;
    }

    // Главный цикл сервера
    while(running_)
    {
        handleEpollEvents();
    }

    shutdown();
}

void TCPServer::shutdown()
{
    if(!running_)
    {
        return; // Уже завершается
    }

    running_ = 0;
    std::cout << "[server] Завершение работы сервера...\n";

    // Закрываем все клиентские соединения
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
        SocketManager::closeSocket(server_fd_);
        server_fd_ = -1;
    }

    std::cout << "[server] Сервер остановлен\n";
}

size_t TCPServer::getActiveConnections() const noexcept
{
    return client_fds_.size();
}

void TCPServer::signalHandler(int signum)
{
    (void)signum; // Подавляем предупреждение о неиспользуемом параметре

    if(g_server_instance)
    {
        g_server_instance->running_ = 0;
        std::cout << "\n[server] Получен сигнал завершения, закрываю соединения...\n";
    }
}

void TCPServer::handleNewConnections()
{
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
                break;
            }
            std::perror("accept");
            break;
        }

        // Переводим клиентский сокет в неблокирующий режим
        if(!SocketManager::setNonBlocking(client_fd))
        {
            SocketManager::closeSocket(client_fd);
            continue;
        }

        // Добавляем клиентский сокет в epoll для чтения данных
        // EPOLLRDHUP - для обнаружения закрытия соединения клиентом
        if(!epoll_manager_->addFileDescriptor(client_fd, EPOLLIN | EPOLLRDHUP))
        {
            SocketManager::closeSocket(client_fd);
            continue;
        }

        client_fds_.insert(client_fd);

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
            continue;
        }
        else if(bytes_received == 0)
        {
            // Клиент закрыл соединение (EOF)
            should_close = true;
            break;
        }
        else
        {
            // bytes_received == -1
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // Больше нет данных для чтения
                break;
            }
            std::perror("recv");
            should_close = true;
            break;
        }
    }

    // Проверяем, нужно ли закрыть соединение
    if(should_close)
    {
        closeClientConnection(client_fd);
    }
}

void TCPServer::handleEpollEvents()
{
    constexpr int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];

    int num_events = epoll_manager_->waitForEvents(events, MAX_EVENTS, -1);
    if(num_events == -1)
    {
        if(errno == EINTR)
        {
            // Прервано сигналом - это нормально
            return;
        }
        std::perror("epoll_wait");
        return;
    }

    for(int i = 0; i < num_events; ++i)
    {
        int fd = events[i].data.fd;
        uint32_t event_flags = events[i].events;

        if(fd == server_fd_)
        {
            // Событие от серверного сокета - новое подключение
            handleNewConnections();
        }
        else
        {
            // Событие от клиентского сокета
            if(event_flags & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                // Соединение закрыто или произошла ошибка
                closeClientConnection(fd);
            }
            else if(event_flags & EPOLLIN)
            {
                // Есть данные для чтения
                handleClientData(fd);
            }
        }
    }
}

void TCPServer::closeClientConnection(int client_fd)
{
    client_fds_.erase(client_fd);
    std::cout << "[server] Соединение закрыто (fd=" << client_fd << ")\n";
    (void)epoll_manager_->removeFileDescriptor(client_fd);
    SocketManager::closeSocket(client_fd);
}
