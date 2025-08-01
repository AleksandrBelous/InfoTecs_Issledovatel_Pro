#include "TCPClient.h"
#include "SocketManager.h"
#include "EpollManager.h"
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
}

TCPClient::~TCPClient()
{
    if(running_)
    {
        shutdown();
    }
}

bool TCPClient::initialize()
{
    // Инициализируем epoll
    if(!epoll_manager_->initialize())
    {
        std::cerr << "[error] Не удалось инициализировать epoll\n";
        return false;
    }

    // Устанавливаем обработчик сигнала SIGINT
    g_client_instance = this;
    signal(SIGINT, signalHandler);

    // Проверяем доступность сервера перед созданием соединений
    int test_fd = SocketManager::createClientSocket(config_.host, config_.port);
    if(test_fd == -1)
    {
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
            std::cerr << "[error] Сервер недоступен: " << config_.host << ":" << config_.port << "\n";
            SocketManager::closeSocket(test_fd);
            return false;
        }
    }
    SocketManager::closeSocket(test_fd);

    // Создаем начальные соединения
    for(size_t i = 0; i < config_.connections; ++i)
    {
        Connection conn;
        if(!startConnection(conn))
        {
            std::cerr << "[error] Не удалось создать соединение " << (i + 1) << "\n";
            return false;
        }
        connections_.emplace(conn.fd, conn);
    }

    checkConnectionCount();

    std::cout << "[client] Инициализирован клиент: " << config_.toString() << "\n";
    std::cout << "[client] Создано соединений: " << connections_.size()
        << " к " << config_.host << ':' << config_.port
        << " (Ctrl-C для завершения)" << std::endl;
    return true;
}

void TCPClient::run()
{
    if(!isRunning())
    {
        std::cerr << "[error] Клиент не инициализирован\n";
        return;
    }

    // Главный цикл клиента
    while(running_)
    {
        handleEpollEvents();
    }

    shutdown();
}

void TCPClient::shutdown()
{
    if(!running_)
    {
        return; // Уже завершается
    }

    running_ = 0;
    std::cout << "[client] Завершение работы клиента...\n";

    // Закрываем все соединения
    for(const auto& fd : connections_ | std::views::keys)
    {
        std::cout << "[client] Закрываю соединение (fd=" << fd << ")\n";
        (void)epoll_manager_->removeFileDescriptor(fd);
        SocketManager::closeSocket(fd);
    }
    connections_.clear();

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
    (void)signum; // Подавляем предупреждение о неиспользуемом параметре

    if(g_client_instance)
    {
        g_client_instance->running_ = 0;
        std::cout << "\n[client] Получен сигнал завершения, закрываю соединения...\n";
        // Принудительно завершаем работу
        g_client_instance->shutdown();
    }
}

bool TCPClient::startConnection(Connection& conn)
{
    // Создаем клиентский сокет и инициируем подключение
    int fd = SocketManager::createClientSocket(config_.host, config_.port);
    if(fd == -1)
    {
        std::cerr << "[error] Не удалось создать сокет для подключения к "
            << config_.host << ":" << config_.port << "\n";
        return false;
    }

    // Генерируем случайное количество байт для отправки (32-1024 байта)
    std::uniform_int_distribution<size_t> dist(32, 1024);
    conn.fd = fd;
    conn.total_bytes = dist(rng_);
    conn.bytes_sent = 0;
    conn.connecting = true;

    // Добавляем сокет в epoll для отслеживания событий подключения и записи
    if(!epoll_manager_->addFileDescriptor(fd, EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    {
        std::cerr << "[error] Не удалось добавить сокет в epoll: fd=" << fd << "\n";
        SocketManager::closeSocket(fd);
        return false;
    }

    std::cout << "[client] Открыто соединение: fd=" << fd
        << " (будет отправлено " << conn.total_bytes << " байт)\n";
    return true;
}

void TCPClient::checkConnectionCount()
{
    // Выводим информацию об изменении количества соединений
    if(last_connection_count_ != connections_.size())
    {
        last_connection_count_ = connections_.size();
        std::cout << "[client] Активных соединений: " << last_connection_count_
            << "/" << config_.connections << std::endl;
    }
}

void TCPClient::restartConnection(int fd)
{
    auto it = connections_.find(fd);
    if(it == connections_.end())
    {
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
        checkConnectionCount();
    }
    else
    {
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
            std::cerr << "[error] Нет активных соединений. Сервер может быть недоступен.\n";
            std::cerr << "[error] Завершение работы клиента.\n";
            running_ = 0;
        }
    }
}

void TCPClient::handleEpollEvents()
{
    constexpr int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];

    // Ожидаем события от epoll
    int num_events = epoll_manager_->waitForEvents(events, MAX_EVENTS, -1);
    if(num_events == -1)
    {
        if(errno == EINTR)
        {
            // Прервано сигналом - проверяем, нужно ли завершить работу
            if(!running_)
            {
                return; // Выходим из цикла, shutdown() будет вызван в run()
            }
            return; // Продолжаем работу, если running_ еще установлен
        }
        std::perror("epoll_wait");
        running_ = 0;
        return;
    }

    // Обрабатываем все полученные события
    for(int i = 0; i < num_events; ++i)
    {
        handleConnectionEvent(events[i].data.fd, events[i].events);
    }

    checkConnectionCount();
}

void TCPClient::handleConnectionEvent(int fd, uint32_t events)
{
    auto it = connections_.find(fd);
    if(it == connections_.end())
    {
        return;
    }

    Connection& conn = it->second;

    // Проверяем ошибки соединения
    if(events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    {
        std::cout << "[client] Соединение разорвано сервером: fd=" << fd << "\n";
        restartConnection(fd);
        return;
    }

    // Если соединение еще подключается, проверяем статус подключения
    if(conn.connecting)
    {
        int err = 0;
        socklen_t len = sizeof(err);
        if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err != 0)
        {
            std::cerr << "[error] Ошибка подключения (fd=" << fd << "): " << strerror(err) << "\n";
            if(err == ECONNREFUSED)
            {
                std::cerr << "[error] Сервер недоступен. Завершение работы клиента.\n";
                running_ = 0;
                return;
            }
            restartConnection(fd);
            return;
        }
        conn.connecting = false;
        std::cout << "[client] Соединение установлено: fd=" << fd << "\n";
    }

    // Если сокет готов к записи, отправляем данные
    if(events & EPOLLOUT)
    {
        // Дополнительная проверка состояния соединения перед отправкой
        if(conn.connecting)
        {
            // Соединение еще не установлено, пропускаем отправку
            return;
        }

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
            }
            else if(bytes_sent == -1)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // Буфер отправки заполнен, попробуем позже
                    break;
                }
                else if(errno == EPIPE || errno == ECONNRESET)
                {
                    // Соединение разорвано сервером
                    std::cout << "[client] Соединение разорвано сервером при отправке: fd=" << fd << "\n";
                    restartConnection(fd);
                    return;
                }
                std::perror("send");
                restartConnection(fd);
                return;
            }
        }

        // Если все данные отправлены, закрываем соединение
        if(conn.bytes_sent >= conn.total_bytes)
        {
            std::cout << "[client] Данные отправлены: fd=" << fd
                << " (" << conn.bytes_sent << "/" << conn.total_bytes << " байт)\n";
            restartConnection(fd);
        }
    }
}
