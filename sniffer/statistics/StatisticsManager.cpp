#include "StatisticsManager.h"
#include "../flow_tracker/FlowStats.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>

StatisticsManager::StatisticsManager()
    : m_flow_tracker(nullptr)
      , m_last_cleanup_time(0)
{
}

void StatisticsManager::updateFlowStats(const FlowTuple& flow_tuple, uint32_t packet_size,
                                        uint32_t payload_size, uint64_t timestamp) const
{
    if(m_flow_tracker)
    {
        m_flow_tracker->updateFlow(flow_tuple, packet_size, payload_size, timestamp);
    }
}

void StatisticsManager::printTopFlows(size_t count) const
{
    auto top_flows = getTopFlows(count);

    if(top_flows.empty())
    {
        std::cout << "\n[info] Активных TCP потоков не обнаружено\n";
        return;
    }

    // Очистка экрана (ANSI escape sequence)
    std::cout << "\033[2J\033[H";

    // Заголовок
    std::cout << "=== ТОП-" << count << " TCP потоков по скорости передачи данных ===\n";
    std::cout << std::string(80, '=') << "\n";

    // Заголовки с правильными ширинами полей
    std::cout << std::left
        << std::setw(16) << "Source"
        << std::setw(8) << "Port"
        << std::setw(16) << "Destination"
        << std::setw(8) << "Port"
        << std::setw(12) << "Speed"
        << std::setw(10) << "AvgSize"
        << std::setw(10) << "Bytes"
        << std::setw(8) << "Packets" << "\n";

    std::cout << std::string(80, '-') << "\n";

    // Вывод потоков с теми же ширинами полей
    for(const auto& flow : top_flows)
    {
        std::cout << std::left
            << std::setw(16) << flow.src_ip_str
            << std::setw(8) << flow.src_port
            << std::setw(16) << flow.dst_ip_str
            << std::setw(8) << flow.dst_port
            << std::setw(12) << formatSpeed(flow.average_speed)
            << std::setw(10) << std::fixed << std::setprecision(1) << flow.average_packet_size
            << std::setw(10) << flow.total_bytes
            << std::setw(8) << flow.packet_count << "\n";
    }

    std::cout << std::string(80, '=') << "\n";
    std::cout << "Всего активных потоков: " << (m_flow_tracker ? m_flow_tracker->getActiveFlowCount() : 0) << "\n";
    std::cout << "Для завершения работы используйте Ctrl-C\n\n";
}

void StatisticsManager::setFlowTracker(FlowTracker& flow_tracker)
{
    m_flow_tracker = &flow_tracker;
}

void StatisticsManager::cleanupOldFlows()
{
    uint64_t current_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if(current_time - m_last_cleanup_time > CLEANUP_INTERVAL)
    {
        if(m_flow_tracker)
        {
            m_flow_tracker->cleanupOldFlows(60); // Удаляем потоки неактивные более 60 секунд
        }
        m_last_cleanup_time = current_time;
    }
}

std::vector<TopFlowInfo> StatisticsManager::getTopFlows(size_t count) const
{
    std::vector<TopFlowInfo> top_flows;

    if(!m_flow_tracker)
    {
        return top_flows;
    }

    auto all_flows = m_flow_tracker->getAllFlows();
    uint64_t current_time = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Преобразуем в TopFlowInfo
    for(const auto& flow_pair : all_flows)
    {
        const FlowTuple& flow_tuple = flow_pair.first;
        const FlowStats& flow_stats = flow_pair.second;

        TopFlowInfo flow_info;
        flow_info.flow_tuple = flow_tuple;
        flow_info.src_ip_str = PacketParser::ipToString(flow_tuple.src_ip);
        flow_info.dst_ip_str = PacketParser::ipToString(flow_tuple.dst_ip);
        flow_info.src_port = flow_tuple.src_port;
        flow_info.dst_port = flow_tuple.dst_port;
        flow_info.average_speed = flow_stats.getAverageSpeed(current_time);
        flow_info.average_packet_size = flow_stats.getAveragePacketSize();
        flow_info.total_bytes = flow_stats.getTotalBytes();
        flow_info.packet_count = flow_stats.getPacketCount();

        top_flows.push_back(flow_info);
    }

    // Сортируем по скорости (убывание)
    std::sort(top_flows.begin(), top_flows.end(),
              [](const TopFlowInfo& a, const TopFlowInfo& b)
              {
                  return a.average_speed > b.average_speed;
              });

    // Возвращаем топ-N
    if(top_flows.size() > count)
    {
        top_flows.resize(count);
    }

    return top_flows;
}

std::string StatisticsManager::formatSpeed(double speed)
{
    std::ostringstream oss;

    if(speed >= 1024 * 1024 * 1024) // >= 1 GB/s
    {
        oss << std::fixed << std::setprecision(1) << (speed / (1024 * 1024 * 1024)) << " GB/s";
    }
    else if(speed >= 1024 * 1024) // >= 1 MB/s
    {
        oss << std::fixed << std::setprecision(1) << (speed / (1024 * 1024)) << " MB/s";
    }
    else if(speed >= 1024) // >= 1 KB/s
    {
        oss << std::fixed << std::setprecision(1) << (speed / 1024) << " KB/s";
    }
    else
    {
        oss << std::fixed << std::setprecision(0) << speed << " B/s";
    }

    return oss.str();
}
