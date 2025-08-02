#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

// Заголовочные файлы sniffer
#include "../sniffer/logging/LogManager.h"
#include "../sniffer/logging/Logger.h"
#include "../sniffer/flow_tracker/FlowTracker.h"
#include "../sniffer/flow_tracker/FlowStats.h"
#include "../sniffer/statistics/StatisticsManager.h"
#include "../sniffer/packet_processor/PacketParser.h"

// Тесты для FlowTuple
class FlowTupleTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(FlowTupleTest, DefaultConstructor)
{
    FlowTuple tuple{};
    EXPECT_EQ(tuple.src_ip, 0);
    EXPECT_EQ(tuple.dst_ip, 0);
    EXPECT_EQ(tuple.src_port, 0);
    EXPECT_EQ(tuple.dst_port, 0);
}

TEST_F(FlowTupleTest, CustomConstructor)
{
    FlowTuple tuple{0x01020304, 0x05060708, 1234, 5678};
    EXPECT_EQ(tuple.src_ip, 0x01020304);
    EXPECT_EQ(tuple.dst_ip, 0x05060708);
    EXPECT_EQ(tuple.src_port, 1234);
    EXPECT_EQ(tuple.dst_port, 5678);
}

TEST_F(FlowTupleTest, ComparisonOperators)
{
    FlowTuple tuple1{0x01020304, 0x05060708, 1234, 5678};
    FlowTuple tuple2{0x01020304, 0x05060708, 1234, 5678};
    FlowTuple tuple3{0x01020304, 0x05060708, 1234, 5679};

    EXPECT_EQ(tuple1, tuple2);
    EXPECT_NE(tuple1, tuple3);
    EXPECT_LT(tuple1, tuple3);
}

TEST_F(FlowTupleTest, Ordering)
{
    FlowTuple tuple1{0x01020304, 0x05060708, 1234, 5678};
    FlowTuple tuple2{0x01020304, 0x05060708, 1234, 5679};
    FlowTuple tuple3{0x01020304, 0x05060708, 1235, 5678};
    FlowTuple tuple4{0x01020305, 0x05060708, 1234, 5678};

    EXPECT_LT(tuple1, tuple2);
    EXPECT_LT(tuple1, tuple3);
    EXPECT_LT(tuple1, tuple4);
}

// Тесты для FlowStats
class FlowStatsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(FlowStatsTest, DefaultConstructor)
{
    FlowStats stats;
    EXPECT_EQ(stats.getPacketCount(), 0);
    EXPECT_EQ(stats.getTotalBytes(), 0);
    EXPECT_EQ(stats.getAveragePacketSize(), 0.0);
    EXPECT_EQ(stats.getAverageSpeed(1000000), 0.0);
}

TEST_F(FlowStatsTest, UpdateStats)
{
    FlowStats stats;
    uint64_t timestamp = 1000000; // 1 секунда

    // Первый пакет
    stats.updateStats(100, 80, timestamp);
    EXPECT_EQ(stats.getPacketCount(), 1);
    EXPECT_EQ(stats.getTotalBytes(), 80);
    EXPECT_EQ(stats.getAveragePacketSize(), 100.0);

    // Второй пакет через 1 секунду
    stats.updateStats(150, 120, timestamp + 1000000);
    EXPECT_EQ(stats.getPacketCount(), 2);
    EXPECT_EQ(stats.getTotalBytes(), 200);
    EXPECT_EQ(stats.getAveragePacketSize(), 125.0);

    // Проверяем скорость (200 байт за 1 секунду = 200 байт/сек)
    EXPECT_NEAR(stats.getAverageSpeed(timestamp + 1000000), 200.0, 1.0);
}

TEST_F(FlowStatsTest, SpeedCalculation)
{
    FlowStats stats;
    uint64_t timestamp = 1000000;

    // Отправляем 1000 байт за 1 секунду
    stats.updateStats(1000, 800, timestamp);
    stats.updateStats(1000, 800, timestamp + 1000000);

    EXPECT_NEAR(stats.getAverageSpeed(timestamp + 1000000), 1600.0, 10.0); // 1600 байт/сек
}

