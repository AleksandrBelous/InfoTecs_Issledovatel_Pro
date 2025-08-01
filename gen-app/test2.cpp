// test2.cpp — серверная часть TCP/IPv4 приложения
// Реализация в соответствии с заданием из Task.md, Часть 1

#include <sys/epoll.h>      // epoll_create1, epoll_ctl, epoll_wait
#include <sys/socket.h>     // socket, bind, listen, accept, recv
#include <arpa/inet.h>      // htons, inet_pton, inet_ntop
#include <fcntl.h>          // fcntl, O_NONBLOCK
#include <unistd.h>         // close
#include <csignal>          // signal, SIGINT
#include <cstdlib>          // exit, EXIT_FAILURE, EXIT_SUCCESS
#include <cstring>          // strerror, memset
#include <iostream>         // std::cout, std::cerr
#include <string>           // std::string
#include <vector>           // std::vector
#include <set>              // std::set
#include <errno.h>          // errno, EAGAIN, EWOULDBLOCK, EINTR

// Глобальный флаг для корректного завершения по Ctrl-C
volatile sig_atomic_t g_running = 1;

// Структура для хранения параметров командной строки
struct ServerConfig {
    std::string host = "0.0.0.0";  // По умолчанию слушаем на всех интерфейсах
    uint16_t port = 0;             // Порт (обязательный параметр)
};

// Обработчик сигнала SIGINT (Ctrl-C)
void signalHandler(int signum) {
    (void)signum; // Подавляем предупреждение о неиспользуемом параметре
    g_running = 0;
    std::cout << "\n[server] Получен сигнал завершения, закрываю соединения...\n";
}

// Функция для перевода файлового дескриптора в неблокирующий режим
bool setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::perror("fcntl(F_GETFL)");
        return false;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::perror("fcntl(F_SETFL)");
        return false;
    }
    
    return true;
}

// Функция для разбора адреса в формате host:port
bool parseAddress(const std::string& addr, ServerConfig& config) {
    size_t colon_pos = addr.find(':');
    if (colon_pos == std::string::npos) {
        std::cerr << "[error] Неверный формат адреса. Используйте host:port\n";
        return false;
    }
    
    std::string host = addr.substr(0, colon_pos);
    std::string port_str = addr.substr(colon_pos + 1);
    
    // Преобразуем localhost в 127.0.0.1
    if (host == "localhost") {
        host = "127.0.0.1";
    }
    
    config.host = host;
    
    try {
        int port = std::stoi(port_str);
        if (port <= 0 || port > 65535) {
            std::cerr << "[error] Некорректный порт: " << port << "\n";
            return false;
        }
        config.port = static_cast<uint16_t>(port);
    } catch (const std::exception& e) {
        std::cerr << "[error] Некорректный порт: " << port_str << "\n";
        return false;
    }
    
    return true;
}

// Функция для разбора аргументов командной строки
bool parseCommandLine(int argc, char* argv[], ServerConfig& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--addr" && i + 1 < argc) {
            if (!parseAddress(argv[++i], config)) {
                return false;
            }
        } else if (arg == "--mode" && i + 1 < argc) {
            std::string mode = argv[++i];
            if (mode != "server") {
                std::cerr << "[error] Поддерживается только режим server\n";
                return false;
            }
        } else {
            std::cerr << "[error] Неизвестный аргумент: " << arg << "\n";
            return false;
        }
    }
    
    if (config.port == 0) {
        std::cerr << "[error] Необходимо указать адрес и порт: --addr host:port\n";
        return false;
    }
    
    return true;
}

// Функция для создания и настройки серверного сокета
int createServerSocket(const ServerConfig& config) {
    // Создаем TCP сокет
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::perror("socket");
        return -1;
    }
    
    // Переводим в неблокирующий режим
    if (!setNonBlocking(server_fd)) {
        close(server_fd);
        return -1;
    }
    
    // Устанавливаем опцию SO_REUSEADDR для быстрого перезапуска
    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        std::perror("setsockopt(SO_REUSEADDR)");
        close(server_fd);
        return -1;
    }
    
    // Настраиваем адрес сервера
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.port);
    
    if (inet_pton(AF_INET, config.host.c_str(), &server_addr.sin_addr) != 1) {
        std::cerr << "[error] Некорректный IP-адрес: " << config.host << "\n";
        close(server_fd);
        return -1;
    }
    
    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::perror("bind");
        close(server_fd);
        return -1;
    }
    
    // Начинаем слушать подключения
    if (listen(server_fd, SOMAXCONN) == -1) {
        std::perror("listen");
        close(server_fd);
        return -1;
    }
    
    std::cout << "[server] Сервер запущен на " << config.host << ":" << config.port << "\n";
    return server_fd;
}

// Функция для создания epoll-инстанса
int createEpollInstance() {
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        std::perror("epoll_create1");
        return -1;
    }
    return epoll_fd;
}

// Функция для добавления файлового дескриптора в epoll
bool addToEpoll(int epoll_fd, int fd, uint32_t events) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        std::perror("epoll_ctl(EPOLL_CTL_ADD)");
        return false;
    }
    
    return true;
}

// Функция для удаления файлового дескриптора из epoll
bool removeFromEpoll(int epoll_fd, int fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        std::perror("epoll_ctl(EPOLL_CTL_DEL)");
        return false;
    }
    return true;
}

