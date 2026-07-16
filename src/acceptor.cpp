// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/Be
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/acceptor.h"

#include "benet/channel.h"
#include "benet/eventloop.h"
#include "benet/logger.h"
#include "benet/socket_ops.h"

namespace benet::details {
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
}  // namespace benet::details

namespace benet {

Acceptor::Acceptor(EventLoop* loop, const InetAddress& addr, bool reuseport)
    : loop_(loop),
      accept_socket_(details::create_socketfd(addr.family())),
      accept_channel_(std::make_unique<Channel>(loop_, accept_socket_.fd())),
      idle_fd_(details::create_idlefd()) {
  accept_socket_.SetReuseAddr(true);
  accept_socket_.SetReusePort(reuseport);
  accept_socket_.Bind(addr);
  accept_channel_->BindReadCallback([this]() {
    loop_->AssertInLoopThread();
    InetAddress peer_addr;

    // [TODO] loop until no more conn.
    int connfd = accept_socket_.Accept(&peer_addr);

    if (connfd < 0) {
      BELOG_ERROR("Failed to accept connfd, errno {}: {}", errno, ERRNO_MSG);
      // 处理 Accept 的错误
      if (errno == EMFILE /* Too many open files */) {
        close_excessfd_with_idlefd(accept_socket_.fd());
      }
      return;
    }

    if (on_new_connection_) {
      // 接受到的 connfd 需要通过 on_new_connection_ 回调去处理，
      // Acceptor 类本身只做接受操作，不处理接受到的 connfd。
      // 如果没有设置 on_new_connection_ 回调，
      // 则 Acceptor 将会直接关闭接受到的 connfd。
      // 这里的 on_new_connection_ 回调将会在 TcpServer 类中去设置，
      // 将会把 connfd 封装为 TcpConnection 并且进行后续的处理。
      on_new_connection_(connfd, peer_addr);
    } else {
      sockets::close(connfd);
    }
  });
}

Acceptor::~Acceptor() {
  accept_channel_->DisableAllEvent();
  accept_channel_->RemoveFromLoop();
  ::close(idle_fd_);
}

void Acceptor::Listen() {
  loop_->AssertInLoopThread();
  listening_ = true;
  accept_socket_.Listen();
  accept_channel_->EnableReadEvent();
}

void Acceptor::BindNewConnCallback(const NewConnCallback& cb) {
  on_new_connection_ = cb;
}

void Acceptor::close_excessfd_with_idlefd(int excessfd) {
  ::close(idle_fd_);                          // 腾出 idle 位置
  idle_fd_ = ::accept(excessfd, NULL, NULL);  // 接受 excess 到 idle 位置
  ::close(idle_fd_);                          // 关闭 excess
  idle_fd_ = details::create_idlefd();        // 恢复 idle 位置
}

}  // namespace benet