TEST_F(FlowStatsTest, LastPacketTime)
{
    FlowStats stats;
    uint64_t timestamp = 1000000;

    EXPECT_EQ(stats.getLastPacketTime(), 0);

    stats.updateStats(100, 80, timestamp);
    EXPECT_EQ(stats.getLastPacketTime(), timestamp);

    stats.updateStats(150, 120, timestamp + 500000);
    EXPECT_EQ(stats.getLastPacketTime(), timestamp + 500000);
}

// Тесты для FlowTracker
class FlowTrackerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        flow_tracker = std::make_unique<FlowTracker>();
    }

    void TearDown() override
    {
        flow_tracker.reset();
    }

    std::unique_ptr<FlowTracker> flow_tracker;
};

TEST_F(FlowTrackerTest, UpdateFlow)
{
    FlowTuple tuple{0x01020304, 0x05060708, 1234, 5678};
    uint64_t timestamp = 1000000;

    // Добавляем первый пакет
    flow_tracker->updateFlow(tuple, 100, 80, timestamp);

    const FlowStats* stats = flow_tracker->getFlowStats(tuple);
    ASSERT_NE(stats, nullptr);
    EXPECT_EQ(stats->getPacketCount(), 1);
    EXPECT_EQ(stats->getTotalBytes(), 80); // payload bytes

    // Добавляем второй пакет в тот же поток
    flow_tracker->updateFlow(tuple, 150, 120, timestamp + 1000000);

    stats = flow_tracker->getFlowStats(tuple);
    ASSERT_NE(stats, nullptr);
    EXPECT_EQ(stats->getPacketCount(), 2);
    EXPECT_EQ(stats->getTotalBytes(), 200); // payload bytes
}

TEST_F(FlowTrackerTest, MultipleFlows)
{
    FlowTuple tuple1{0x01020304, 0x05060708, 1234, 5678};
    FlowTuple tuple2{0x02030405, 0x06070809, 2345, 6789};
    uint64_t timestamp = 1000000;

    // Обновляем разные потоки
    flow_tracker->updateFlow(tuple1, 100, 80, timestamp);
    flow_tracker->updateFlow(tuple2, 200, 160, timestamp);
    flow_tracker->updateFlow(tuple1, 150, 120, timestamp + 1000000);

    const FlowStats* stats1 = flow_tracker->getFlowStats(tuple1);
    const FlowStats* stats2 = flow_tracker->getFlowStats(tuple2);

    ASSERT_NE(stats1, nullptr);
    ASSERT_NE(stats2, nullptr);

    EXPECT_EQ(stats1->getPacketCount(), 2);
    EXPECT_EQ(stats1->getTotalBytes(), 200); // payload bytes

    EXPECT_EQ(stats2->getPacketCount(), 1);
    EXPECT_EQ(stats2->getTotalBytes(), 160); // payload bytes
}

TEST_F(FlowTrackerTest, GetAllFlows)
{
    FlowTuple tuple1{0x01020304, 0x05060708, 1234, 5678};
    FlowTuple tuple2{0x02030405, 0x06070809, 2345, 6789};
    uint64_t timestamp = 1000000;

    flow_tracker->updateFlow(tuple1, 100, 80, timestamp);
    flow_tracker->updateFlow(tuple2, 200, 160, timestamp);

    auto all_flows = flow_tracker->getAllFlows();
    EXPECT_EQ(all_flows.size(), 2);

    EXPECT_NE(all_flows.find(tuple1), all_flows.end());
    EXPECT_NE(all_flows.find(tuple2), all_flows.end());
}

TEST_F(FlowTrackerTest, CleanupOldFlows)
{
    FlowTuple tuple{0x01020304, 0x05060708, 1234, 5678};
    uint64_t timestamp = 1000000;

    flow_tracker->updateFlow(tuple, 100, 80, timestamp);

    // Проверяем, что поток существует
    EXPECT_NE(flow_tracker->getFlowStats(tuple), nullptr);

    // Очищаем потоки старше 1 секунды (текущее время + 2 секунды)
    flow_tracker->cleanupOldFlows(1);

    // Поток должен быть удален
    EXPECT_EQ(flow_tracker->getFlowStats(tuple), nullptr);
}

