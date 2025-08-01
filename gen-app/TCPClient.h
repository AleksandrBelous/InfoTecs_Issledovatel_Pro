#pragma once

#include "ClientConfig.h"
#include <unordered_map>
#include <random>
#include <memory>
#include <csignal>

class EpollManager;
class SocketManager;

/**
 * @brief TCP клиент с поддержкой множества параллельных соединений
 */
class TCPClient
{
public:
    explicit TCPClient(ClientConfig config);
    ~TCPClient();

    TCPClient(const TCPClient&) = delete;
    TCPClient& operator=(const TCPClient&) = delete;
    TCPClient(TCPClient&&) noexcept = default;
    TCPClient& operator=(TCPClient&&) noexcept = default;

    bool initialize();
    void run();
    void shutdown();

    [[nodiscard]] bool isRunning() const noexcept { return running_; }

private:
    struct Connection
    {
        int fd = -1;
        size_t total_bytes = 0;
        size_t bytes_sent = 0;
        bool connecting = true;
    };

    static void signalHandler(int signum);

    bool startConnection(Connection& conn);
    void restartConnection(int fd);
    void handleEpollEvents();
    void handleConnectionEvent(int fd, uint32_t events);

    ClientConfig config_;
    std::unique_ptr<EpollManager> epoll_manager_;
    std::unordered_map<int, Connection> connections_;
    std::mt19937 rng_;
    volatile sig_atomic_t running_ = 1;
};
