#include "PacketProcessor.h"
#include "../flow_tracker/FlowTracker.h"
#include "../statistics/StatisticsManager.h"
#include "../logging/LogManager.h"
#include <iostream>
#include <cstring>
#include <chrono>

PacketProcessor::PacketProcessor(const std::string& interface, FlowTracker& flow_tracker,
                                 StatisticsManager& stats_manager)
    : m_interface(interface)
      , m_flow_tracker(flow_tracker)
      , m_stats_manager(stats_manager)
      , m_pcap_handle(nullptr)
      , m_running(false)
{
}

PacketProcessor::~PacketProcessor()
{
    stop();
    if(m_pcap_handle)
    {
        pcap_close(m_pcap_handle);
    }
}

void PacketProcessor::start()
{
    if(m_running.load())
    {
        return;
    }

    if(!initializePcap())
    {
        throw std::runtime_error("Не удалось инициализировать libpcap");
    }

    m_running = true;
    m_packet_thread = std::thread(&PacketProcessor::packetLoop, this);
}

void PacketProcessor::stop()
{
    if(!m_running.load())
    {
        return;
    }

    m_running = false;

    if(m_pcap_handle)
    {
        pcap_breakloop(m_pcap_handle);
    }

    if(m_packet_thread.joinable())
    {
        m_packet_thread.join();
    }
}

bool PacketProcessor::initializePcap()
{
    char errbuf[PCAP_ERRBUF_SIZE];

    // Открытие интерфейса для захвата пакетов
    m_pcap_handle = pcap_open_live(m_interface.c_str(), 65535, 1, 1000, errbuf);
    if(!m_pcap_handle)
    {
        std::cerr << "[error] Не удалось открыть интерфейс " << m_interface << ": " << errbuf << "\n";
        return false;
    }

    // Установка фильтра для TCP/IP пакетов
    struct bpf_program fp;
    const char* filter_exp = "tcp and ip";

    if(pcap_compile(m_pcap_handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1)
    {
        std::cerr << "[error] Не удалось скомпилировать фильтр: " << pcap_geterr(m_pcap_handle) << "\n";
        pcap_close(m_pcap_handle);
        m_pcap_handle = nullptr;
        return false;
    }

    if(pcap_setfilter(m_pcap_handle, &fp) == -1)
    {
        std::cerr << "[error] Не удалось установить фильтр: " << pcap_geterr(m_pcap_handle) << "\n";
        pcap_close(m_pcap_handle);
        m_pcap_handle = nullptr;
        return false;
    }

    std::cout << "[info] Инициализирован захват пакетов на интерфейсе " << m_interface << "\n";
    return true;
}

void PacketProcessor::processPacket(const struct pcap_pkthdr* header, const u_char* packet)
{
    try
    {
        // Проверяем что это TCP/IP пакет
        if(!PacketParser::isTcpIpv4Packet(packet, header->len))
        {
            return;
        }

        // Парсим пакет
        uint64_t timestamp = static_cast<uint64_t>(header->ts.tv_sec) * 1000000 +
            static_cast<uint64_t>(header->ts.tv_usec);

        auto packet_info = m_packet_parser.parsePacket(packet, header->len, timestamp);
        if(!packet_info)
        {
            return;
        }

        // Обновляем статистику потока
        m_flow_tracker.updateFlow(packet_info->flow_tuple, packet_info->packet_size,
                                  packet_info->payload_size, packet_info->timestamp);

        // Обновляем общую статистику
        m_stats_manager.updateFlowStats(packet_info->flow_tuple, packet_info->packet_size,
                                        packet_info->payload_size, packet_info->timestamp);
    }
    catch(const std::exception& e)
    {
        std::cerr << "[error] Ошибка при обработке пакета: " << e.what() << "\n";
    }
}

void PacketProcessor::packetLoop()
{
    std::cout << "[info] Начало захвата пакетов...\n";
    
    int packet_count = 0;
    int processed_count = 0;

    while(m_running.load())
    {
        struct pcap_pkthdr header;
        const u_char* packet = pcap_next(m_pcap_handle, &header);

        if(packet)
        {
            packet_count++;
            // std::cout << "[debug] Получено пакетов: " << packet_count << "\n";
            
            processPacket(&header, packet);
            processed_count++;
        }
        else if(pcap_geterr(m_pcap_handle))
        {
            std::cerr << "[error] Ошибка при захвате пакета: " << pcap_geterr(m_pcap_handle) << "\n";
            break;
        }
        else
        {
            // Нет пакетов, небольшая задержка
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    std::cout << "[info] Захват пакетов остановлен. Всего получено: " << packet_count 
              << ", обработано: " << processed_count << "\n";
}
