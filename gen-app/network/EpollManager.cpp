#include "EpollManager.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>

EpollManager::EpollManager() : epoll_fd_(-1)
{
}

EpollManager::~EpollManager()
{
    if(epoll_fd_ >= 0)
    {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }
}

EpollManager::EpollManager(EpollManager&& other) noexcept
    : epoll_fd_(other.epoll_fd_)
{
    other.epoll_fd_ = -1;
}

EpollManager& EpollManager::operator=(EpollManager&& other) noexcept
{
    if(this != &other)
    {
        if(epoll_fd_ >= 0)
        {
            close(epoll_fd_);
        }
        epoll_fd_ = other.epoll_fd_;
        other.epoll_fd_ = -1;
    }
    return *this;
}

bool EpollManager::initialize()
{
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if(epoll_fd_ == -1)
    {
        std::perror("epoll_create1");
        return false;
    }
    return true;
}

bool EpollManager::addFileDescriptor(const int fd, const uint32_t events) const
{
    struct epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        std::perror("epoll_ctl(EPOLL_CTL_ADD)");
        return false;
    }

    return true;
}

bool EpollManager::removeFileDescriptor(const int fd) const
{
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == -1)
    {
        std::perror("epoll_ctl(EPOLL_CTL_DEL)");
        return false;
    }
    return true;
}

int EpollManager::waitForEvents(struct epoll_event* events, const int max_events, const int timeout) const
{
    return epoll_wait(epoll_fd_, events, max_events, timeout);
}
