# Sniffer: Анализатор Сетевого Трафика

Этот каталог содержит приложение для анализа и мониторинга сетевого трафика в реальном времени с использованием libpcap.

## Архитектура приложения

### Многопоточная архитектура

- **Основной поток**: вывод статистики и управление приложением
    - `main()` → `runSniffer()` → цикл вывода статистики каждую секунду
- **Поток обработки пакетов**: захват и обработка сетевых пакетов
    - `PacketProcessor::start()` → `std::thread(&PacketProcessor::packetLoop, this)`
- Синхронизация через атомарные переменные
    - `std::atomic<bool> m_running` в PacketProcessor

### Основные компоненты

#### Обработка пакетов (`packet_processor/`)

- **PacketProcessor** (`packet_processor/PacketProcessor.h/cpp`) - основной процессор сетевых пакетов
    - `PacketProcessor::start()` - запуск обработки пакетов
    - `PacketProcessor::stop()` - остановка обработки пакетов
    - `PacketProcessor::packetLoop()` - основной цикл обработки пакетов
    - `PacketProcessor::processPacket()` - обработка одного пакета
    - `PacketProcessor::initializePcap()` - инициализация libpcap
- **PacketParser** (`packet_processor/PacketParser.h/cpp`) - парсер заголовков пакетов (Ethernet, IP, TCP)
    - `PacketParser::parsePacket()` - парсинг пакета
    - `PacketParser::isTcpIpv4Packet()` - проверка TCP/IPv4 пакета
    - `PacketParser::extractFlowTuple()` - извлечение 4-tuple
    - `PacketParser::ipToString()` - преобразование IP в строку

#### Отслеживание потоков (`flow_tracker/`)

- **FlowTracker** (`flow_tracker/FlowTracker.h/cpp`) - трекер сетевых потоков
    - `FlowTracker::updateFlow()` - обновление статистики потока
    - `FlowTracker::getFlowStats()` - получение статистики потока
    - `FlowTracker::getAllFlows()` - получение всех потоков
    - `FlowTracker::cleanupOldFlows()` - очистка старых потоков
    - `FlowTracker::getActiveFlowCount()` - количество активных потоков
- **FlowStats** (`flow_tracker/FlowStats.h/cpp`) - статистика по потокам
    - `FlowStats::updateStats()` - обновление статистики
    - `FlowStats::getAveragePacketSize()` - средний размер пакета
    - `FlowStats::getAverageSpeed()` - средняя скорость передачи
    - `FlowStats::reset()` - сброс статистики
- **FlowTuple** (`packet_processor/PacketParser.h`) - 4-tuple идентификация потоков (определена в PacketParser.h)
    - `src_ip`, `dst_ip`, `src_port`, `dst_port` - поля 4-tuple
    - `operator<()` - для использования в std::map
    - `operator==()` - сравнение потоков

#### Статистика (`statistics/`)

- **StatisticsManager** (`statistics/StatisticsManager.h/cpp`) - менеджер статистики и метрик
    - `StatisticsManager::updateFlowStats()` - обновление статистики потоков
    - `StatisticsManager::printTopFlows()` - вывод топ-10 потоков
    - `StatisticsManager::getTopFlows()` - получение топ потоков
    - `StatisticsManager::cleanupOldFlows()` - очистка старых потоков
    - `StatisticsManager::formatSpeed()` - форматирование скорости

#### Система логирования (`logging/`)

- **Logger** (`logging/Logger.h/cpp`) - базовый логгер
    - `Logger::log_start()` - начало логирования функции
    - `Logger::log_stop()` - завершение логирования функции
    - `Logger::log_message()` - логирование сообщения
- **LogManager** (`logging/LogManager.h/cpp`) - менеджер логирования
    - `LogManager::initialize()` - инициализация логирования
    - `LogManager::get_logger()` - получение логгера
- **LogMacros** (`logging/LogMacros.h`) - макросы для удобного логирования
    - `LOG_FUNCTION()` - автоматическое логирование функций
    - `LOG_MESSAGE()` - логирование сообщений

### Точка входа

- **main.cpp** - главный файл приложения
    - `signalHandler()` - обработчик сигналов SIGINT/SIGTERM
    - `parseCommandLine()` - разбор аргументов командной строки
    - `runSniffer()` - запуск sniffer приложения

## Функциональность

### Захват пакетов

- **Захват сетевых пакетов через libpcap**
    - `PacketProcessor::initializePcap()` → `pcap_open_live()`
    - `PacketProcessor::packetLoop()` → `pcap_dispatch()`
- **Фильтрация пакетов по протоколам**
    - `PacketParser::isTcpIpv4Packet()` - только TCP/IPv4 пакеты
- **Обработка в реальном времени**
    - `PacketProcessor::packetLoop()` - непрерывный цикл обработки
- **Поддержка различных сетевых интерфейсов**
    - Параметр `--interface` в командной строке
- **Многопоточная архитектура (основной поток + поток обработки пакетов)**
    - Основной поток: `runSniffer()` с циклом вывода статистики
    - Поток пакетов: `PacketProcessor::packetLoop()` в отдельном `std::thread`

### Анализ пакетов

- **PacketParser: разбор заголовков Ethernet, IP, TCP (только TCP/IPv4)**
    - `PacketParser::parsePacket()` - полный парсинг пакета
    - `PacketParser::extractFlowTuple()` - извлечение 4-tuple
- **Извлечение метаданных пакетов**
    - `PacketInfo` структура: `packet_size`, `payload_size`, `timestamp`
