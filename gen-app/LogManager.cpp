#include "LogManager.h"
#include <chrono>
#include <iomanip>
#include <sstream>

std::shared_ptr<Logger> LogManager::logger_ = nullptr;
bool LogManager::logging_enabled_ = false;

void LogManager::initialize(bool enable_logging, const std::string& component_name)
{
    logging_enabled_ = enable_logging;

    if(enable_logging)
    {
        // Создаем имя файла с временной меткой
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y_%m_%d_%H_%M_%S");
        ss << '_' << std::setfill('0') << std::setw(3) << ms.count();

        std::string timestamp = ss.str();
        std::string log_file_path = "logs/log_" + component_name + "_" + timestamp + ".txt";

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
