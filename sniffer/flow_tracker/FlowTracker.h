#ifndef FLOW_TRACKER_H
#define FLOW_TRACKER_H

#include "../packet_processor/PacketParser.h"
#include "FlowStats.h"
#include <map>
#include <mutex>

/**
 * @brief Класс для отслеживания TCP потоков
 */
class FlowTracker
{
public:
    /**
     * @brief Конструктор
     */
    FlowTracker();

    /**
     * @brief Деструктор
     */
    ~FlowTracker() = default;

    /**
     * @brief Обновление статистики потока
     * @param flow_tuple 4-tuple потока
     * @param packet_size Размер пакета на уровне Ethernet
     * @param payload_size Размер полезной нагрузки TCP
     * @param timestamp Временная метка пакета
     */
    void updateFlow(const FlowTuple& flow_tuple, uint32_t packet_size,
                    uint32_t payload_size, uint64_t timestamp);

    /**
     * @brief Получение статистики потока
     * @param flow_tuple 4-tuple потока
     * @return Указатель на статистику потока или nullptr если поток не найден
     */
    const FlowStats* getFlowStats(const FlowTuple& flow_tuple) const;

    /**
     * @brief Получение всех активных потоков
     * @return Карта всех потоков с их статистикой
     */
    std::map<FlowTuple, FlowStats> getAllFlows() const;

    /**
     * @brief Очистка устаревших потоков
     * @param timeout_seconds Таймаут в секундах для удаления неактивных потоков
     */
    void cleanupOldFlows(uint64_t timeout_seconds);

    /**
     * @brief Получение количества активных потоков
     * @return Количество активных потоков
     */
    size_t getActiveFlowCount() const;

private:
    mutable std::mutex m_flows_mutex;
    std::map<FlowTuple, FlowStats> m_flows;
};

#endif // FLOW_TRACKER_H
