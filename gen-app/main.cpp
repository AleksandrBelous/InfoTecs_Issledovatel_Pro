// ── 1. Заголовочные файлы POSIX ─────────────────────────────────────────────
#include <sys/epoll.h>   // ::epoll_create1, ::epoll_ctl, ::epoll_wait      – ядровой механизм мультиплексирования I/O
#include <sys/socket.h>  // ::socket, ::bind, ::listen, ::accept, ::recv    – базовые операции с сокетами
#include <arpa/inet.h>   // ::htons, ::inet_pton, структура sockaddr_in     – работа с IP-адресами
#include <fcntl.h>       // ::fcntl, константа O_NONBLOCK                   – изменение флагов файловых дескрипторов
#include <unistd.h>      // ::close                                         – закрытие файловых дескрипторов
#include <iostream>      // std::cerr/std::cout – вывод сообщений.

// ── 2. Функция для переключения дескриптора в неблокирующий режим. ───
// Если сокет неблокирующий, операции чтения/записи мгновенно возвращают EAGAIN,
// вместо того чтобы «зависнуть», если данных пока нет.
int set_nonblock(const int fd)
{
    // fcntl(fd, F_GETFL) – получаем текущие флаги (например, O_RDWR).
    int flags = fcntl(fd, F_GETFL, 0);
    // Добавляем к ним бит O_NONBLOCK.
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// ── 3. Точка входа программы. ───────────────────────────────────────────────
int main()
{
    // -------- Шаг 1. Создание «listening»‑сокета (серверного). ----------
    const int server_socket = socket(AF_INET /* домен IPv4 */,
                                     SOCK_STREAM /* TCP */,
                                     0); /* TCP по умолчанию */
    if (server_socket < 0)
    {
        perror("[-] Server socket");
        return 1;
    }

    // setsockopt(…, SO_REUSEADDR) позволяет сразу перезапускать программу,
    //  не ожидая, пока ядро освободит порт после предыдущего запуска.
    int yes = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    // Переводим listening‑сокет в неблокирующий режим.
    set_nonblock(server_socket);

    // Заполняем структуру sockaddr_in для адреса 0.0.0.0:80.
    sockaddr_in addr{}; // Инициализируем нулями (C++20).
    addr.sin_family = AF_INET; // IPv4‑семейство.
    addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0 → «слушать на всех интерфейсах».
    addr.sin_port = htons(80); // htons – «host‑to‑network short»: переводим 80 в сетевой порядок байт.

    // Привязываем сокет к адресу/порту.
    bind(server_socket, reinterpret_cast<sockaddr*>(&addr), sizeof addr);

    // Помещаем сокет в состояние «слушать» (queue= SOMAXCONN – максимум для ОС).
    listen(server_socket, SOMAXCONN);

    // -------- Шаг 2. Подготавливаем epoll для отслеживания событий. ------
    const int epoll = epoll_create1(0); // 0 → без дополнительных флагов.

    // epoll_event – структура для описания того, за какими событиями следим.
    epoll_event ep_event{}; // Инициализируем нулями.
    ep_event.events = EPOLLIN; // EPOLLIN → «дескриптор готов к чтению».
    ep_event.data.fd = server_socket; // Кладём в поле data наш listening‑fd.

    // Регистрируем ls в epoll: ADD = добавить, &ev = с какими событиями.
    epoll_ctl(epoll, EPOLL_CTL_ADD, server_socket, &ep_event);

    // Массив для получаемых событий и буфер для чтения данных.
    epoll_event evs[64]; // За раз обрабатываем до 64 событий.
    char buf[4096]; // В этот буфер читаем входящие данные.

    // -------- Шаг 3. Главный цикл обработки событий. --------------------
    while (true)
    {
        // epoll_wait блокируется, пока не будет событий (‑1 = ждать бесконечно).
        int n = epoll_wait(epoll, evs, 64, -1);
        if (n < 0)
        {
            perror("epoll_wait");
            break;
        }

        // Перебираем полученные события.
        for (int i = 0; i < n; ++i)
        {
            int fd = evs[i].data.fd; // Какой дескриптор «зафайрился».

            // ---- 3.1. Новые входящие соединения. ----
            if (fd == server_socket)
            {
                // accept может принять несколько клиентов подряд.
                while (true)
                {
                    int c = accept(server_socket, nullptr, nullptr); // Клиентский fd.
                    if (c < 0) break; // -1 и errno==EAGAIN → очередь пуста.

                    set_nonblock(c); // Делаем клиентский сокет неблокирующим.

                    // Регистрируем его в epoll, чтобы отслеживать чтение и событие закрытия (EPOLLRDHUP).
                    epoll_event c_ev{};
                    c_ev.events = EPOLLIN | EPOLLRDHUP;
                    c_ev.data.fd = c;
                    epoll_ctl(epoll, EPOLL_CTL_ADD, c, &c_ev);
                }
            }
            // ---- 3.2. События на уже существующем клиентском сокете. ----
            else
            {
                // Читаем, пока есть данные.
                while (recv(fd, buf, sizeof buf, 0) > 0)
                {
                    /* Данные нам не нужны → отбрасываем. */
                }
                // Удаляем дескриптор из epoll и закрываем.
                epoll_ctl(epoll, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
            }
        }
    }

    // Если главный цикл оборвался – закрываем ресурсы (на практике сюда не доходим).
    close(epoll);
    close(server_socket);
    return 0;
}
