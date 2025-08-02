#ifndef PACKET_PROCESSOR_H
#define PACKET_PROCESSOR_H

#include <string>
#include <thread>
#include <atomic>
#include <pcap.h>
#include "PacketParser.h"

// Forward declarations
class FlowTracker;
class StatisticsManager;

/**
 * @brief Класс для обработки сетевых пакетов с использованием libpcap
 */
class PacketProcessor
{
public:
    /**
     * @brief Конструктор
     * @param interface Интерфейс для прослушивания
     * @param flow_tracker Ссылка на трекер потоков
     * @param stats_manager Ссылка на менеджер статистики
     */
    PacketProcessor(const std::string& interface, FlowTracker& flow_tracker, StatisticsManager& stats_manager);

    /**
     * @brief Деструктор
     */
    ~PacketProcessor();

    /**
     * @brief Запуск обработки пакетов
     */
    void start();

    /**
     * @brief Остановка обработки пакетов
     */
    void stop();

    /**
     * @brief Проверка активности
     * @return true если процессор активен
     */
    bool isRunning() const { return m_running.load(); }

private:
    /**
     * @brief Инициализация libpcap
     * @return true при успешной инициализации
     */
    bool initializePcap();

    /**
     * @brief Обработка одного пакета
     * @param header Заголовок пакета
     * @param packet Данные пакета
     */
    void processPacket(const struct pcap_pkthdr* header, const u_char* packet);

    /**
     * @brief Основной цикл обработки пакетов
     */
    void packetLoop();

    std::string m_interface;
    FlowTracker& m_flow_tracker;
    StatisticsManager& m_stats_manager;

    pcap_t* m_pcap_handle;
    std::thread m_packet_thread;
    std::atomic<bool> m_running;

    PacketParser m_packet_parser;
};

#endif // PACKET_PROCESSOR_H
