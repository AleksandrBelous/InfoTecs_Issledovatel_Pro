#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Конфигурация сервера
 * 
 * Содержит параметры для настройки TCP-сервера:
 * - адрес и порт для прослушивания
 * - дополнительные настройки
 */
class ServerConfig
{
public:
    /**
     * @brief Конструктор по умолчанию
     * 
     * Инициализирует сервер для прослушивания на всех интерфейсах
     */
    ServerConfig() = default;

    /**
     * @brief Конструктор с параметрами
     * @param host IP-адрес для прослушивания
     * @param port Порт для прослушивания
     */
    ServerConfig(std::string host, uint16_t port);

    /**
     * @brief Деструктор
     */
    ~ServerConfig() = default;

    /**
     * @brief Конструктор копирования
     */
    ServerConfig(const ServerConfig&) = default;

    /**
     * @brief Оператор присваивания
     */
    ServerConfig& operator=(const ServerConfig&) = default;

    /**
     * @brief Конструктор перемещения
     */
    ServerConfig(ServerConfig&&) = default;

    /**
     * @brief Оператор перемещающего присваивания
     */
    ServerConfig& operator=(ServerConfig&&) = default;

    // Геттеры
    [[nodiscard]] const std::string& getHost() const noexcept { return host_; }
    [[nodiscard]] uint16_t getPort() const noexcept { return port_; }

    // Сеттеры
    void setHost(const std::string& host) { host_ = host; }
    void setPort(uint16_t port) { port_ = port; }

    /**
     * @brief Проверка валидности конфигурации
     * @return true если конфигурация корректна
     */
    [[nodiscard]] bool isValid() const noexcept;

private:
    std::string host_ = "0.0.0.0"; ///< IP-адрес для прослушивания
    uint16_t port_ = 0; ///< Порт для прослушивания
};
