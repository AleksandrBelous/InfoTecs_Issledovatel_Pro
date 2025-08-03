// main.cpp — приложение для анализа сетевого трафика

#include "packet_processor/PacketProcessor.h"
#include "flow_tracker/FlowTracker.h"
#include "statistics/StatisticsManager.h"
#include "logging/LogManager.h"
#include <iostream>
#include <string>
#include <csignal>
#include <chrono>
#include <thread>
#include <memory>

// Глобальные переменные для корректного завершения
static bool g_running = true;
static std::unique_ptr<PacketProcessor> g_packet_processor = nullptr;

/**
 * @brief Обработчик сигнала для корректного завершения
 */
void signalHandler(int signal)
{
    if(signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\n[info] Получен сигнал завершения. Завершение работы...\n";
        g_running = false;
        if(g_packet_processor)
        {
            g_packet_processor->stop();
        }
    }
}

/**
 * @brief Разбор аргументов командной строки
 * @param argc Количество аргументов
 * @param argv Массив аргументов
 * @param interface Интерфейс для прослушивания
 * @param enable_logging Флаг включения логирования
 * @return true при успешном разборе
 */
bool parseCommandLine(int argc, char* argv[], std::string& interface, bool& enable_logging)
{
    enable_logging = false;
    interface = "";

    for(int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if(arg == "--help" || arg == "-h")
        {
            std::cout << "Использование: " << argv[0] << " --interface <interface> [--log]\n";
            std::cout << "\nОпции:\n";
            std::cout << "  --interface <interface>  Интерфейс для прослушивания (обязательно)\n";
            std::cout <<
                "  --log                    Включить логирование в файлы logs/log_sniffer_YYYYMMDD_HHMMSS_mmm.txt\n";
            std::cout << "  --help, -h               Показать эту справку\n";
            std::cout << "\nПримеры:\n";
            std::cout << "  " << argv[0] << " --interface lo\n";
            std::cout << "  " << argv[0] << " --interface eth0 --log\n";
            return false; // Завершаем программу после вывода справки
        }
        else if(arg == "--interface" && i + 1 < argc)
        {
            interface = argv[++i];
        }
        else if(arg == "--log")
        {
            enable_logging = true;
        }
        else
        {
            std::cerr << "[error] Неизвестный аргумент: " << arg << "\n";
            std::cerr << "Используйте --help для получения справки\n";
            return false;
        }
    }

    if(interface.empty())
    {
        std::cerr << "[error] Не указан интерфейс. Используйте --interface <interface>\n";
        std::cerr << "Используйте --help для получения справки\n";
        return false;
    }

    return true;
}

/**
 * @brief Запуск sniffer приложения
 * @param interface Интерфейс для прослушивания
 * @param enable_logging Флаг включения логирования
 * @return Код возврата
 */
int runSniffer(const std::string& interface, bool enable_logging)
{
    try
    {
        // Инициализация логирования
        LogManager::initialize(enable_logging, "sniffer");

        std::cout << "[info] Запуск sniffer на интерфейсе: " << interface << "\n";
        std::cout << "[info] Для завершения работы используйте Ctrl-C\n\n";

        // Создание компонентов
        FlowTracker flow_tracker;
        StatisticsManager stats_manager;
        stats_manager.setFlowTracker(flow_tracker);
        
        // Создаем PacketProcessor и сохраняем в глобальную переменную
        g_packet_processor = std::make_unique<PacketProcessor>(interface, flow_tracker, stats_manager);

        // Запуск обработки пакетов в отдельном потоке
        std::thread packet_thread([&]()
        {
            g_packet_processor->start();
        });

        // Основной цикл вывода статистики
        while(g_running)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            stats_manager.cleanupOldFlows();
            stats_manager.printTopFlows(10);
        }

        // Остановка и ожидание завершения
        g_packet_processor->stop();
        if(packet_thread.joinable())
        {
            packet_thread.join();
        }

        std::cout << "\n[info] Sniffer завершен\n";
        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << "[error] Ошибка при запуске sniffer: " << e.what() << "\n";
        return 1;
    }
}

int main(int argc, char* argv[])
{
    // Установка обработчика сигналов
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::string interface;
    bool enable_logging;

    if(!parseCommandLine(argc, argv, interface, enable_logging))
    {
        return 1;
    }

    return runSniffer(interface, enable_logging);
}
