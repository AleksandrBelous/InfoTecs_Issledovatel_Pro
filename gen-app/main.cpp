// main.cpp — универсальное клиент/серверное приложение

#include "TCPServer.h"
#include "ServerConfig.h"
#include "TCPClient.h"
#include "ClientConfig.h"
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
 * @param mode Режим server | client
 * @param server_config Конфигурация для сервера
 * @param client_config Конфигурация для клиента
 * @return true при успешном разборе
 */
bool parseCommandLine(int argc, char* argv[], std::string& mode, ServerConfig& server_config,
                      ClientConfig& client_config)
{
    for(int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if(arg == "--help" || arg == "-h")
        {
            std::cout << "Использование: " << argv[0] <<
                " --addr host:port --mode server|client [--connections N --seed S]\n";
            std::cout << "\nОпции:\n";
            std::cout << "  --addr host:port    Адрес и порт сервера (обязательно)\n";
            std::cout << "  --mode server|client Режим работы (обязательно)\n";
            std::cout <<
                "  --connections N     Количество параллельных соединений (только для клиента, по умолчанию 1)\n";
            std::cout <<
                "  --seed S            Зерно для генератора случайных чисел (только для клиента, по умолчанию 1)\n";
            std::cout << "  --help, -h          Показать эту справку\n";
            std::cout << "\nПримеры:\n";
            std::cout << "  " << argv[0] << " --addr localhost:8000 --mode server\n";
            std::cout << "  " << argv[0] << " --addr localhost:8000 --mode client --connections 512 --seed 1337\n";
            return false; // Завершаем программу после вывода справки
        }
        else if(arg == "--addr" && i + 1 < argc)
        {
            if(!parseAddress(argv[++i], server_config))
            {
                return false;
            }
            client_config.host = server_config.getHost();
            client_config.port = server_config.getPort();
        }
        else if(arg == "--mode" && i + 1 < argc)
        {
            mode = argv[++i];
            if(mode != "server" && mode != "client")
            {
                std::cerr << "[error] Поддерживаются режимы server или client\n";
                return false;
            }
        }
        else if(arg == "--connections" && i + 1 < argc)
        {
            client_config.connections = std::stoul(argv[++i]);
            if(client_config.connections == 0)
            {
                client_config.connections = 1;
            }
        }
        else if(arg == "--seed" && i + 1 < argc)
        {
            client_config.seed = static_cast<uint32_t>(std::stoul(argv[++i]));
        }
        else
        {
            std::cerr << "[error] Неизвестный аргумент: " << arg << "\n";
            std::cerr << "Используйте --help для получения справки\n";
            return false;
        }
    }

    if(!server_config.isValid())
    {
        std::cerr << "[error] Необходимо указать адрес и порт: --addr host:port\n";
        return false;
    }

    if(mode == "client" && !client_config.isValid())
    {
        std::cerr << "[error] Неверные параметры клиента\n";
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
 * @brief Запуск клиента
 * @param config Конфигурация клиента
 * @return Код возврата
 */
int runClient(const ClientConfig& config)
{
    try
    {
        // Создаем и инициализируем клиент
        TCPClient client(config);

        if(!client.initialize())
        {
            std::cerr << "[error] Не удалось инициализировать клиент\n";
            std::cerr << "[error] Проверьте доступность сервера: "
                << config.host << ":" << config.port << "\n";
            return EXIT_FAILURE;
        }

        // Запускаем основной цикл клиента
        client.run();

        return EXIT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        std::cerr << "[error] Исключение в клиенте: " << e.what() << "\n";
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
    ServerConfig sconfig;
    ClientConfig cconfig;
    std::string mode;

    // Разбираем аргументы командной строки
    if(!parseCommandLine(argc, argv, mode, sconfig, cconfig))
    {
        return EXIT_FAILURE; // Завершаем программу после вывода справки
    }

    // Запускаем сервер или клиент
    if(mode == "server")
    {
        return runServer(sconfig);
    }
    return runClient(cconfig);
}
