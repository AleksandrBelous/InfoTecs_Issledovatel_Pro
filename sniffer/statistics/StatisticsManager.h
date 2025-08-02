#ifndef STATISTICS_MANAGER_H
#define STATISTICS_MANAGER_H

#include "../packet_processor/PacketParser.h"
#include "../flow_tracker/FlowTracker.h"
#include <vector>
#include <string>
#include <chrono>

/**
 * @brief Структура для хранения информации о топ-потоке
 */
struct TopFlowInfo
{
    FlowTuple flow_tuple;
    std::string src_ip_str;
    std::string dst_ip_str;
    uint16_t src_port;
    uint16_t dst_port;
    double average_speed; // Байт в секунду
    double average_packet_size;
    uint64_t total_bytes;
    uint64_t packet_count;
};

/**
 * @brief Класс для управления статистикой потоков
 */
class StatisticsManager
{
public:
    /**
     * @brief Конструктор
     */
    StatisticsManager();

    /**
     * @brief Деструктор
     */
    ~StatisticsManager() = default;

    /**
     * @brief Обновление статистики потока
     * @param flow_tuple 4-tuple потока
     * @param packet_size Размер пакета на уровне Ethernet
     * @param payload_size Размер полезной нагрузки TCP
     * @param timestamp Временная метка пакета
     */
    void updateFlowStats(const FlowTuple& flow_tuple, uint32_t packet_size,
                         uint32_t payload_size, uint64_t timestamp);

    /**
     * @brief Вывод топ-N потоков по скорости передачи данных
     * @param count Количество потоков для вывода
     */
    void printTopFlows(size_t count);

    /**
     * @brief Установка трекера потоков
     * @param flow_tracker Ссылка на трекер потоков
     */
    void setFlowTracker(FlowTracker& flow_tracker);

    /**
     * @brief Очистка устаревших потоков
     */
    void cleanupOldFlows();

private:
    /**
     * @brief Получение топ-N потоков по скорости
     * @param count Количество потоков
     * @return Вектор топ-потоков
     */
    std::vector<TopFlowInfo> getTopFlows(size_t count);

    /**
     * @brief Форматирование скорости для вывода
     * @param speed Скорость в байтах в секунду
     * @return Отформатированная строка
     */
    std::string formatSpeed(double speed);

    FlowTracker* m_flow_tracker;
    uint64_t m_last_cleanup_time;
    static constexpr uint64_t CLEANUP_INTERVAL = 30; // секунды
};

#endif // STATISTICS_MANAGER_H
