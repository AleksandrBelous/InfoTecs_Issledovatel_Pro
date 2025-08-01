#include "ServerConfig.h"

ServerConfig::ServerConfig(std::string host, uint16_t port)
    : host_(std::move(host)), port_(port)
{
}

bool ServerConfig::isValid() const noexcept
{
    return !host_.empty() && port_ > 0;
}
