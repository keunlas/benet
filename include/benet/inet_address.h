// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_INETADDRESS_H_
#define KEUNLAS_BENET_INETADDRESS_H_

#include <netinet/in.h>

#include <cstdint>
#include <string>

#include "benet/copy_move_policy.h"

namespace benet {

/**
 * @brief 底层 IP 地址封装，支持 ipv4 和 ipv6。
 *
 */
class InetAddress : Copyable {
 public:
  InetAddress() {}
  InetAddress(uint16_t port) : InetAddress(port, "0.0.0.0") {}
  InetAddress(uint16_t port, const std::string& ip, bool ipv6 = false);
  explicit InetAddress(const sockaddr_storage& addr) : addr_(addr) {}

  /// @brief 地址族
  unsigned short /* sa_family_t */ family() const;

  /// @brief 通用 socket 地址
  sockaddr* addr();

  /// @brief 通用 socket 地址
  const sockaddr* addr() const;

  /// @brief 设置通用 socket 地址
  void set_addr(const sockaddr_storage& addr);

  /// @brief 获取 ip 的字符串形式
  /// eg. "127.0.0.1"
  std::string ip() const;

  /// @brief 获取 port 的数字形式
  /// eg. 8080
  uint16_t port() const;

  /// @brief 获取字符串形式的完整地址
  /// eg. "127.0.0.1:8080"
  std::string AsString() const;

 private:
  sockaddr_storage addr_;  // compatible to ipv4 and ipv6.
};

}  // namespace benet

#endif  // !KEUNLAS_BENET_INETADDRESS_H_
