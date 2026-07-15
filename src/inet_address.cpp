// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/inet_address.h"

#include <arpa/inet.h>

#include <cstring>
#include <format>

#include "benet/logger.h"

namespace benet {

InetAddress::InetAddress(uint16_t port, const std::string& ip, bool ipv6) {
  std::memset(&addr_, 0, sizeof(addr_));
  if (ipv6) {
    auto* addr6_ptr = reinterpret_cast<sockaddr_in6*>(&addr_);
    addr6_ptr->sin6_family = AF_INET6;
    addr6_ptr->sin6_port = ::htons(port);
    if (::inet_pton(AF_INET6, ip.data(), &addr6_ptr->sin6_addr) > 0) {
      return;
    }
  }
  auto* addr4_ptr = reinterpret_cast<sockaddr_in*>(&addr_);
  addr4_ptr->sin_family = AF_INET;
  addr4_ptr->sin_port = ::htons(port);
  if (::inet_pton(AF_INET, ip.data(), &addr4_ptr->sin_addr) <= 0) {
    BELOG_CRITICAL("Invalid IP Address: '{}'", ip);
  }
}

std::string InetAddress::ip() const {
  char ip_buf[INET6_ADDRSTRLEN] = {0};

  if (family() == AF_INET) {
    const auto* addr4_ptr = reinterpret_cast<const sockaddr_in*>(&addr_);
    ::inet_ntop(AF_INET, &(addr4_ptr->sin_addr), ip_buf, INET_ADDRSTRLEN);
  } else if (family() == AF_INET6) {
    const auto* addr6_ptr = reinterpret_cast<const sockaddr_in6*>(&addr_);
    ::inet_ntop(AF_INET6, &(addr6_ptr->sin6_addr), ip_buf, INET6_ADDRSTRLEN);
  }

  return std::string(ip_buf);
}

uint16_t InetAddress::port() const {
  uint16_t port = 0;

  if (family() == AF_INET) {
    const auto* addr4_ptr = reinterpret_cast<const sockaddr_in*>(&addr_);
    port = ::ntohs(addr4_ptr->sin_port);
  } else if (family() == AF_INET6) {
    const auto* addr6_ptr = reinterpret_cast<const sockaddr_in6*>(&addr_);
    port = ::ntohs(addr6_ptr->sin6_port);
  }

  return port;
}

std::string InetAddress::AsString() const {
  return std::format("{}:{}", ip(), port());
}

sa_family_t InetAddress::family() const { return addr_.ss_family; }

sockaddr* InetAddress::addr() { return reinterpret_cast<sockaddr*>(&addr_); }

const sockaddr* InetAddress::addr() const {
  return reinterpret_cast<const sockaddr*>(&addr_);
}

void InetAddress::set_addr(const sockaddr_storage& addr) {
  std::memcpy(&addr_, &addr, sizeof(addr_));
}

}  // namespace benet
