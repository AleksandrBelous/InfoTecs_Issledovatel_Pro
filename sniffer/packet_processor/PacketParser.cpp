#include "PacketParser.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

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
        const struct iphdr* ip_header = reinterpret_cast<const struct iphdr*>(packet + ethernet_size);
        uint32_t ip_header_size = (ip_header->ihl & 0x0F) * 4;

        const struct tcphdr* tcp_header = reinterpret_cast<const struct tcphdr*>(
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
    // Проверяем минимальный размер
    if(packet_size < sizeof(struct ether_header) + sizeof(struct iphdr))
    {
        return false;
    }

    // Проверяем тип Ethernet кадра (IPv4 = 0x0800)
    const struct ether_header* eth_header = reinterpret_cast<const struct ether_header*>(packet);
    uint16_t ether_type = ntohs(eth_header->ether_type);
    if(ether_type != ETHERTYPE_IP)
    {
        return false;
    }

    // Проверяем версию IP (IPv4 = 4)
    const struct iphdr* ip_header = reinterpret_cast<const struct iphdr*>(packet + sizeof(struct ether_header));
    if((ip_header->version & 0xF0) != 0x40) // IPv4
    {
        return false;
    }

    // Проверяем протокол (TCP = 6)
    if(ip_header->protocol != IPPROTO_TCP)
    {
        return false;
    }

    return true;
}

std::string PacketParser::ipToString(uint32_t ip)
{
    struct in_addr addr;
    addr.s_addr = ip;
    return inet_ntoa(addr);
}

FlowTuple PacketParser::extractFlowTuple(const u_char* packet, uint32_t packet_size)
{
    (void)packet_size; // Подавляем предупреждение о неиспользуемом параметре
    // Пропускаем Ethernet заголовок
    const struct iphdr* ip_header = reinterpret_cast<const struct iphdr*>(
        packet + sizeof(struct ether_header));

    // Получаем IP адреса
    uint32_t src_ip = ip_header->saddr;
    uint32_t dst_ip = ip_header->daddr;

    // Вычисляем смещение к TCP заголовку
    uint32_t ip_header_size = (ip_header->ihl & 0x0F) * 4;
    const struct tcphdr* tcp_header = reinterpret_cast<const struct tcphdr*>(
        packet + sizeof(struct ether_header) + ip_header_size);

    // Получаем порты
    uint16_t src_port = ntohs(tcp_header->source);
    uint16_t dst_port = ntohs(tcp_header->dest);

    FlowTuple flow_tuple;
    flow_tuple.src_ip = src_ip;
    flow_tuple.dst_ip = dst_ip;
    flow_tuple.src_port = src_port;
    flow_tuple.dst_port = dst_port;

    return flow_tuple;
}