TEST_F(FlowTrackerTest, ActiveFlowCount)
{
    FlowTuple tuple1{0x01020304, 0x05060708, 1234, 5678};
    FlowTuple tuple2{0x02030405, 0x06070809, 2345, 6789};
    uint64_t timestamp = 1000000;

    EXPECT_EQ(flow_tracker->getActiveFlowCount(), 0);

    flow_tracker->updateFlow(tuple1, 100, 80, timestamp);
    EXPECT_EQ(flow_tracker->getActiveFlowCount(), 1);

    flow_tracker->updateFlow(tuple2, 200, 160, timestamp);
    EXPECT_EQ(flow_tracker->getActiveFlowCount(), 2);
}

// Тесты для PacketParser
class PacketParserTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(PacketParserTest, IsTcpIpv4Packet)
{
    // Создаем минимальный TCP/IP пакет
    std::vector<uint8_t> packet(60, 0); // 60 байт

    // Ethernet заголовок (14 байт)
    packet[12] = 0x08; // EtherType = IPv4
    packet[13] = 0x00;

    // IP заголовок (начиная с байта 14)
    packet[14] = 0x45; // Version=4, IHL=5
    packet[23] = 0x06; // Protocol = TCP

    // TCP заголовок (начиная с байта 34)
    packet[34] = 0x12; // Source port
    packet[35] = 0x34;
    packet[36] = 0x56; // Destination port
    packet[37] = 0x78;

    EXPECT_TRUE(PacketParser::isTcpIpv4Packet(packet.data(), packet.size()));
}

TEST_F(PacketParserTest, ExtractFlowTuple)
{
    // Создаем тестовый пакет
    std::vector<uint8_t> packet(60, 0);

    // Ethernet заголовок
    packet[12] = 0x08;
    packet[13] = 0x00;

    // IP заголовок
    packet[14] = 0x45;
    packet[23] = 0x06;

    // IP адреса (байты 26-33)
    packet[26] = 0x01; // Source IP: 1.2.3.4
    packet[27] = 0x02;
    packet[28] = 0x03;
    packet[29] = 0x04;

    packet[30] = 0x05; // Destination IP: 5.6.7.8
    packet[31] = 0x06;
    packet[32] = 0x07;
    packet[33] = 0x08;

    // TCP порты (байты 34-37)
    packet[34] = 0x12; // Source port: 4660
    packet[35] = 0x34;
    packet[36] = 0x56; // Destination port: 22136
    packet[37] = 0x78;

    PacketParser parser;
    auto packet_info = parser.parsePacket(packet.data(), packet.size(), 1000000);
    ASSERT_NE(packet_info, nullptr);

    // Учитываем порядок байт (network byte order)
    EXPECT_EQ(packet_info->flow_tuple.src_ip, 0x04030201);
    EXPECT_EQ(packet_info->flow_tuple.dst_ip, 0x08070605);
    EXPECT_EQ(packet_info->flow_tuple.src_port, 4660);
    EXPECT_EQ(packet_info->flow_tuple.dst_port, 22136);
}

TEST_F(PacketParserTest, IpToString)
{
    uint32_t ip = 0x01020304; // 1.2.3.4
    std::string ip_str = PacketParser::ipToString(ip);
    EXPECT_EQ(ip_str, "4.3.2.1"); // Обратный порядок байт

    ip = 0x0A0B0C0D; // 10.11.12.13
    ip_str = PacketParser::ipToString(ip);
    EXPECT_EQ(ip_str, "13.12.11.10"); // Обратный порядок байт
}

