#pragma once

#include <string>
#include <cstdint>
#include <sys/socket.h>

/**
 * @brief Менеджер сокетов
 * 
 * Отвечает за:
 * - Создание и настройку сокетов
 * - Управление неблокирующим режимом
 * - Привязку сокетов к адресам
 */
class SocketManager
{
public:
    /**
     * @brief Конструктор
     */
    SocketManager() = default;

    /**
     * @brief Деструктор
     */
    ~SocketManager() = default;

    /**
     * @brief Запрет копирования
     */
    SocketManager(const SocketManager&) = delete;
    SocketManager& operator=(const SocketManager&) = delete;

    /**
     * @brief Разрешение перемещения
     */
    SocketManager(SocketManager&&) noexcept = default;
    SocketManager& operator=(SocketManager&&) noexcept = default;

    /**
     * @brief Создание серверного сокета
     * @param host IP-адрес для привязки
     * @param port Порт для привязки
     * @return Файловый дескриптор сокета или -1 при ошибке
     */
    [[nodiscard]] static int createServerSocket(const std::string& host, uint16_t port);

    /**
     * @brief Перевод сокета в неблокирующий режим
     * @param fd Файловый дескриптор
     * @return true при успехе
     */
    [[nodiscard]] static bool setNonBlocking(int fd);

    /**
     * @brief Закрытие сокета
     * @param fd Файловый дескриптор
     */
    static void closeSocket(int fd);

    /**
     * @brief Принятие нового подключения
     * @param server_fd Файловый дескриптор серверного сокета
     * @param client_addr Указатель на структуру адреса клиента
     * @param addr_len Указатель на длину адреса
     * @return Файловый дескриптор клиентского сокета или -1 при ошибке
     */
    [[nodiscard]] static int acceptConnection(int server_fd, struct sockaddr_in* client_addr, socklen_t* addr_len);

    /**
     * @brief Чтение данных из сокета
     * @param fd Файловый дескриптор
     * @param buffer Буфер для данных
     * @param buffer_size Размер буфера
     * @return Количество прочитанных байт или -1 при ошибке
     */
    [[nodiscard]] static ssize_t receiveData(int fd, char* buffer, size_t buffer_size);

    /**
     * @brief Получение строкового представления IP-адреса
     * @param addr Структура адреса
     * @return Строка с IP-адресом
     */
    [[nodiscard]] static std::string getClientIP(const struct sockaddr_in& addr);

    /**
     * @brief Получение порта клиента
     * @param addr Структура адреса
     * @return Порт клиента
     */
    [[nodiscard]] static uint16_t getClientPort(const struct sockaddr_in& addr);

private:
    /**
     * @brief Установка опции SO_REUSEADDR
     * @param fd Файловый дескриптор
     * @return true при успехе
     */
    [[nodiscard]] static bool setReuseAddr(int fd);
};
