// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/connector.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

#include "benet/eventloop.h"
#include "benet/logger.h"
#include "benet/socket_ops.h"

namespace benet {

Connector::Connector(EventLoop* loop, const InetAddress& server_addr)
    : loop_(loop), server_addr_(server_addr) {
  SPDLOG_DEBUG("Connector constructing");
}

Connector::~Connector() {
  SPDLOG_DEBUG("Connector deconstructing");
  assert(!channel_);
}

void Connector::BindNewConnCallback(const std::function<void(int sockfd)>& cb) {
  new_connection_cb_ = cb;
}

void Connector::Start() {
  connect_.store(true);
  loop_->RunInLoop(std::bind(&Connector::starting, this));
}

void Connector::starting() {
  loop_->AssertInLoopThread();
  assert(state_.load() == States::Disconnected);
  if (connect_.load() == true) {
    connect();
  } else {
    SPDLOG_DEBUG("Do Not Connect");
  }
}

void Connector::Stop() {
  connect_.store(false);
  loop_->RunInLoop([this]() {
    auto expected = States::Connecting;
    if (state_.compare_exchange_strong(expected, States::Disconnected)) {
      int sockfd = remove_and_reset_channel();
      retry(sockfd);
    }
  });
}

void Connector::Restart() {
  loop_->AssertInLoopThread();
  set_state(States::Disconnected);
  retry_delay_ms_ = kInitRetryDelayMs;
  connect_.store(true);
  loop_->RunInLoop(std::bind(&Connector::starting, this));
}

void Connector::connect() {
  int sockfd = sockets::create_nonblocking_or_die(server_addr_.family());
  int ret = sockets::connect(sockfd, server_addr_.addr());
  int code = (ret == 0) ? 0 : errno;
  switch (code) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      connecting(sockfd);
      break;
    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      retry(sockfd);
      break;
    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      BELOG_ERROR("Failed to connect to sockfd = {}: {}", sockfd, ERRNO_MSG);
      sockets::close(sockfd);
      break;
    default:
      BELOG_ERROR("Failed to connect to sockfd = {}: {}", sockfd, ERRNO_MSG);
      sockets::close(sockfd);
      break;
  }
}

void Connector::connecting(int sockfd) {
  set_state(States::Connecting);
  assert(!channel_);

  // channel_->tie(shared_from_this()); is not working,
  // as channel_ is not managed by shared_ptr
  channel_ = std::make_unique<Channel>(loop_, sockfd);
  channel_->BindWriteCallback(std::bind(&Connector::handle_write, this));
  channel_->BindErrorCallback(std::bind(&Connector::handle_error, this));
  channel_->EnableWriteEvent();
}

int Connector::remove_and_reset_channel() {
  channel_->DisableAllEvent();
  // use queue_in_loop to remove channel，
  // avoid to remove active channel in handling event.
  loop_->QueueInLoop(std::bind(&Channel::RemoveFromLoop, channel_.get()));
  int sockfd = channel_->fd();
  loop_->QueueInLoop(std::bind(&Connector::reset_channel, this));
  return sockfd;
}

void Connector::handle_write() {
  BELOG_TRACE("Connector handle write");

  if (state_.load() == States::Connecting) {
    int sockfd = remove_and_reset_channel();
    int err = sockets::get_socket_error(sockfd);
    if (err != 0) {
      BELOG_WARN("Connector SO_ERROR: {}", ERRCODE_MSG(err));
      retry(sockfd);
    } else if (sockets::is_self_connect(sockfd)) {
      BELOG_WARN("Connector Self Connect");
      retry(sockfd);
    } else {
      set_state(States::Connected);
      if (connect_.load() == true) {
        if (new_connection_cb_) new_connection_cb_(sockfd);
      } else {
        sockets::close(sockfd);
      }
    }
  } else {
    if (state_.load() != States::Disconnected) {
      BELOG_ERROR("Unknow Error");
    }
  }
}

void Connector::handle_error() {
  BELOG_ERROR("Connector handle error, state = {}",
              static_cast<int>(state_.load()));
  if (state_.load() == States::Connecting) {
    int sockfd = remove_and_reset_channel();
    int err = sockets::get_socket_error(sockfd);
    BELOG_ERROR("Connector SO_ERROR: {}", ERRCODE_MSG(err));
    retry(sockfd);
  }
}

void Connector::retry(int sockfd) {
  sockets::close(sockfd);
  set_state(States::Disconnected);
  if (connect_.load() == true) {
    BELOG_INFO("Retry to connect to {} in {} ms", server_addr_.AsString(),
               retry_delay_ms_);
    loop_->RunAfter(retry_delay_ms_ / 1000.0,
                    std::bind(&Connector::starting, shared_from_this()));
    retry_delay_ms_ = std::min(retry_delay_ms_ * 2.0, kMaxRetryDelayMs);
  } else {
    SPDLOG_DEBUG("Do Not Connect");
  }
}

}  // namespace benet
