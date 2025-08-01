#include "SocketManager.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

int SocketManager::createServerSocket(const std::string& host, uint16_t port)
{
    // Создаем TCP сокет
    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1)
    {
        std::perror("socket");
        return -1;
    }

    // Переводим в неблокирующий режим
    if(!setNonBlocking(server_fd))
    {
        closeSocket(server_fd);
        return -1;
    }

    // Устанавливаем опцию SO_REUSEADDR
    if(!setReuseAddr(server_fd))
    {
        closeSocket(server_fd);
        return -1;
    }

    // Настраиваем адрес сервера
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) != 1)
    {
        std::cerr << "[error] Некорректный IP-адрес: " << host << "\n";
        closeSocket(server_fd);
        return -1;
    }

    // Привязываем сокет к адресу
    if(bind(server_fd, reinterpret_cast<const struct sockaddr*>(&server_addr), sizeof(server_addr)) == -1)
    {
        std::perror("bind");
        closeSocket(server_fd);
        return -1;
    }

    // Начинаем слушать подключения
    if(listen(server_fd, SOMAXCONN) == -1)
    {
        std::perror("listen");
        closeSocket(server_fd);
        return -1;
    }

    std::cout << "[server] Сервер запущен на " << host << ":" << port << "\n";
    return server_fd;
}

int SocketManager::createClientSocket(const std::string& host, uint16_t port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        std::perror("socket");
        return -1;
    }

    if(!setNonBlocking(fd))
    {
        closeSocket(fd);
        return -1;
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if(inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
    {
        std::cerr << "[error] Некорректный IP-адрес: " << host << "\n";
        closeSocket(fd);
        return -1;
    }

    if(connect(fd, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) == -1)
    {
        if(errno != EINPROGRESS)
        {
            std::perror("connect");
            closeSocket(fd);
            return -1;
        }
    }

    return fd;
}

bool SocketManager::setNonBlocking(int fd)
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

    return true;
}

void SocketManager::closeSocket(int fd)
{
    if(fd >= 0)
    {
        close(fd);
    }
}

int SocketManager::acceptConnection(int server_fd, struct sockaddr_in* client_addr, socklen_t* addr_len)
{
    return accept(server_fd, reinterpret_cast<struct sockaddr*>(client_addr), addr_len);
}

ssize_t SocketManager::sendData(int fd, const char* buffer, size_t buffer_size)
{
    return send(fd, buffer, buffer_size, 0);
}

ssize_t SocketManager::receiveData(int fd, char* buffer, size_t buffer_size)
{
    return recv(fd, buffer, buffer_size, 0);
}

std::string SocketManager::getClientIP(const struct sockaddr_in& addr)
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    return std::string{ip};
}

uint16_t SocketManager::getClientPort(const struct sockaddr_in& addr)
{
    return ntohs(addr.sin_port);
}

bool SocketManager::setReuseAddr(int fd)
{
    int yes = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        std::perror("setsockopt(SO_REUSEADDR)");
        return false;
    }
    return true;
}
