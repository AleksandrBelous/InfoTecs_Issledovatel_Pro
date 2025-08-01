#include "TCPClient.h"
#include "SocketManager.h"
#include "EpollManager.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <cerrno>

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
    if(!epoll_manager_->initialize())
    {
        return false;
    }

    g_client_instance = this;
    signal(SIGINT, signalHandler);

    for(size_t i = 0; i < config_.connections; ++i)
    {
        Connection conn;
        if(startConnection(conn))
        {
            connections_.emplace(conn.fd, std::move(conn));
        }
        else
        {
            return false;
        }
    }

    std::cout << "[client] Создано соединений: " << connections_.size()
        << " к " << config_.host << ':' << config_.port
        << " (Ctrl-C для завершения)" << std::endl;
    return true;
}

void TCPClient::run()
{
    while(running_)
    {
        handleEpollEvents();
    }
    shutdown();
}

void TCPClient::shutdown()
{
    running_ = 0;
    for(auto& [fd, conn] : connections_)
    {
        (void)epoll_manager_->removeFileDescriptor(fd);
        SocketManager::closeSocket(fd);
    }
    connections_.clear();
}

void TCPClient::signalHandler(int /*signum*/)
{
    if(g_client_instance)
    {
        g_client_instance->running_ = 0;
        std::cout << "\n[client] Завершение работы..." << std::endl;
    }
}

bool TCPClient::startConnection(Connection& conn)
{
    int fd = SocketManager::createClientSocket(config_.host, config_.port);
    if(fd == -1)
    {
        return false;
    }

    std::uniform_int_distribution<size_t> dist(32, 1024);
    conn.fd = fd;
    conn.total_bytes = dist(rng_);
    conn.bytes_sent = 0;
    conn.connecting = true;

    if(!epoll_manager_->addFileDescriptor(fd, EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    {
        SocketManager::closeSocket(fd);
        return false;
    }
    return true;
}

void TCPClient::restartConnection(int fd)
{
    auto it = connections_.find(fd);
    if(it == connections_.end())
    {
        return;
    }
    (void)epoll_manager_->removeFileDescriptor(fd);
    SocketManager::closeSocket(fd);
    connections_.erase(it);

    Connection conn;
    if(startConnection(conn))
    {
        connections_.emplace(conn.fd, std::move(conn));
    }
}

void TCPClient::handleEpollEvents()
{
    constexpr int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];

    int num = epoll_manager_->waitForEvents(events, MAX_EVENTS, -1);
    if(num == -1)
    {
        if(errno == EINTR)
        {
            return;
        }
        std::perror("epoll_wait");
        running_ = 0;
        return;
    }

    for(int i = 0; i < num; ++i)
    {
        handleConnectionEvent(events[i].data.fd, events[i].events);
    }
}

void TCPClient::handleConnectionEvent(int fd, uint32_t events)
{
    auto it = connections_.find(fd);
    if(it == connections_.end())
    {
        return;
    }

    Connection& conn = it->second;

    if(events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    {
        restartConnection(fd);
        return;
    }

    if(conn.connecting)
    {
        int err = 0;
        socklen_t len = sizeof(err);
        if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err != 0)
        {
            restartConnection(fd);
            return;
        }
        conn.connecting = false;
    }

    if(events & EPOLLOUT)
    {
        static const char data[1024] = {};
        while(conn.bytes_sent < conn.total_bytes)
        {
            size_t to_send = std::min(sizeof(data), conn.total_bytes - conn.bytes_sent);
            ssize_t n = SocketManager::sendData(fd, data, to_send);
            if(n > 0)
            {
                conn.bytes_sent += static_cast<size_t>(n);
            }
            else if(n == -1)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                restartConnection(fd);
                return;
            }
        }
        if(conn.bytes_sent >= conn.total_bytes)
        {
            restartConnection(fd);
        }
    }
}
