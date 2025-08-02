#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Конфигурация клиента
 * 
 * Содержит параметры для настройки TCP-клиента:
 * - адрес и порт сервера
 * - количество параллельных соединений
 * - параметры генератора случайных чисел
 */
struct ClientConfig
{
    std::string host = "127.0.0.1"; ///< Адрес сервера
    uint16_t port = 0; ///< Порт сервера
    size_t connections = 1; ///< Количество параллельных соединений
    uint32_t seed = 1; ///< Зёрно для генератора случайных чисел

    /**
     * @brief Проверка валидности конфигурации
     * @return true если конфигурация корректна
     */
    [[nodiscard]] bool isValid() const noexcept
    {
        return !host.empty() && port > 0 && connections > 0;
    }

    /**
     * @brief Получение строкового представления конфигурации
     * @return Строка с параметрами конфигурации
     */
    [[nodiscard]] std::string toString() const
    {
        return "host=" + host + ", port=" + std::to_string(port) +
            ", connections=" + std::to_string(connections) +
            ", seed=" + std::to_string(seed);
    }
};
