// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/Be
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/acceptor.h"

namespace benet {
namespace details {
int create_socketfd(sa_family_t family) {
  int sockfd =
      ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd < 0) {
    BELOG_CRITICAL("Failed to create socket fd, errno {}: {}", errno,
                   ERRNO_MSG);
  }
  return sockfd;
}
int create_idlefd() {
  int fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    BELOG_CRITICAL("Failed to create idle fd, errno {}: {}", errno, ERRNO_MSG);
  }
  return fd;
}
}  // namespace details

Acceptor::Acceptor(EventLoop* loop, const InetAddress& addr, bool reuseport)
    : loop_(loop),
      accept_socket_(details::create_socketfd(addr.family())),
      accept_channel_(loop_, accept_socket_.fd()),
      idle_fd_(details::create_idlefd()) {
  accept_socket_.SetReuseAddr(true);
  accept_socket_.SetReusePort(reuseport);
  accept_socket_.Bind(addr);
  accept_channel_.BindReadCallback(std::bind(&Acceptor::handle_new_conn, this));
}

Acceptor::~Acceptor() {
  accept_channel_.DisableAllEvent();
  accept_channel_.RemoveFromLoop();
  ::close(idle_fd_);
}

void Acceptor::Listen() {
  loop_->AssertInLoopThread();
  listening_ = true;
  accept_socket_.Listen();
  accept_channel_.EnableReadEvent();
}

void Acceptor::handle_new_conn() {
  loop_->AssertInLoopThread();
  InetAddress peer_addr;

  // [TODO] loop until no more conn.
  int connfd = accept_socket_.Accept(&peer_addr);
  if (connfd >= 0) {
    if (on_new_connection_) {
      // Do new connection callbacks, which setten in TcpServer.
      // Wrap this connfd into TcpConnection and set it's callbacks.
      on_new_connection_(connfd, peer_addr);
    } else {
      if (::close(connfd) < 0) {
        BELOG_ERROR("Failed to close connfd {}, errno {}: {}", connfd, errno,
                    ERRNO_MSG);
      }
    }
  } else {
    BELOG_ERROR("Failed to accept connfd, errno {}: {}", errno, ERRNO_MSG);

    if (errno == EMFILE) {
      ::close(idle_fd_);
      idle_fd_ = ::accept(accept_socket_.fd(), NULL, NULL);
      ::close(idle_fd_);
      idle_fd_ = details::create_idlefd();
    }
  }
}

void Acceptor::BindNewConnCallback(const NewConnCallback& cb) {
  on_new_connection_ = cb;
}

}  // namespace benet
