#pragma once

#include "LogManager.h"

// Макросы для автоматического логирования входа и выхода из функций
#define LOG_FUNCTION_START() \
    if(LogManager::is_logging_enabled()) { \
        LogManager::get_logger()->log_start(__FUNCTION__); \
    }

#define LOG_FUNCTION_STOP() \
    if(LogManager::is_logging_enabled()) { \
        LogManager::get_logger()->log_stop(__FUNCTION__); \
    }

#define LOG_MESSAGE(msg) \
    if(LogManager::is_logging_enabled()) { \
        LogManager::get_logger()->log_message(msg); \
    }

// Класс для автоматического логирования входа и выхода из функции
class FunctionLogger
{
public:
    explicit FunctionLogger(const std::string& function_name)
        : function_name_(function_name)
    {
        if(LogManager::is_logging_enabled())
        {
            LogManager::get_logger()->log_start(function_name_);
        }
    }

    ~FunctionLogger()
    {
        if(LogManager::is_logging_enabled())
        {
            LogManager::get_logger()->log_stop(function_name_);
        }
    }

private:
    std::string function_name_;
};

// Макрос для автоматического создания FunctionLogger
#define LOG_FUNCTION() FunctionLogger function_logger(__FUNCTION__)
