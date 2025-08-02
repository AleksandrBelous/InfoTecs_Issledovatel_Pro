#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

Logger::Logger(const std::string& log_file_path, const std::string& component_name)
    : component_name_(component_name)
      , enabled_(false)
      , indent_level_(0)
{
    // Создаем директорию logs если её нет
    std::filesystem::path log_dir = std::filesystem::path(log_file_path).parent_path();
    if(!log_dir.empty())
    {
        std::filesystem::create_directories(log_dir);
    }

    // Открываем файл для записи (перезаписываем старый файл)
    log_file_.open(log_file_path, std::ios::trunc);
    if(log_file_.is_open())
    {
        enabled_ = true;
        log_message("Logger initialized for " + component_name_);
    }
    else
    {
        std::cerr << "[error] Failed to open log file: " << log_file_path << std::endl;
    }
}

Logger::~Logger()
{
    if(enabled_)
    {
        log_message("Logger destroyed");
        log_file_.close();
    }
}

void Logger::log_start(const std::string& function_name)
{
    if(!enabled_) return;

    std::string indent = get_indent();
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    log_file_ << "[" << ss.str() << "] " << indent << "start " << function_name << std::endl;
    log_file_.flush();
    indent_level_++;
}

void Logger::log_stop(const std::string& function_name)
{
    if(!enabled_) return;

    indent_level_--;
    if(indent_level_ < 0) indent_level_ = 0;

    std::string indent = get_indent();
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    log_file_ << "[" << ss.str() << "] " << indent << "stop  " << function_name << std::endl;
    log_file_.flush();
}

void Logger::log_message(const std::string& message)
{
    if(!enabled_) return;

    std::string indent = get_indent();
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    log_file_ << "[" << ss.str() << "] " << indent << "msg: " << message << std::endl;
    log_file_.flush();
}

std::string Logger::get_indent() const
{
    return std::string(indent_level_ * 4, ' ');
}
