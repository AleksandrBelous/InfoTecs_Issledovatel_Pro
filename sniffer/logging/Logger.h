#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <memory>

/**
 * @brief Класс для логирования с отслеживанием стека вызовов
 */
class Logger
{
public:
    /**
     * @brief Конструктор
     * @param log_file_path Путь к файлу лога
     * @param component_name Имя компонента (server/client)
     */
    Logger(const std::string& log_file_path, const std::string& component_name);

    /**
     * @brief Деструктор
     */
    ~Logger();

    /**
     * @brief Логирование входа в функцию
     * @param function_name Имя функции
     */
    void log_start(const std::string& function_name);

    /**
     * @brief Логирование выхода из функции
     * @param function_name Имя функции
     */
    void log_stop(const std::string& function_name);

    /**
     * @brief Логирование сообщения
     * @param message Сообщение для логирования
     */
    void log_message(const std::string& message);

    /**
     * @brief Проверка, включено ли логирование
     * @return true если логирование включено
     */
    [[nodiscard]] bool is_enabled() const { return enabled_; }

private:
    std::ofstream log_file_;
    std::string component_name_;
    bool enabled_;
    int indent_level_;

    /**
     * @brief Получение отступа для текущего уровня стека
     * @return Строка с отступом
     */
    std::string get_indent() const;
};