// Тесты для StatisticsManager
class StatisticsManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stats_manager = std::make_unique<StatisticsManager>();
        flow_tracker = std::make_unique<FlowTracker>();
        stats_manager->setFlowTracker(*flow_tracker);
    }

    void TearDown() override
    {
        stats_manager.reset();
        flow_tracker.reset();
    }

    std::unique_ptr<StatisticsManager> stats_manager;
    std::unique_ptr<FlowTracker> flow_tracker;
};

TEST_F(StatisticsManagerTest, UpdateFlowStats)
{
    FlowTuple tuple{0x01020304, 0x05060708, 1234, 5678};
    uint64_t timestamp = 1000000;

    stats_manager->updateFlowStats(tuple, 100, 80, timestamp);

    // Проверяем, что статистика обновилась
    const FlowStats* stats = flow_tracker->getFlowStats(tuple);
    ASSERT_NE(stats, nullptr);
    EXPECT_EQ(stats->getPacketCount(), 1);
    EXPECT_EQ(stats->getTotalBytes(), 80); // payload bytes
}

TEST_F(StatisticsManagerTest, PrintTopFlows)
{
    // Создаем несколько потоков с разной скоростью
    FlowTuple tuple1{0x01020304, 0x05060708, 1234, 5678};
    FlowTuple tuple2{0x02030405, 0x06070809, 2345, 6789};
    FlowTuple tuple3{0x03040506, 0x0708090A, 3456, 7890};

    uint64_t timestamp = 1000000;

    // Поток 1: 100 байт/сек
    stats_manager->updateFlowStats(tuple1, 100, 80, timestamp);
    stats_manager->updateFlowStats(tuple1, 100, 80, timestamp + 1000000);

    // Поток 2: 200 байт/сек
    stats_manager->updateFlowStats(tuple2, 200, 160, timestamp);
    stats_manager->updateFlowStats(tuple2, 200, 160, timestamp + 1000000);

    // Поток 3: 300 байт/сек
    stats_manager->updateFlowStats(tuple3, 300, 240, timestamp);
    stats_manager->updateFlowStats(tuple3, 300, 240, timestamp + 1000000);

    // Проверяем, что все потоки существуют
    EXPECT_NE(flow_tracker->getFlowStats(tuple1), nullptr);
    EXPECT_NE(flow_tracker->getFlowStats(tuple2), nullptr);
    EXPECT_NE(flow_tracker->getFlowStats(tuple3), nullptr);
}

TEST_F(StatisticsManagerTest, CleanupOldFlows)
{
    FlowTuple tuple{0x01020304, 0x05060708, 1234, 5678};
    uint64_t timestamp = 1000000;

    stats_manager->updateFlowStats(tuple, 100, 80, timestamp);

    // Проверяем, что поток существует
    EXPECT_NE(flow_tracker->getFlowStats(tuple), nullptr);

    // Очищаем старые потоки
    stats_manager->cleanupOldFlows();

    // Поток должен быть удален
    EXPECT_EQ(flow_tracker->getFlowStats(tuple), nullptr);
}

// Интеграционные тесты
class SnifferIntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        LogManager::initialize(false, "sniffer_integration_test");
        flow_tracker = std::make_unique<FlowTracker>();
        stats_manager = std::make_unique<StatisticsManager>();
        stats_manager->setFlowTracker(*flow_tracker);
    }

    void TearDown() override
    {
        flow_tracker.reset();
        stats_manager.reset();
    }

    std::unique_ptr<FlowTracker> flow_tracker;
    std::unique_ptr<StatisticsManager> stats_manager;
};

