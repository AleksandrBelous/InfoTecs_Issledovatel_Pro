#include "PacketParser.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/tcp.h>
#include <net/ethernet.h>

std::unique_ptr<PacketInfo> PacketParser::parsePacket(const u_char* packet, uint32_t packet_size, uint64_t timestamp)
{
    try
    {
        // Проверяем минимальный размер пакета (Ethernet + IP + TCP заголовки)
        if(packet_size < sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct tcphdr))
        {
            return nullptr;
        }

        // Извлекаем 4-tuple
        FlowTuple flow_tuple = extractFlowTuple(packet, packet_size);

        // Вычисляем размеры
        uint32_t ethernet_size = sizeof(struct ether_header);
        const auto* ip_header = reinterpret_cast<const struct iphdr*>(packet + ethernet_size);
        uint32_t ip_header_size = (ip_header->ihl & 0x0F) * 4;

        const auto* tcp_header = reinterpret_cast<const struct tcphdr*>(
            packet + ethernet_size + ip_header_size);
        uint32_t tcp_header_size = ((tcp_header->doff & 0xF0) >> 4) * 4;

        uint32_t payload_size = packet_size - ethernet_size - ip_header_size - tcp_header_size;

        auto packet_info = std::make_unique<PacketInfo>();
        packet_info->flow_tuple = flow_tuple;
        packet_info->packet_size = packet_size;
        packet_info->payload_size = payload_size;
        packet_info->timestamp = timestamp;

        return packet_info;
    }
    catch(const std::exception& e)
    {
        std::cerr << "[error] Ошибка при парсинге пакета: " << e.what() << "\n";
        return nullptr;
    }
}

bool PacketParser::isTcpIpv4Packet(const u_char* packet, uint32_t packet_size)
{
    static int total_checked = 0;
    static int size_failed = 0;
    static int ether_type_failed = 0;
    static int protocol_failed = 0;

    total_checked++;

    // Проверяем минимальный размер
    if(packet_size < sizeof(struct ether_header) + sizeof(struct iphdr))
    {
        size_failed++;
        if(size_failed % 100 == 0)
        {
            std::cout << "[debug] Проверок размера пакета: " << size_failed << "/" << total_checked << "\n";
        }
        return false;
    }

    // Проверяем тип Ethernet кадра (IPv4 = 0x0800)
    const auto* eth_header = reinterpret_cast<const struct ether_header*>(packet);
    uint16_t ether_type = ntohs(eth_header->ether_type);
    if(ether_type != ETHERTYPE_IP)
    {
        ether_type_failed++;
        if(ether_type_failed % 50 == 0)
        {
            // std::cout << "[debug] Неверный тип Ethernet: " << std::hex << ether_type
            //     << " (ожидалось: " << ETHERTYPE_IP << "), всего: " << ether_type_failed << "/" << total_checked << "\n";

            // Дополнительная отладочная информация для первых нескольких пакетов
            if(ether_type_failed <= 3)
            {
                // std::cout << "[debug] Первые 16 байт пакета: ";
                // for(uint32_t i = 0; i < 16 && i < packet_size; i++)
                // {
                //     std::cout << std::hex << (int)packet[i] << " ";
                // }
                // std::cout << std::dec << "\n";

                // std::cout << "[debug] Ethernet тип: " << std::hex << ether_type << std::dec << "\n";
                // std::cout << "[debug] Размер пакета: " << packet_size << "\n";
            }
        }
        return false;
    }

    // Проверяем версию IP (IPv4 = 4)
    const auto* ip_header = reinterpret_cast<const struct iphdr*>(packet + sizeof(struct ether_header));
    if(ip_header->version != 4)
    {
        return false;
    }

    // Проверяем протокол (TCP = 6)
    if(ip_header->protocol != IPPROTO_TCP)
    {
        protocol_failed++;
        if(protocol_failed % 50 == 0)
        {
            // std::cout << "[debug] Неверный протокол: " << (int)ip_header->protocol
            //     << " (ожидалось: " << IPPROTO_TCP << "), всего: " << protocol_failed << "/" << total_checked << "\n";
        }
        return false;
    }

    return true;
}

std::string PacketParser::ipToString(uint32_t ip)
{
    struct in_addr addr{};
    addr.s_addr = ip;
    return inet_ntoa(addr);
}

FlowTuple PacketParser::extractFlowTuple(const u_char* packet, uint32_t packet_size)
{
    (void)packet_size; // Подавляем предупреждение о неиспользуемом параметре
    // Пропускаем Ethernet заголовок
    const auto* ip_header = reinterpret_cast<const struct iphdr*>(
        packet + sizeof(struct ether_header));

    // Получаем IP адреса
    uint32_t src_ip = ip_header->saddr;
    uint32_t dst_ip = ip_header->daddr;

    // Вычисляем смещение к TCP заголовку
    uint32_t ip_header_size = (ip_header->ihl & 0x0F) * 4;
    const auto* tcp_header = reinterpret_cast<const struct tcphdr*>(
        packet + sizeof(struct ether_header) + ip_header_size);

    // Получаем порты
    uint16_t src_port = ntohs(tcp_header->source);
    uint16_t dst_port = ntohs(tcp_header->dest);

    FlowTuple flow_tuple{};
    flow_tuple.src_ip = src_ip;
    flow_tuple.dst_ip = dst_ip;
    flow_tuple.src_port = src_port;
    flow_tuple.dst_port = dst_port;

    return flow_tuple;
}
