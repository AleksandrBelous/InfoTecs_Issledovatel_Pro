// main.cpp — универсальное клиент/серверное приложение

#include "TCPServer.h"
#include "ServerConfig.h"
#include <iostream>
#include <cstdlib>
#include <string>

/**
 * @brief Разбор адреса в формате host:port
 * @param addr Строка адреса в формате host:port
 * @param config Конфигурация для заполнения
 * @return true при успешном разборе
 */
bool parseAddress(const std::string& addr, ServerConfig& config)
{
    size_t colon_pos = addr.find(':');
    if(colon_pos == std::string::npos)
    {
        std::cerr << "[error] Неверный формат адреса. Используйте host:port\n";
        return false;
    }

    std::string host = addr.substr(0, colon_pos);
    std::string port_str = addr.substr(colon_pos + 1);

    // Преобразуем localhost в 127.0.0.1
    if(host == "localhost")
    {
        host = "127.0.0.1";
    }

    config.setHost(host);

    try
    {
        int port = std::stoi(port_str);
        if(port <= 0 || port > 65535)
        {
            std::cerr << "[error] Некорректный порт: " << port << "\n";
            return false;
        }
        config.setPort(static_cast<uint16_t>(port));
    }
    catch(const std::exception&)
    {
        std::cerr << "[error] Некорректный порт: " << port_str << "\n";
        return false;
    }

    return true;
}

/**
 * @brief Разбор аргументов командной строки
 * @param argc Количество аргументов
 * @param argv Массив аргументов
 * @param config Конфигурация для заполнения
 * @return true при успешном разборе
 */
bool parseCommandLine(int argc, char* argv[], ServerConfig& config)
{
    for(int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if(arg == "--addr" && i + 1 < argc)
        {
            if(!parseAddress(argv[++i], config))
            {
                return false;
            }
        }
        else if(arg == "--mode" && i + 1 < argc)
        {
            std::string mode = argv[++i];
            if(mode != "server")
            {
                std::cerr << "[error] Поддерживается только режим server\n";
                return false;
            }
        }
        else
        {
            std::cerr << "[error] Неизвестный аргумент: " << arg << "\n";
            return false;
        }
    }

    if(!config.isValid())
    {
        std::cerr << "[error] Необходимо указать адрес и порт: --addr host:port\n";
        return false;
    }

    return true;
}

/**
 * @brief Запуск сервера
 * @param config Конфигурация сервера
 * @return Код возврата
 */
int runServer(const ServerConfig& config)
{
    try
    {
        // Создаем и инициализируем сервер
        TCPServer server(config);

        if(!server.initialize())
        {
            std::cerr << "[error] Не удалось инициализировать сервер\n";
            return EXIT_FAILURE;
        }

        // Запускаем основной цикл сервера
        server.run();

        return EXIT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        std::cerr << "[error] Исключение: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}

/**
 * @brief Точка входа в программу
 * @param argc Количество аргументов командной строки
 * @param argv Массив аргументов командной строки
 * @return Код возврата
 */
int main(int argc, char* argv[])
{
    ServerConfig config;

    // Разбираем аргументы командной строки
    if(!parseCommandLine(argc, argv, config))
    {
        std::cerr << "Использование: " << argv[0] << " --addr host:port --mode server\n";
        std::cerr << "Пример: " << argv[0] << " --addr localhost:8000 --mode server\n";
        return EXIT_FAILURE;
    }

    // Запускаем сервер
    return runServer(config);
}