TEST_F(SnifferIntegrationTest, PacketProcessingPipeline)
{
    // Создаем тестовый пакет
    std::vector<uint8_t> packet(60, 0);

    // Настраиваем пакет как TCP/IP
    packet[12] = 0x08; // EtherType = IPv4
    packet[13] = 0x00;
    packet[14] = 0x45; // Version=4, IHL=5
    packet[23] = 0x06; // Protocol = TCP

    // IP адреса
    packet[26] = 0x01; // Source IP: 1.2.3.4
    packet[27] = 0x02;
    packet[28] = 0x03;
    packet[29] = 0x04;
    packet[30] = 0x05; // Destination IP: 5.6.7.8
    packet[31] = 0x06;
    packet[32] = 0x07;
    packet[33] = 0x08;

    // TCP порты
    packet[34] = 0x12; // Source port: 4660
    packet[35] = 0x34;
    packet[36] = 0x56; // Destination port: 22136
    packet[37] = 0x78;

    uint64_t timestamp = 1000000;

    // Проверяем, что это TCP/IP пакет
    EXPECT_TRUE(PacketParser::isTcpIpv4Packet(packet.data(), packet.size()));

    // Парсим пакет
    PacketParser parser;
    auto packet_info = parser.parsePacket(packet.data(), packet.size(), timestamp);
    ASSERT_NE(packet_info, nullptr);

    // Обновляем статистику только один раз
    stats_manager->updateFlowStats(packet_info->flow_tuple, packet_info->packet_size,
                                   packet_info->payload_size, packet_info->timestamp);

    // Проверяем, что поток создался
    const FlowStats* stats = flow_tracker->getFlowStats(packet_info->flow_tuple);
    ASSERT_NE(stats, nullptr);
    EXPECT_EQ(stats->getPacketCount(), 1);
    // getTotalBytes возвращает payload bytes
    EXPECT_EQ(stats->getTotalBytes(), packet_info->payload_size);
}

// Тесты производительности
class SnifferPerformanceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        flow_tracker = std::make_unique<FlowTracker>();
    }

    void TearDown() override
    {
        flow_tracker.reset();
    }

    std::unique_ptr<FlowTracker> flow_tracker;
};

TEST_F(SnifferPerformanceTest, HighSpeedPacketProcessing)
{
    constexpr int num_packets = 10000;

    auto start = std::chrono::high_resolution_clock::now();

    for(int i = 0; i < num_packets; ++i)
    {
        constexpr int num_flows = 100;
        FlowTuple tuple{
            static_cast<uint32_t>(i % num_flows),
            static_cast<uint32_t>(i % num_flows + 1),
            static_cast<uint16_t>(i % 65535),
            static_cast<uint16_t>((i + 1) % 65535)
        };

        uint64_t timestamp = 1000000 + i * 1000; // 1ms между пакетами
        flow_tracker->updateFlow(tuple, 100 + (i % 100), 80 + (i % 80), timestamp);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Проверяем производительность
    EXPECT_LT(duration.count(), 100000); // Менее 100ms для 10000 пакетов

    // Проверяем, что потоки создались
    // Каждый поток может быть уникальным из-за разных портов
    EXPECT_GT(flow_tracker->getActiveFlowCount(), 0);
}

// Тесты для многопоточности
class SnifferThreadingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        flow_tracker = std::make_unique<FlowTracker>();
    }

    void TearDown() override
    {
        flow_tracker.reset();
    }

    std::unique_ptr<FlowTracker> flow_tracker;
};

TEST_F(SnifferThreadingTest, ConcurrentFlowUpdates)
{
    constexpr int num_threads = 10;
    constexpr int packets_per_thread = 1000;
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    // Запускаем несколько потоков, которые одновременно обновляют потоки
    for(int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([this, i, packets_per_thread]()
        {
            for(int j = 0; j < packets_per_thread; ++j)
            {
                FlowTuple tuple{
                    static_cast<uint32_t>(i * 1000 + j),
                    static_cast<uint32_t>(i * 1000 + j + 1),
                    static_cast<uint16_t>(i * 100 + j),
                    static_cast<uint16_t>(i * 100 + j + 1)
                };

                uint64_t timestamp = 1000000 + j * 1000;
                flow_tracker->updateFlow(tuple, 100, 80, timestamp);
            }
        });
    }

    // Ждем завершения всех потоков
    for(auto& thread : threads)
    {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Проверяем, что все операции выполнились без ошибок
    EXPECT_GT(duration.count(), 0);
    EXPECT_EQ(flow_tracker->getActiveFlowCount(), num_threads * packets_per_thread);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
