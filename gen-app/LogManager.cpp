#include "LogManager.h"

std::shared_ptr<Logger> LogManager::logger_ = nullptr;
bool LogManager::logging_enabled_ = false;

void LogManager::initialize(bool enable_logging, const std::string& component_name)
{
    logging_enabled_ = enable_logging;

    if(enable_logging)
    {
        std::string log_file_path = "logs/log_" + component_name + ".txt";
        logger_ = std::make_shared<Logger>(log_file_path, component_name);
    }
}

std::shared_ptr<Logger> LogManager::get_logger()
{
    return logger_;
}

bool LogManager::is_logging_enabled()
{
    return logging_enabled_ && logger_ && logger_->is_enabled();
}
