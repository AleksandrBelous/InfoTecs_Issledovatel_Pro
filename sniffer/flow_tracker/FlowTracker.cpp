#include "FlowTracker.h"
#include <chrono>
#include <iostream>

FlowTracker::FlowTracker()
{
}

void FlowTracker::updateFlow(const FlowTuple& flow_tuple, uint32_t packet_size,
                             uint32_t payload_size, uint64_t timestamp)
{
    std::lock_guard<std::mutex> lock(m_flows_mutex);

    auto it = m_flows.find(flow_tuple);
    if(it == m_flows.end())
    {
        // Создаем новый поток
        FlowStats new_stats;
        new_stats.updateStats(packet_size, payload_size, timestamp);
        m_flows[flow_tuple] = new_stats;
    }
    else
    {
        // Обновляем существующий поток
        it->second.updateStats(packet_size, payload_size, timestamp);
    }
}

const FlowStats* FlowTracker::getFlowStats(const FlowTuple& flow_tuple) const
{
    std::lock_guard<std::mutex> lock(m_flows_mutex);

    auto it = m_flows.find(flow_tuple);
    if(it != m_flows.end())
    {
        return &it->second;
    }
    return nullptr;
}

std::map<FlowTuple, FlowStats> FlowTracker::getAllFlows() const
{
    std::lock_guard<std::mutex> lock(m_flows_mutex);
    return m_flows;
}

void FlowTracker::cleanupOldFlows(uint64_t timeout_seconds)
{
    uint64_t current_time = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    uint64_t timeout_us = timeout_seconds * 1000000;

    std::lock_guard<std::mutex> lock(m_flows_mutex);

    auto it = m_flows.begin();
    while(it != m_flows.end())
    {
        uint64_t last_packet_time = it->second.getLastPacketTime();
        if(current_time - last_packet_time > timeout_us)
        {
            it = m_flows.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

size_t FlowTracker::getActiveFlowCount() const
{
    std::lock_guard<std::mutex> lock(m_flows_mutex);
    return m_flows.size();
}
