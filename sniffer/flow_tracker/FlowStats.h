#ifndef FLOW_STATS_H
#define FLOW_STATS_H

#include "../packet_processor/PacketParser.h"
#include <cstdint>
#include <string>

/**
 * @brief Класс для хранения статистики потока
 */
class FlowStats
{
public:
    /**
     * @brief Конструктор
     */
    FlowStats();

    /**
     * @brief Обновление статистики новым пакетом
     * @param packet_size Размер пакета на уровне Ethernet
     * @param payload_size Размер полезной нагрузки TCP
     * @param timestamp Временная метка пакета
     */
    void updateStats(uint32_t packet_size, uint32_t payload_size, uint64_t timestamp);

    /**
     * @brief Получение среднего размера пакета
     * @return Средний размер пакета в байтах
     */
    double getAveragePacketSize() const;

    /**
     * @brief Получение общего количества переданных байт
     * @return Общее количество байт в полезной нагрузке
     */
    uint64_t getTotalBytes() const { return total_bytes; }

    /**
     * @brief Получение количества пакетов
     * @return Количество пакетов в потоке
     */
    uint64_t getPacketCount() const { return m_packet_count; }

    /**
     * @brief Получение средней скорости передачи данных
     * @param current_time Текущее время в микросекундах
     * @return Скорость в байтах в секунду
     */
    double getAverageSpeed(uint64_t current_time) const;

    /**
     * @brief Получение времени последнего пакета
     * @return Временная метка последнего пакета
     */
    uint64_t getLastPacketTime() const { return m_last_packet_time; }

    /**
     * @brief Сброс статистики
     */
    void reset();

private:
    uint64_t total_bytes; // Общее количество байт в полезной нагрузке
    uint64_t m_packet_count; // Количество пакетов
    uint64_t m_total_packet_size; // Общий размер пакетов на уровне Ethernet
    uint64_t m_first_packet_time; // Время первого пакета
    uint64_t m_last_packet_time; // Время последнего пакета
};

#endif // FLOW_STATS_H
