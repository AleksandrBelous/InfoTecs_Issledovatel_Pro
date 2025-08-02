#pragma once

#include "ServerConfig.h"
#include <memory>
#include <unordered_set>
#include <csignal>

// Предварительные объявления
class EpollManager;
class SocketManager;

/**
 * @brief TCP-сервер с использованием epoll
 * 
 * Основной класс сервера, реализующий:
 * - Ожидание подключений
 * - Чтение данных от клиентов
 * - Обработку отключений
 * - Корректное завершение работы
 */
class TCPServer
{
public:
    /**
     * @brief Конструктор
     * @param config Конфигурация сервера
     */
    explicit TCPServer(ServerConfig config);

    /**
     * @brief Деструктор
     */
    ~TCPServer();

    /**
     * @brief Запрет копирования
     */
    TCPServer(const TCPServer&) = delete;
    TCPServer& operator=(const TCPServer&) = delete;

    /**
     * @brief Разрешение перемещения
     */
    TCPServer(TCPServer&&) noexcept = default;
    TCPServer& operator=(TCPServer&&) noexcept = default;

    /**
     * @brief Инициализация сервера
     * @return true при успешной инициализации
     */
    bool initialize();

    /**
     * @brief Запуск основного цикла сервера
     */
    void run();

    /**
     * @brief Корректное завершение работы сервера
     */
    void shutdown();

    /**
     * @brief Проверка состояния сервера
     * @return true если сервер работает
     */
    [[nodiscard]] bool isRunning() const noexcept { return running_; }

    /**
     * @brief Получение количества активных соединений
     * @return Количество клиентских соединений
     */
    [[nodiscard]] size_t getActiveConnections() const noexcept;

private:
    /**
     * @brief Обработчик сигнала SIGINT
     * @param signum Номер сигнала
     */
    static void signalHandler(int signum);

    /**
     * @brief Обработка новых подключений
     */
    void handleNewConnections();

    /**
     * @brief Обработка данных от клиента
     * @param client_fd Файловый дескриптор клиента
     */
    void handleClientData(int client_fd);

    /**
     * @brief Обработка событий epoll
     */
    void handleEpollEvents();

    /**
     * @brief Закрытие клиентского соединения
     * @param client_fd Файловый дескриптор клиента
     */
    void closeClientConnection(int client_fd);

    ServerConfig config_; ///< Конфигурация сервера
    std::unique_ptr<EpollManager> epoll_manager_; ///< Менеджер epoll
    std::unordered_set<int> client_fds_; ///< Активные клиентские соединения
    volatile sig_atomic_t running_ = 1; ///< Флаг работы сервера
    int server_fd_ = -1; ///< Файловый дескриптор серверного сокета
};
