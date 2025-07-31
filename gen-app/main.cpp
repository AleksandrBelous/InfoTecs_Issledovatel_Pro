// gen_app.cpp — universal client/server executable
// (Step 1) Headers + утилита setNonBlocking
// ------------------------------------------------------------
// На данном шаге мы только подключаем необходимые системные
// заголовки и добавляем функцию, переводящую файловый дескриптор
// в неблокирующий режим. Логика сервера появится позже.

#include <sys/epoll.h>   // epoll_create1, epoll_ctl, epoll_wait — ядровой механизм I/O
#include <sys/socket.h>  // socket, bind, listen, accept, recv   — базовые операции с сокетами
#include <arpa/inet.h>   // htons, inet_pton                     — работа с IP‑адресами
#include <fcntl.h>       // fcntl, O_NONBLOCK                    — управление флагами дескрипторов
#include <unistd.h>      // close                                — закрытие дескрипторов
#include <csignal>       // signal                               — обработка SIGINT
#include <cstdlib>       // exit, EXIT_FAILURE                   — корректный выход
#include <cstring>       // strerror                             — строки ошибок
#include <iostream>      // std::cout, std::cerr                 — вывод сообщений
#include <ranges>

// ── 1. Утилита перевода дескриптора в неблокирующий режим ─────────────────────
// Возвращает true при успехе, false при ошибке.
bool setNonBlocking(const int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1)
    {
        std::perror("fcntl(F_GETFL)");
        return false;
    }
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::perror("fcntl(F_SETFL)");
        return false;
    }
    std::cout << "flags=" << flags << std::endl;
    return true;
}

// ──────────────────── Глобальный флаг для SIGINT ─────────────────────────────
volatile sig_atomic_t running = 1;

void handleSigint(int /*signum*/)
{
    running = 0; // Уведомим главный цикл выйти
}

// ── 2. Структура для параметров командной строки ────────────────────────────
struct CmdOptions
{
    std::string host = "0.0.0.0"; // по умолчанию — любой интерфейс
    uint16_t port = 0; // обязателен
    std::string mode; // "server" или "client"
};

// ── 3. Простейший разбор argv ───────────────────────────────────────────────
bool parseCmdLine(int argc, char* argv[], CmdOptions& opt)
{
    for(int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if(arg == "--addr" && i + 1 < argc)
        {
            std::string addr = argv[++i];
            auto colon = addr.find(':');
            if(colon == std::string::npos)
            {
                std::cerr << "[error] --addr требует host:port\n";
                return false;
            }
            opt.host = addr.substr(0, colon);
            int port = std::stoi(addr.substr(colon + 1));
            if(port <= 0 || port > 65535)
            {
                std::cerr << "[error] некорректный порт: " << port << "\n";
                return false;
            }
            opt.port = static_cast<uint16_t>(port);
        }
        else if(arg == "--mode" && i + 1 < argc)
        {
            opt.mode = argv[++i];
        }
        else
        {
            std::cerr << "[error] неизвестный или неполный аргумент: " << arg << "\n";
            return false;
        }
    }

    if(opt.mode.empty())
    {
        std::cerr << "[error] нужно указать --mode server|client\n";
        return false;
    }
    if(opt.port == 0)
    {
        std::cerr << "[error] нужно указать --addr host:port\n";
        return false;
    }
    return true;
}

// ── 4. Функция запуска сервера (без epoll‑цикла пока) ───────────────────────
int runServer(const CmdOptions& opt)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1)
    {
        std::perror("socket");
        return EXIT_FAILURE;
    }

    // Переводим в неблокирующий режим
    if(!setNonBlocking(listen_fd))
    {
        close(listen_fd);
        return EXIT_FAILURE;
    }

    // Позволяет быстро перезапускать сервер на том же порту
    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(opt.port);
    if(inet_pton(AF_INET, opt.host.c_str(), &addr.sin_addr) != 1)
    {
        std::cerr << "[error] некорректный адрес: " << opt.host << "\n";
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if(bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
    {
        std::perror("bind");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if(listen(listen_fd, SOMAXCONN) == -1)
    {
        std::perror("listen");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    std::cout << "[server] Listening on " << opt.host << ':' << opt.port << " (non‑blocking)\n";

    // 2) Настраиваем epoll
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if(epoll_fd == -1)
    {
        std::perror("epoll_create1");
        close(listen_fd);
        return EXIT_FAILURE;
    }
    epoll_event ev{};
    ev.events = EPOLLIN; // Level‑triggered read events
    ev.data.fd = listen_fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
    {
        std::perror("epoll_ctl ADD listen_fd");
        close(epoll_fd);
        close(listen_fd);
        return EXIT_FAILURE;
    }

    // 3) Обработчик Ctrl‑C
    signal(SIGINT, handleSigint);

    constexpr int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];

    // 4) Главный цикл
    while(running)
    {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if(n == -1)
        {
            if(errno == EINTR) continue; // прерваны сигналом
            std::perror("epoll_wait");
            break;
        }
        for(int i = 0; i < n; ++i)
        {
            int fd = events[i].data.fd;
            if(fd == listen_fd)
            {
                // Новые подключения
                while(true)
                {
                    sockaddr_in cliAddr{};
                    socklen_t len = sizeof(cliAddr);
                    int client_fd = accept(listen_fd, reinterpret_cast<sockaddr*>(&cliAddr), &len);
                    if(client_fd == -1)
                    {
                        if(errno == EAGAIN || errno == EWOULDBLOCK) break; // больше нет
                        std::perror("accept");
                        break;
                    }
                    if(!setNonBlocking(client_fd))
                    {
                        close(client_fd);
                        continue;
                    }
                    epoll_event cev{};
                    cev.events = EPOLLIN | EPOLLRDHUP; // читаем + обнаруживаем закрытие peer‑ом
                    cev.data.fd = client_fd;
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &cev) == -1)
                    {
                        std::perror("epoll_ctl ADD client_fd");
                        close(client_fd);
                        continue;
                    }
                }
            }
            else
            {
                // Событие от клиента
                bool closeConn = false;
                char buf[4096];
                while(true)
                {
                    ssize_t r = recv(fd, buf, sizeof(buf), 0);
                    if(r > 0)
                    {
                        // Данные получены — просто отбрасываем (задача не требует обработки)
                        continue;
                    }
                    else if(r == 0)
                    {
                        // EOF — клиент закрыл соединение
                        closeConn = true;
                        break;
                    }
                    else
                    {
                        // r == -1
                        if(errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            break; // прочитали всё
                        }
                        std::perror("recv");
                        closeConn = true;
                        break;
                    }
                }
                if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                {
                    closeConn = true;
                }
                if(closeConn)
                {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                }
            }
        }
    }

    std::cout << "\n[server] Shutting down…\n";
    close(epoll_fd);
    close(listen_fd);
    return EXIT_SUCCESS;
}

// ── 5. Точка входа ──────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    CmdOptions opts;
    if(!parseCmdLine(argc, argv, opts))
    {
        return EXIT_FAILURE;
    }

    if(opts.mode == "server")
    {
        return runServer(opts);
    }
    std::cerr << "[error] client mode ещё не реализован\n";
    return EXIT_FAILURE;
}
