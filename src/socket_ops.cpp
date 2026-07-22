// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/socket_ops.h"

#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/uio.h>  // readv
#include <unistd.h>

#include <cerrno>
#include <cstdio>  // snprintf

#include "benet/logger.h"

namespace benet::sockets {

int create_nonblocking_or_die(unsigned short /* sa_family_t */ family) {
  int socktype = SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC;
  int sockprotocol = IPPROTO_TCP;
  int sockfd = ::socket(family, socktype, sockprotocol);
  if (sockfd < 0) {
    BELOG_CRITICAL("Failed to create sockfd: {}", ERRNO_MSG);
  }
  return sockfd;
}

void bind_or_die(int sockfd, const sockaddr* addr) {
  int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(sockaddr)));
  if (ret < 0) {
    BELOG_CRITICAL("Failed to bind sockfd {}, errno {}: {}", sockfd, errno,
                   ERRNO_MSG);
  }
}

void listen_or_die(int sockfd) {
  int ret = ::listen(sockfd, SOMAXCONN);
  if (ret < 0) {
    BELOG_CRITICAL("Failed to listen sockfd {}, errno {}: {}", sockfd, errno,
                   ERRNO_MSG);
  }
}

int connect(int sockfd, const sockaddr* addr) {
  return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(sockaddr)));
}

int accept(int sockfd, sockaddr_storage* addr) {
  socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));
  auto addrptr = reinterpret_cast<sockaddr*>(addr);

  static int accept_flags = SOCK_NONBLOCK | SOCK_CLOEXEC;
  int connfd = ::accept4(sockfd, addrptr, &addrlen, accept_flags);

  if (connfd < 0) {
    BELOG_ERROR("Failed to accept sockfd {}, errno {}: {}", sockfd, errno,
                ERRNO_MSG);
    switch (errno) {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO:
      case EPERM:
      case EMFILE:
        break;
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        BELOG_CRITICAL("Critical errors in accept sockfd {}, errno {}: {}",
                       sockfd, errno, ERRNO_MSG);
        break;
      default:
        BELOG_CRITICAL("Unknown errors in accept sockfd {}, errno {}: {}",
                       sockfd, errno, ERRNO_MSG);
        break;
    }
  }
  return connfd;
}

ssize_t read(int sockfd, void* buf, size_t count) {
  return ::read(sockfd, buf, count);
}

ssize_t readv(int sockfd, const iovec* iov, int iovcnt) {
  return ::readv(sockfd, iov, iovcnt);
}

ssize_t write(int sockfd, const void* buf, size_t count) {
  return ::write(sockfd, buf, count);
}

ssize_t send(int sockfd, const void* buf, size_t count) {
  return ::send(sockfd, buf, count, MSG_NOSIGNAL);
}

void close(int sockfd) {
  if (::close(sockfd) < 0) {
    BELOG_ERROR("Failed to close sockfd {}, errno {}: {}", sockfd, errno,
                ERRNO_MSG);
  }
}

void shutdown_write(int sockfd) {
  if (::shutdown(sockfd, SHUT_WR) < 0) {
    BELOG_ERROR("Failed to shutdown write sockfd {}, errno {}: {}", sockfd,
                errno, ERRNO_MSG);
  }
}

int get_socket_error(int sockfd) {
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof(optval));

  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  } else {
    return optval;
  }
}

sockaddr_storage get_local_addr(int sockfd) {
  sockaddr_storage localaddr;
  std::memset(&localaddr, 0, sizeof(localaddr));
  socklen_t addrlen = static_cast<socklen_t>(sizeof(localaddr));
  if (::getsockname(sockfd, reinterpret_cast<sockaddr*>(&localaddr), &addrlen) <
      0) {
    BELOG_ERROR("Failed to get local addr sockfd {}, errno {}: {}", sockfd,
                errno, ERRNO_MSG);
  }
  return localaddr;
}

sockaddr_storage get_peer_addr(int sockfd) {
  sockaddr_storage peeraddr;
  std::memset(&peeraddr, 0, sizeof(peeraddr));
  socklen_t addrlen = static_cast<socklen_t>(sizeof(peeraddr));
  if (::getpeername(sockfd, reinterpret_cast<sockaddr*>(&peeraddr), &addrlen) <
      0) {
    BELOG_ERROR("Failed to get peer addr sockfd {}, errno {}: {}", sockfd,
                errno, ERRNO_MSG);
  }
  return peeraddr;
}

bool is_self_connect(int sockfd) {
  sockaddr_storage localaddr = get_local_addr(sockfd);
  sockaddr_storage peeraddr = get_peer_addr(sockfd);
  sa_family_t family = reinterpret_cast<sockaddr*>(&localaddr)->sa_family;
  if (family == AF_INET) {
    const sockaddr_in* laddr4 =
        reinterpret_cast<const sockaddr_in*>(&localaddr);
    const sockaddr_in* raddr4 = reinterpret_cast<const sockaddr_in*>(&peeraddr);
    return laddr4->sin_port == raddr4->sin_port &&
           laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
  } else if (family == AF_INET6) {
    const sockaddr_in6* laddr6 =
        reinterpret_cast<const sockaddr_in6*>(&localaddr);
    const sockaddr_in6* raddr6 =
        reinterpret_cast<const sockaddr_in6*>(&peeraddr);
    return laddr6->sin6_port == raddr6->sin6_port &&
           std::memcmp(&laddr6->sin6_addr, &raddr6->sin6_addr,
                       sizeof(laddr6->sin6_addr)) == 0;
  } else {
    return false;
  }
}

}  // namespace benet::sockets
