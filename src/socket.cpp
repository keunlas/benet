// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/socket.h"

#include "benet/logger.h"
#include "benet/socket_ops.h"

namespace benet {

Socket::~Socket() { sockets::close(sockfd_); }

void Socket::Bind(const InetAddress& local_addr) {
  sockets::bind_or_die(sockfd_, local_addr.addr());
}

void Socket::Listen() { sockets::listen_or_die(sockfd_); }

int Socket::Accept(InetAddress* peer_addr) {
  sockaddr_storage addr;
  std::memset(&addr, 0, sizeof(addr));
  int connfd = sockets::accept(sockfd_, &addr);
  if (connfd >= 0) {
    peer_addr->set_addr(addr);
  }
  return connfd;
}

void Socket::ShutdownWrite() { sockets::shutdown_write(sockfd_); }

void Socket::SetTcpNodelay(bool on) {
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval,
                         static_cast<socklen_t>(sizeof(optval)));
  if (ret < 0 && on) {
    BELOG_ERROR("Failed to set sockfd {} TCP_NODELAY, errno {}: {}", sockfd_,
                errno, ERRNO_MSG);
  }
}

void Socket::SetReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval,
                         static_cast<socklen_t>(sizeof(optval)));
  if (ret < 0 && on) {
    BELOG_ERROR("Failed to set sockfd {} SO_REUSEADDR, errno {}: {}", sockfd_,
                errno, ERRNO_MSG);
  }
}

void Socket::SetReusePort(bool on) {
#ifdef SO_REUSEPORT
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval,
                         static_cast<socklen_t>(sizeof(optval)));
  if (ret < 0 && on) {
    BELOG_ERROR("Failed to set sockfd {} SO_REUSEPORT, errno {}: {}", sockfd_,
                errno, ERRNO_MSG);
  }
#else
  if (on) {
    BELOG_ERROR("System not support SO_REUSEPORT");
  }
#endif
}

void Socket::SetKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval,
                         static_cast<socklen_t>(sizeof(optval)));
  if (ret < 0 && on) {
    BELOG_ERROR("Failed to set sockfd {} SO_KEEPALIVE, errno {}: {}", sockfd_,
                errno, ERRNO_MSG);
  }
}

}  // namespace benet
