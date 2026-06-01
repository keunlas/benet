// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_SOCKET_H_
#define BENET_SOCKET_H_

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "benet/copy_move_type.h"
#include "benet/inet_address.h"

namespace benet {

class Socket : NotCopyableOrMovable {
 public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}
  ~Socket();

  int fd() const { return sockfd_; }

  void Bind(const InetAddress& local_addr);
  void Listen();
  int Accept(InetAddress* peer_addr);

  void ShutdownWrite();

  void SetTcpNodelay(bool on);
  void SetReuseAddr(bool on);
  void SetReusePort(bool on);
  void SetKeepAlive(bool on);

 private:
  const int sockfd_;
};

}  // namespace benet

#endif  // !BENET_SOCKET_H_
