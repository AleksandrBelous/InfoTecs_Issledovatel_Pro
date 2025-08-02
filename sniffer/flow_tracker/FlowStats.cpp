#include "FlowStats.h"
#include <algorithm>

FlowStats::FlowStats()
    : total_bytes(0)
      , m_packet_count(0)
      , m_total_packet_size(0)
      , m_first_packet_time(0)
      , m_last_packet_time(0)
{
}

void FlowStats::updateStats(uint32_t packet_size, uint32_t payload_size, uint64_t timestamp)
{
    total_bytes += payload_size;
    m_total_packet_size += packet_size;
    m_packet_count++;

    if(m_first_packet_time == 0)
    {
        m_first_packet_time = timestamp;
    }
    m_last_packet_time = timestamp;
}

double FlowStats::getAveragePacketSize() const
{
    if(m_packet_count == 0)
    {
        return 0.0;
    }
    return static_cast<double>(m_total_packet_size) / m_packet_count;
}

double FlowStats::getAverageSpeed(uint64_t current_time) const
{
    if(m_packet_count == 0 || m_first_packet_time == 0)
    {
        return 0.0;
    }

    uint64_t duration_us = current_time - m_first_packet_time;
    if(duration_us == 0)
    {
        return 0.0;
    }

    // Конвертируем в секунды и вычисляем скорость в байтах в секунду
    double duration_seconds = static_cast<double>(duration_us) / 1000000.0;
    return static_cast<double>(total_bytes) / duration_seconds;
}

void FlowStats::reset()
{
    total_bytes = 0;
    m_packet_count = 0;
    m_total_packet_size = 0;
    m_first_packet_time = 0;
    m_last_packet_time = 0;
}
