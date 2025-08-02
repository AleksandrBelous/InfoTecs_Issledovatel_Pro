#pragma once

#include "Logger.h"
#include <memory>

/**
 * @brief Менеджер логирования для приложения
 */
class LogManager
{
public:
    /**
     * @brief Инициализация системы логирования
     * @param enable_logging Включить ли логирование
     * @param component_name Имя компонента (server/client)
     */
    static void initialize(bool enable_logging, const std::string& component_name);

    /**
     * @brief Получение логгера
     * @return Указатель на логгер
     */
    static std::shared_ptr<Logger> get_logger();

    /**
     * @brief Проверка, включено ли логирование
     * @return true если логирование включено
     */
    static bool is_logging_enabled();

private:
    static std::shared_ptr<Logger> logger_;
    static bool logging_enabled_;
};