- **Определение протоколов и портов**
    - `PacketParser::isTcpIpv4Packet()` - проверка протокола
    - `FlowTuple` - извлечение портов из TCP заголовка
- **Валидация целостности пакетов**
    - Проверка минимального размера пакета в `isTcpIpv4Packet()`

### Отслеживание потоков

- **FlowTracker: группировка пакетов по потокам**
    - `FlowTracker::updateFlow()` - обновление статистики потока
    - `std::map<FlowTuple, FlowStats> m_flows` - хранение потоков
- **FlowTuple: идентификация потоков по 4-tuple (src_ip, dst_ip, src_port, dst_port)**
    - Структура `FlowTuple` в `PacketParser.h`
    - `PacketParser::extractFlowTuple()` - извлечение из пакета
- **FlowStats: сбор статистики по потокам**
    - `FlowStats::updateStats()` - обновление счетчиков
    - `packet_count`, `total_bytes`, `total_payload`, `first_seen`, `last_seen`
- **Отслеживание состояния соединений**
    - `FlowTracker::cleanupOldFlows()` - удаление неактивных потоков

### Статистика и метрики

- **StatisticsManager: агрегация статистики**
    - `StatisticsManager::updateFlowStats()` - обновление статистики
    - `StatisticsManager::getTopFlows()` - сортировка по скорости
- **Метрики производительности**
    - `StatisticsManager::printTopFlows()` - вывод каждую секунду
- **Статистика по протоколам**
    - Только TCP/IPv4 (фильтрация в `isTcpIpv4Packet()`)
- **Отчеты по потокам**
    - Топ-10 потоков по скорости передачи данных

### Система логирования

- **Логирование функций и сообщений**
    - `Logger::log_start()`, `Logger::log_stop()`, `Logger::log_message()`
- **Логирование событий захвата**
    - `LOG_FUNCTION()`, `LOG_MESSAGE()` в PacketProcessor
- **Статистические отчеты**
    - `StatisticsManager::printTopFlows()` - вывод статистики

## Возможности анализа

### Протоколы

- **Ethernet**: анализ кадров Ethernet
    - `PacketParser::parsePacket()` - разбор Ethernet заголовка
- **IP**: анализ заголовков IPv4
    - Извлечение IP адресов из IP заголовка
- **TCP**: анализ TCP соединений (только TCP/IPv4)
    - `PacketParser::isTcpIpv4Packet()` - проверка протокола
    - Извлечение портов из TCP заголовка

### Статистика потоков

- **Количество пакетов и байт**
    - `FlowStats::packet_count`, `FlowStats::total_bytes`
- **Время жизни потока**
    - `FlowStats::first_seen`, `FlowStats::last_seen`
- **Средняя скорость передачи**
    - `FlowStats::getAverageSpeed()` - байт/сек
- **Средний размер пакета**
    - `FlowStats::getAveragePacketSize()` - байт/пакет

### Метрики производительности

- **Скорость обработки пакетов**
    - Количество пакетов в секунду (неявно)
- **Статистика ошибок**
    - Логирование ошибок в `PacketProcessor::packetLoop()`

## Конфигурация

### Параметры захвата

- **Сетевой интерфейс (обязательно)**
    - Параметр `--interface` в командной строке
    - Передается в `PacketProcessor::PacketProcessor(interface, ...)`

### Настройки логирования

- **Включение/отключение логирования**
    - Параметр `--log` в командной строке
    - `LogManager::initialize(enable_logging, "sniffer")`
- **Формат логов**
    - `Logger::Logger()` - создание файла с временной меткой

## Технические требования

- **Стандарт C++**: C++20
    - `set(CMAKE_CXX_STANDARD 20)` в корневом CMakeLists.txt
- **Системные библиотеки**: libpcap, pthread
    - `target_link_libraries(sniffer PRIVATE pcap pthread)` в sniffer/CMakeLists.txt
    - `#include <pcap.h>` в PacketProcessor.cpp
- **Сборка**: CMake
    - Корневой CMakeLists.txt + подпроекты
- **Платформа**: Linux
    - Использование libpcap для захвата пакетов
- **Привилегии**: root (для захвата пакетов)
    - `pcap_open_live()` требует привилегий root

## Структура файлов

```
sniffer/
├── main.cpp                    # Главный файл приложения
├── packet_processor/
│   ├── PacketProcessor.h/cpp   # Основной процессор пакетов
│   ├── PacketParser.h/cpp      # Парсер заголовков пакетов
│   └── CMakeLists.txt          # CMake для библиотеки обработки пакетов
├── flow_tracker/
│   ├── FlowTracker.h/cpp       # Трекер потоков
│   ├── FlowStats.h/cpp         # Статистика потоков
│   └── CMakeLists.txt          # CMake для библиотеки трекера
├── statistics/
│   ├── StatisticsManager.h/cpp # Менеджер статистики
│   └── CMakeLists.txt          # CMake для библиотеки статистики
├── logging/
│   ├── Logger.h/cpp            # Базовый логгер
│   ├── LogManager.h/cpp        # Менеджер логирования
│   ├── LogMacros.h             # Макросы логирования
│   └── CMakeLists.txt          # CMake для библиотеки логирования
└── CMakeLists.txt              # CMake для sniffer (наследует от корневого)
```

**Примечание:** Проект собирается из корневой директории. Корневой `CMakeLists.txt` определяет общие настройки (C++20,
флаги компиляции), а gen-app, sniffer и tests собираются как подпроекты.