// Функция для обработки нового клиентского подключения
void handleNewConnection(int server_fd, int epoll_fd, std::set<int>& client_fds) {
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        memset(&client_addr, 0, client_addr_len);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Больше нет ожидающих подключений
                break;
            }
            std::perror("accept");
            break;
        }
        
        // Переводим клиентский сокет в неблокирующий режим
        if (!setNonBlocking(client_fd)) {
            close(client_fd);
            continue;
        }
        
        // Добавляем клиентский сокет в epoll для чтения данных
        // EPOLLRDHUP - для обнаружения закрытия соединения клиентом
        if (!addToEpoll(epoll_fd, client_fd, EPOLLIN | EPOLLRDHUP)) {
            close(client_fd);
            continue;
        }
        
        client_fds.insert(client_fd);
        
        // Выводим информацию о новом подключении
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        std::cout << "[server] Новое подключение от " << client_ip 
                  << ":" << ntohs(client_addr.sin_port) 
                  << " (fd=" << client_fd << ")\n";
    }
}

// Функция для обработки данных от клиента
void handleClientData(int client_fd, int epoll_fd, std::set<int>& client_fds) {
    char buffer[4096];
    bool should_close = false;
    
    // Читаем все доступные данные от клиента
    while (true) {
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        
        if (bytes_received > 0) {
            // Данные получены - согласно заданию, просто читаем их
            // (не требуется обработка данных)
            continue;
        } else if (bytes_received == 0) {
            // Клиент закрыл соединение (EOF)
            should_close = true;
            break;
        } else {
            // bytes_received == -1
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Больше нет данных для чтения
                break;
            }
            std::perror("recv");
            should_close = true;
            break;
        }
    }
    
    // Проверяем, нужно ли закрыть соединение
    if (should_close) {
        client_fds.erase(client_fd);
        std::cout << "[server] Соединение закрыто (fd=" << client_fd << ")\n";
        removeFromEpoll(epoll_fd, client_fd);
        close(client_fd);
    }
}

// Функция для обработки событий epoll
void handleEpollEvents(int epoll_fd, int server_fd, std::set<int>& client_fds) {
    const int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];
    
    int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (num_events == -1) {
        if (errno == EINTR) {
            // Прервано сигналом - это нормально
            return;
        }
        std::perror("epoll_wait");
        return;
    }
    
    for (int i = 0; i < num_events; ++i) {
        int fd = events[i].data.fd;
        uint32_t event_flags = events[i].events;
        
        if (fd == server_fd) {
            // Событие от серверного сокета - новое подключение
            handleNewConnection(server_fd, epoll_fd, client_fds);
        } else {
            // Событие от клиентского сокета
            if (event_flags & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // Соединение закрыто или произошла ошибка
                client_fds.erase(fd);
                std::cout << "[server] Соединение закрыто клиентом (fd=" << fd << ")\n";
                removeFromEpoll(epoll_fd, fd);
                close(fd);
            } else if (event_flags & EPOLLIN) {
                // Есть данные для чтения
                handleClientData(fd, epoll_fd, client_fds);
            }
        }
    }
}

// Основная функция сервера
int runServer(const ServerConfig& config) {
    // Создаем серверный сокет
    int server_fd = createServerSocket(config);
    if (server_fd == -1) {
        return EXIT_FAILURE;
    }
    
    // Создаем epoll-инстанс
    int epoll_fd = createEpollInstance();
    if (epoll_fd == -1) {
        close(server_fd);
        return EXIT_FAILURE;
    }
    
    // Добавляем серверный сокет в epoll для ожидания новых подключений
    if (!addToEpoll(epoll_fd, server_fd, EPOLLIN)) {
        close(epoll_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }
    
    // Устанавливаем обработчик сигнала SIGINT
    signal(SIGINT, signalHandler);
    
    // Множество для хранения активных клиентских соединений
    std::set<int> client_fds;
    
    std::cout << "[server] Ожидаю подключения... (Ctrl-C для завершения)\n";
    
    // Главный цикл сервера
    while (g_running) {
        handleEpollEvents(epoll_fd, server_fd, client_fds);
    }
    
    // Корректное завершение работы
    std::cout << "[server] Завершение работы сервера...\n";
    
    // Закрываем все клиентские соединения
    for (int client_fd : client_fds) {
        std::cout << "[server] Закрываю клиентское соединение (fd=" << client_fd << ")\n";
        removeFromEpoll(epoll_fd, client_fd);
        close(client_fd);
    }
    client_fds.clear();
    
    // Закрываем epoll и серверный сокет
    close(epoll_fd);
    close(server_fd);
    
    std::cout << "[server] Сервер остановлен\n";
    return EXIT_SUCCESS;
}

// Точка входа в программу
int main(int argc, char* argv[]) {
    ServerConfig config;
    
    // Разбираем аргументы командной строки
    if (!parseCommandLine(argc, argv, config)) {
        std::cerr << "Использование: " << argv[0] << " --addr host:port --mode server\n";
        std::cerr << "Пример: " << argv[0] << " --addr localhost:8000 --mode server\n";
        return EXIT_FAILURE;
    }
    
    // Запускаем сервер
    return runServer(config);
} 