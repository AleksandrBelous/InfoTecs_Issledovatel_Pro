#ifndef PACKET_PARSER_H
#define PACKET_PARSER_H

#include <string>
#include <memory>
#include <netinet/ip.h>


/**
 * @brief Структура для хранения 4-tuple потока
 */
struct FlowTuple
{
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;

    /**
     * @brief Оператор сравнения для использования в контейнерах
     */
    bool operator<(const FlowTuple& other) const
    {
        if(src_ip != other.src_ip) return src_ip < other.src_ip;
        if(dst_ip != other.dst_ip) return dst_ip < other.dst_ip;
        if(src_port != other.src_port) return src_port < other.src_port;
        return dst_port < other.dst_port;
    }

    /**
     * @brief Оператор равенства
     */
    bool operator==(const FlowTuple& other) const
    {
        return src_ip == other.src_ip && dst_ip == other.dst_ip &&
            src_port == other.src_port && dst_port == other.dst_port;
    }
};

/**
 * @brief Структура для хранения информации о пакете
 */
struct PacketInfo
{
    FlowTuple flow_tuple;
    uint32_t packet_size; // Размер пакета на уровне Ethernet
    uint32_t payload_size; // Размер полезной нагрузки TCP
    uint64_t timestamp; // Временная метка в микросекундах
};

/**
 * @brief Класс для парсинга сетевых пакетов
 */
class PacketParser
{
public:
    /**
     * @brief Конструктор
     */
    PacketParser() = default;

    /**
     * @brief Деструктор
     */
    ~PacketParser() = default;

    /**
     * @brief Парсинг пакета
     * @param packet Указатель на данные пакета
     * @param packet_size Размер пакета
     * @param timestamp Временная метка пакета
     * @return Информация о пакете или nullptr если парсинг не удался
     */
    static std::unique_ptr<PacketInfo> parsePacket(const u_char* packet, uint32_t packet_size, uint64_t timestamp);

    /**
     * @brief Проверка является ли пакет TCP/IPv4
     * @param packet Указатель на данные пакета
     * @param packet_size Размер пакета
     * @return true если пакет TCP/IPv4
     */
    static bool isTcpIpv4Packet(const u_char* packet, uint32_t packet_size);

    /**
     * @brief Преобразование IP адреса в строку
     * @param ip IP адрес в сетевом порядке байт
     * @return Строковое представление IP адреса
     */
    static std::string ipToString(uint32_t ip);

private:
    /**
     * @brief Извлечение 4-tuple из TCP/IP пакета
     * @param packet Указатель на данные пакета
     * @param packet_size Размер пакета
     * @return 4-tuple потока
     */
    static FlowTuple extractFlowTuple(const u_char* packet, uint32_t packet_size);
};

#endif // PACKET_PARSER_H
