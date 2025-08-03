# Gen-app: TCP Клиент/Сервер Приложение

Этот каталог содержит приложение для генерации и обработки TCP соединений, состоящее из клиентской и серверной частей.

## Архитектура приложения

### Основные компоненты

#### Клиентская часть (`client/`)

- **TCPClient** (`client/TCPClient.h/cpp`) - TCP клиент для подключения к серверу
    - `TCPClient::initialize()` - инициализация клиента
    - `TCPClient::run()` - основной цикл работы клиента
    - `TCPClient::startConnection()` - создание нового соединения
    - `TCPClient::handleConnectionEvent()` - обработка событий соединения
    - `TCPClient::restartConnection()` - пересоздание соединения
- **ClientConfig** (`client/ClientConfig.h`) - конфигурация клиентских параметров
    - `host`, `port`, `connections`, `seed` - параметры конфигурации

#### Серверная часть (`server/`)

- **TCPServer** (`server/TCPServer.h/cpp`) - TCP сервер для приема соединений
    - `TCPServer::initialize()` - инициализация сервера
    - `TCPServer::run()` - основной цикл работы сервера
    - `TCPServer::handleNewConnections()` - обработка новых подключений
    - `TCPServer::handleClientData()` - обработка данных от клиентов
    - `TCPServer::handleEpollEvents()` - обработка событий epoll
- **ServerConfig** (`server/ServerConfig.h/cpp`) - конфигурация серверных параметров
    - `host`, `port` - параметры конфигурации

#### Сетевая подсистема (`network/`)

- **EpollManager** (`network/EpollManager.h/cpp`) - управление epoll для неблокирующего I/O
    - `EpollManager::initialize()` - создание epoll-инстанса
    - `EpollManager::addFileDescriptor()` - добавление файлового дескриптора
    - `EpollManager::waitForEvents()` - ожидание событий
    - `EpollManager::removeFileDescriptor()` - удаление файлового дескриптора
- **SocketManager** (`network/SocketManager.h/cpp`) - управление сокетами и соединениями
    - `SocketManager::createServerSocket()` - создание серверного сокета
    - `SocketManager::createClientSocket()` - создание клиентского сокета
    - `SocketManager::acceptConnection()` - принятие соединения
    - `SocketManager::sendData()` - отправка данных
    - `SocketManager::receiveData()` - прием данных
    - `SocketManager::setNonBlocking()` - установка неблокирующего режима

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
    - `parseAddress()` - разбор адреса в формате host:port
    - `parseCommandLine()` - разбор аргументов командной строки
    - `runServer()` - запуск сервера
    - `runClient()` - запуск клиента

## Функциональность

### TCP Сервер

- **Неблокирующее принятие соединений через epoll**
    - `TCPServer::initialize()` → `EpollManager::addFileDescriptor(server_fd_, EPOLLIN)`
    - `TCPServer::handleEpollEvents()` → `EpollManager::waitForEvents()`
    - `TCPServer::handleNewConnections()` → `SocketManager::acceptConnection()`
- **Событийно-ориентированная обработка клиентских запросов**
    - `TCPServer::handleEpollEvents()` - основной цикл обработки событий
    - `TCPServer::handleClientData()` - обработка данных от клиентов
- **Однопоточная архитектура**
    - Один поток в `TCPServer::run()` с циклом `handleEpollEvents()`
- **Настраиваемые параметры**
    - `ServerConfig::getHost()`, `ServerConfig::getPort()`
- **Логирование всех операций**
    - `LOG_FUNCTION()`, `LOG_MESSAGE()` во всех методах TCPServer

### TCP Клиент

- **Подключение к серверу по TCP**
    - `TCPClient::startConnection()` → `SocketManager::createClientSocket()`
- **Отправка случайного количества данных (32-1024 байта)**
    - `TCPClient::handleConnectionEvent()` → `SocketManager::sendData()`
    - Генерация размера: `std::uniform_int_distribution<size_t> dist(32, 1024)`
- **Поддержание постоянного количества параллельных соединений**
    - `TCPClient::checkConnectionCount()` - отслеживание количества
    - `TCPClient::restartConnection()` - пересоздание при закрытии
- **Автоматическое переподключение при разрыве соединений**
    - `TCPClient::restartConnection()` → `TCPClient::startConnection()`
