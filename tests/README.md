# Юнит-тесты с Google Test Framework

Этот каталог содержит комплексные юнит-тесты для проекта InfoTecs_Issledovatel_Pro, написанные с использованием Google
Test Framework.

## Структура тестов

### gen_app_tests.cpp

Тесты для компонентов gen-app (TCP клиент/сервер):

- **ServerConfigTest** - тесты конфигурации сервера
- **ClientConfigTest** - тесты конфигурации клиента
- **LogManagerTest** - тесты системы логирования
- **EpollManagerTest** - тесты управления epoll
- **SocketManagerTest** - тесты работы с сокетами
- **IntegrationTest** - интеграционные тесты
- **PerformanceTest** - тесты производительности
- **ThreadingTest** - тесты многопоточности

### sniffer_tests.cpp

Тесты для компонентов sniffer (анализатор сетевого трафика):

- **FlowTupleTest** - тесты 4-tuple потоков
- **FlowStatsTest** - тесты статистики потоков
- **FlowTrackerTest** - тесты трекера потоков
- **PacketParserTest** - тесты парсинга пакетов
- **StatisticsManagerTest** - тесты менеджера статистики
- **SnifferIntegrationTest** - интеграционные тесты
- **SnifferPerformanceTest** - тесты производительности
- **SnifferThreadingTest** - тесты многопоточности

## Статистика тестов

### Gen-app тесты

- **Всего тестов:** 20
- **Тестовых наборов:** 8
- **Покрытие:** Все основные компоненты

### Sniffer тесты

- **Всего тестов:** 22
- **Тестовых наборов:** 8
- **Покрытие:** Все основные компоненты

## Требования

- Google Test Framework (установлен в системе)
- C++20 компилятор
- libpcap (для sniffer тестов)
- pthread
