#pragma once

#include "ClientConfig.h"
#include <unordered_map>
#include <random>
#include <memory>
#include <csignal>

class EpollManager;
class SocketManager;

/**
 * @brief TCP-клиент с поддержкой множественных параллельных соединений
 * 
 * Основной класс клиента, реализующий:
 * - Поддержку заданного количества параллельных соединений
 * - Автоматическое пересоздание соединений после завершения
 * - Отправку случайных данных на сервер
 * - Корректное завершение работы по сигналу
 */
class TCPClient
{
public:
    /**
     * @brief Конструктор
     * @param config Конфигурация клиента
     */
    explicit TCPClient(ClientConfig config);

    /**
     * @brief Деструктор
     */
    ~TCPClient();

    /**
     * @brief Запрет копирования
     */
    TCPClient(const TCPClient&) = delete;
    TCPClient& operator=(const TCPClient&) = delete;

    /**
     * @brief Разрешение перемещения
     */
    TCPClient(TCPClient&&) noexcept = default;
    TCPClient& operator=(TCPClient&&) noexcept = default;

    /**
     * @brief Инициализация клиента
     * @return true при успешной инициализации
     */
    bool initialize();

    /**
     * @brief Запуск основного цикла клиента
     */
    void run();

    /**
     * @brief Корректное завершение работы клиента
     */
    void shutdown();

    /**
     * @brief Проверка состояния клиента
     * @return true если клиент работает
     */
    [[nodiscard]] bool isRunning() const noexcept { return running_; }

    /**
     * @brief Получение количества активных соединений
     * @return Количество активных соединений
     */
    [[nodiscard]] size_t getActiveConnections() const noexcept { return connections_.size(); }

    /**
     * @brief Получение статистики клиента
     * @return Строка со статистикой
     */
    [[nodiscard]] std::string getStats() const;

private:
    /**
     * @brief Структура для хранения состояния соединения
     */
    struct Connection
    {
        int fd = -1; ///< Файловый дескриптор соединения
        size_t total_bytes = 0; ///< Общее количество байт для отправки
        size_t bytes_sent = 0; ///< Количество уже отправленных байт
        bool is_connecting = true; ///< Флаг процесса подключения
        int reconnect_attempts = 0; ///< Количество попыток переподключения
    };

    /**
     * @brief Обработчик сигнала SIGINT
     * @param signum Номер сигнала
     */
    static void signalHandler(int signum);

    /**
     * @brief Проверка доступности сервера
     * @return true если сервер доступен
     */
    bool checkServerAvailability() const;

    /**
     * @brief Создание нового соединения
     * @param conn Ссылка на структуру соединения для заполнения
     * @return true при успешном создании
     */
    bool startConnection(Connection& conn);

    /**
     * @brief Пересоздание соединения
     * @param fd Файловый дескриптор закрытого соединения
     */
    void restartConnection(int fd);

    /**
     * @brief Обработка событий epoll
     */
    void handleEpollEvents();

    /**
     * @brief Обработка события конкретного соединения
     * @param fd Файловый дескриптор соединения
     * @param events Маска событий
     */
    void handleConnectionEvent(int fd, uint32_t events);

    /**
     * @brief Проверка и вывод количества активных соединений
     */
    void checkConnectionCount();

    /**
     * @brief Обработка недоступности сервера
     * @param context Контекст ошибки для логирования
     */
    void handleServerUnavailable(const std::string& context);

    ClientConfig config_; ///< Конфигурация клиента
    std::unique_ptr<EpollManager> epoll_manager_; ///< Менеджер epoll
    std::unordered_map<int, Connection> connections_; ///< Активные соединения
    std::mt19937 rng_; ///< Генератор случайных чисел
    size_t last_connection_count_ = 0; ///< Последнее выведенное количество соединений
    volatile sig_atomic_t running_ = 1; ///< Флаг работы клиента
    static constexpr int MAX_RECONNECT_ATTEMPTS = 3; ///< Максимальное количество попыток переподключения
    static constexpr int MAX_TOTAL_FAILURES = 10; ///< Максимальное общее количество неудач
    int total_failures_ = 0; ///< Общее количество неудачных попыток подключения
};
