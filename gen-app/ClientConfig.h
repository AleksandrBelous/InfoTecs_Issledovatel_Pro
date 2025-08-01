#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Конфигурация клиента
 */
struct ClientConfig
{
    std::string host = "127.0.0.1"; ///< Адрес сервера
    uint16_t port = 0; ///< Порт сервера
    size_t connections = 1; ///< Количество параллельных соединений
    uint32_t seed = 1; ///< Зёрно ГСЧ

    [[nodiscard]] bool isValid() const noexcept
    {
        return !host.empty() && port > 0 && connections > 0;
    }
};
