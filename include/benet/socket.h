// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_SOCKET_H_
#define KEUNLAS_BENET_SOCKET_H_

#include "benet/copy_move_policy.h"
#include "benet/inet_address.h"

namespace benet {

  /**
   * @brief 底层 Socket 封装，支持 TCP。
   * 
   */
class Socket : NotCopyableOrMovable {
 public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}
  ~Socket();

  /// @brief 内部文件描述符
  int fd() const { return sockfd_; }

  /// @brief 绑定地址
  void Bind(const InetAddress& local_addr);

  /// @brief 监听已经绑定的地址
  void Listen();

  /// @brief 接受并填写对端地址
  int Accept(InetAddress* peer_addr);

  /// @brief 关闭写
  void ShutdownWrite();

  /// @brief 设置 TCP 无延迟传输
  void SetTcpNodelay(bool on);

  /// @brief 设置地址复用
  void SetReuseAddr(bool on);

  /// @brief 设置端口复用
  void SetReusePort(bool on);

  /// @brief 设置 TCP Keep-Alive 机制
  void SetKeepAlive(bool on);

 private:
  const int sockfd_;
};

}  // namespace benet

#endif  // !KEUNLAS_BENET_SOCKET_H_