- **Обработка ошибок соединения**
    - `TCPClient::handleConnectionEvent()` - обработка `EPOLLERR`, `EPOLLHUP`

### Сетевая подсистема

- **EpollManager: управление событиями epoll для эффективного I/O**
    - `EpollManager::initialize()` - создание epoll-инстанса
    - `EpollManager::waitForEvents()` - ожидание событий с таймаутом
- **SocketManager: создание и управление сокетами**
    - `SocketManager::createServerSocket()` - создание серверного сокета
    - `SocketManager::createClientSocket()` - создание клиентского сокета
    - `SocketManager::setNonBlocking()` - установка неблокирующего режима
- **Поддержка множественных соединений**
    - `std::unordered_set<int> client_fds_` в TCPServer
    - `std::map<int, Connection> connections_` в TCPClient
- **Неблокирующая обработка данных**
    - `SocketManager::sendData()`, `SocketManager::receiveData()` с проверкой `EAGAIN`

### Система логирования

- **Логирование функций и сообщений**
    - `Logger::log_start()`, `Logger::log_stop()`, `Logger::log_message()`
- **Запись в файлы с временными метками**
    - `Logger::Logger()` - создание файла с временной меткой
- **Настраиваемые форматы вывода**
    - `Logger::get_indent()` - форматирование отступов

## Конфигурация

### Серверная конфигурация

- **IP-адрес для прослушивания (по умолчанию 0.0.0.0)**
    - `ServerConfig::host_` с значением по умолчанию "0.0.0.0"
- **Порт для прослушивания**
    - `ServerConfig::port_`
- **Параметры логирования**
    - `LogManager::initialize(enable_logging, component_name)`

### Клиентская конфигурация

- **Адрес и порт сервера**
    - `ClientConfig::host`, `ClientConfig::port`
- **Количество параллельных соединений**
    - `ClientConfig::connections`
- **Зерно для генератора случайных чисел**
    - `ClientConfig::seed` → `std::mt19937 rng_(seed)`

## Технические требования

- **Стандарт C++**: C++20
    - `set(CMAKE_CXX_STANDARD 20)` в корневом CMakeLists.txt
- **Системные библиотеки**: pthread, epoll
    - `target_link_libraries(gen-app PRIVATE pthread)` в gen-app/CMakeLists.txt
    - `#include <sys/epoll.h>` в EpollManager.cpp
- **Сборка**: CMake
    - Корневой CMakeLists.txt + подпроекты
- **Платформа**: Linux
    - Использование Linux-специфичных API (epoll, socket)

## Структура файлов

````
gen-app/
├── main.cpp              # Главный файл приложения
├── client/
│   ├── TCPClient.h/cpp   # TCP клиент
│   ├── ClientConfig.h    # Конфигурация клиента
│   └── CMakeLists.txt    # CMake для клиентской библиотеки
├── server/
│   ├── TCPServer.h/cpp   # TCP сервер
│   ├── ServerConfig.h/cpp # Конфигурация сервера
│   └── CMakeLists.txt    # CMake для серверной библиотеки
├── network/
│   ├── EpollManager.h/cpp # Управление epoll
│   ├── SocketManager.h/cpp # Управление сокетами
│   └── CMakeLists.txt    # CMake для сетевой библиотеки
├── logging/
│   ├── Logger.h/cpp      # Базовый логгер
│   ├── LogManager.h/cpp  # Менеджер логирования
│   ├── LogMacros.h       # Макросы логирования
│   └── CMakeLists.txt    # CMake для библиотеки логирования
└── CMakeLists.txt        # CMake для gen-app (наследует от корневого)
````

**Примечание:** Проект собирается из корневой директории. Корневой `CMakeLists.txt` определяет общие настройки (C++20,
флаги компиляции), а gen-app, sniffer и tests собираются как подпроекты.

## Сборка и запуск

````bash
# Сборка из корневой директории проекта
rm -rf build && mkdir build && cd build
cmake ..
make
````

````bash
# Запуск сервера
./bin/gen-app --addr localhost:8000 --mode server
````

````bash
# Запуск клиента
./bin/gen-app --addr localhost:8000 --mode client --connections 512 --seed 1337
````