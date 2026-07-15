// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_INETADDRESS_H_
#define BENET_INETADDRESS_H_

#include <netinet/in.h>

#include <cstdint>
#include <cstring>
#include <string>

#include "benet/copy_move_type.h"

namespace benet {

class InetAddress : Copyable {
 public:
  InetAddress() : InetAddress(8080, "0.0.0.0") {}
  InetAddress(uint16_t port) : InetAddress(port, "0.0.0.0") {}
  InetAddress(uint16_t port, const std::string& ip, bool ipv6 = false);
  explicit InetAddress(const sockaddr_storage& addr) : addr_(addr) {}

  sa_family_t family() const;

  sockaddr* addr();
  const sockaddr* addr() const;
  void set_addr(const sockaddr_storage& addr);

  std::string ip() const;  // eg. "127.0.0.1"
  uint16_t port() const;   // eg. "8080"

  std::string AsString() const;  // eg. "127.0.0.1:8080"

 private:
  sockaddr_storage addr_;  // compatible to ipv4 and ipv6.
};

}  // namespace benet

#endif  // !BENET_INETADDRESS_H_
