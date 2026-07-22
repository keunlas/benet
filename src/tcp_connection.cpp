// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/tcp_connection.h"

#include "benet/logger.h"
#include "benet/socket_ops.h"

namespace benet {

TcpConnection::TcpConnection(EventLoop* loop, int sockfd,
                             const InetAddress& local_addr,
                             const InetAddress& peer_addr,
                             const std::string& name)
    : loop_(loop),
      name_(name),
      socket_(std::make_unique<Socket>(sockfd)),
      channel_(std::make_unique<Channel>(loop_, sockfd)),
      local_addr_(local_addr),
      peer_addr_(peer_addr) {
  using std::placeholders::_1;
  channel_->BindReadCallback(std::bind(&TcpConnection::handle_read, this, _1));
  channel_->BindWriteCallback(std::bind(&TcpConnection::handle_write, this));
  channel_->BindCloseCallback(std::bind(&TcpConnection::handle_close, this));
  channel_->BindErrorCallback(std::bind(&TcpConnection::handle_error, this));
  BELOG_TRACE("TcpConnection constructed at sockfd {}", sockfd);
  socket_->SetKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  BELOG_TRACE("TcpConnection destructed at sockfd {}, state = {}",
              channel_->fd(), state_as_string());
  assert(state_ == State::Disconnected);
}

void TcpConnection::Send(std::string_view msg) {
  Send(reinterpret_cast<const void*>(msg.data()), msg.size());
}

void TcpConnection::Send(const void* msg, size_t len) {
  if (state_.load() != State::Connected) return;
  if (loop_->IsInLoopThread()) {
    send_in_loop(msg, len);
  } else {
    auto conn = shared_from_this();
    std::string message(static_cast<const char*>(msg), len);
    loop_->RunInLoop([conn, message = std::move(message)]() {
      conn->send_in_loop(message.data(), message.size());
    });
  }
}

void TcpConnection::Send(Buffer* buf) {
  if (state_.load() != State::Connected) return;
  if (loop_->IsInLoopThread()) {
    send_in_loop(buf->Peek(), buf->ReadableBytes());
    buf->RetrieveAll();
  } else {
    auto conn = shared_from_this();
    auto message = buf->RetrieveAllAsString();
    loop_->RunInLoop([conn, message = std::move(message)]() {
      conn->send_in_loop(message.data(), message.size());
    });
  }
}

void TcpConnection::send_in_loop(const void* msg, size_t len) {
  loop_->AssertInLoopThread();

  if (state_.load() != State::Connected) {
    BELOG_WARN("{}, writing are rejected", state_as_string());
    return;
  }

  ssize_t n_wrote = 0;
  size_t n_remaining = len;
  bool fault_error = false;

  // send directly, when no write event AND no data in output buffer

  if (!channel_->IsWriteEvent() && output_buffer_.ReadableBytes() == 0) {
    n_wrote = ::write(channel_->fd(), msg, len);

    if (n_wrote >= 0) {
      n_remaining = len - n_wrote;
      if (n_remaining == 0 && on_write_complete_) {
        loop_->QueueInLoop(std::bind(on_write_complete_, shared_from_this()));
      }
    } else {
      n_wrote = 0;
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        BELOG_ERROR("Failed send directly in loop: {}", ERRNO_MSG);
        if (errno == EINTR) {
          // [TODO] retry to send
        }
        if (errno == EPIPE || errno == ECONNRESET) {
          fault_error = true;
        }
      }
    }
  }

  if (fault_error) {
    handle_error_with_code(errno);
    handle_close();
    return;
  }

  if (n_remaining <= 0) {
    return;
  }

  // Add remaining to output buffers

  size_t old_buf_len = output_buffer_.ReadableBytes();
  if (old_buf_len + n_remaining >= high_water_mark_ &&
      old_buf_len < high_water_mark_ && on_high_water_mark_) {
    loop_->QueueInLoop(std::bind(on_high_water_mark_, shared_from_this(),
                                 old_buf_len + n_remaining));
  }

  output_buffer_.Append(static_cast<const char*>(msg) + n_wrote, n_remaining);
  if (!channel_->IsWriteEvent()) {
    channel_->EnableWriteEvent();
  }
}

void TcpConnection::Shutdown() {
  State expected = State::Connected;
  if (state_.compare_exchange_strong(expected, State::Disconnecting)) {
    auto conn = shared_from_this();
    loop_->RunInLoop([this, conn]() {
      loop_->AssertInLoopThread();
      // 如果还有写事件，则留给 handle_wirte 处理
      if (!channel_->IsWriteEvent()) {
        socket_->ShutdownWrite();
      }
    });
  }
}

void TcpConnection::ForceClose() {
  State expected = State::Connected;
  if (state_.compare_exchange_strong(expected, State::Disconnecting) ||
      state_.load() == State::Disconnecting) {
    auto conn = shared_from_this();
    loop_->QueueInLoop([this, conn]() {
      loop_->AssertInLoopThread();
      if (state_ == State::Connected || state_ == State::Disconnecting) {
        handle_close();
      }
    });
  }
}

void TcpConnection::ForceCloseDelay(double sec) {
  std::weak_ptr<TcpConnection> conn_weak(shared_from_this());
  loop_->RunAfter(sec, [conn_weak]() {
    TcpConnectionPtr conn(conn_weak.lock());
    if (conn) conn->ForceClose();
  });
}

void TcpConnection::SetTcpNoDelay(bool on) { socket_->SetTcpNodelay(on); }

void TcpConnection::StartRead() {
  loop_->RunInLoop([this] {
    loop_->AssertInLoopThread();
    if (!reading_ || !channel_->IsReadEvent()) {
      channel_->EnableReadEvent();
      reading_.store(true);
    }
  });
}

void TcpConnection::StopRead() {
  loop_->RunInLoop([this] {
    loop_->AssertInLoopThread();
    if (reading_ || channel_->IsReadEvent()) {
      channel_->DisableReadEvent();
      reading_ = false;
    }
  });
}

void TcpConnection::ConnectionEstablished() {
  loop_->AssertInLoopThread();
  assert(state_ == State::Connecting);
  set_state(State::Connected);
  channel_->Tie(shared_from_this());
  channel_->EnableReadEvent();
  on_connection_(shared_from_this());
}

void TcpConnection::ConnectionDestroyed() {
  loop_->AssertInLoopThread();
  State expected = State::Connected;
  if (state_.compare_exchange_strong(expected, State::Disconnected)) {
    channel_->DisableAllEvent();
    on_connection_(shared_from_this());
  }
  channel_->RemoveFromLoop();
}

void TcpConnection::handle_read(TimePoint receive_time) {
  loop_->AssertInLoopThread();
  ssize_t n = input_buffer_.ReadFd(channel_->fd());

  if (n > 0) {
    on_message_(shared_from_this(), &input_buffer_, receive_time);
  } else if (n == 0) {
    handle_close();
  } else {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return;  // because connfd is nonblocking
    }
    if (errno == EINTR) {
      return;  // [TODO] retry it
    }
    BELOG_ERROR("Failed to read Channel fd {}, errno {}: {}", channel_->fd(),
                errno, ERRNO_MSG);
    handle_error_with_code(errno);
    handle_close();
  }
}

void TcpConnection::handle_write() {
  loop_->AssertInLoopThread();

  if (!channel_->IsWriteEvent()) {
    BELOG_DEBUG("Channel fd {} already down, no more writing");
    return;
  }

  ssize_t n = ::write(channel_->fd(), output_buffer_.Peek(),
                      output_buffer_.ReadableBytes());

  if (n <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return;  // because connfd is nonblocking
    }
    if (errno == EINTR) {
      return;  // [TODO] retry it
    }
    BELOG_ERROR("Failed to write Channel fd {}, errno {}: {}", channel_->fd(),
                errno, ERRNO_MSG);
    handle_error_with_code(errno);
    handle_close();
    return;
  }

  output_buffer_.Retrieve(n);
  if (output_buffer_.ReadableBytes() > 0) {
    return;
  }

  channel_->DisableWriteEvent();
  if (on_write_complete_) {
    loop_->QueueInLoop(std::bind(on_write_complete_, shared_from_this()));
  }

  if (state_ == State::Disconnecting) {
    loop_->RunInLoop([this]() {
      if (!channel_->IsWriteEvent()) {
        socket_->ShutdownWrite();
      }
    });
  }
}

void TcpConnection::handle_close() {
  loop_->AssertInLoopThread();

  auto curr_state = state_.load();
  if (curr_state != State::Connected && curr_state != State::Disconnecting) {
    BELOG_ERROR(
        "Channel fd {} close, but TcpConnection not connected or already close",
        channel_->fd());
    return;
  }

  set_state(State::Disconnected);
  channel_->DisableAllEvent();

  int so_error = sockets::get_socket_error(channel_->fd());
  BELOG_DEBUG(
      "TcpConnection at sockfd {} is handling close, "
      "errno {}: {}, "
      "SO_ERROR {}: {}",
      channel_->fd(), errno, ERRNO_MSG, so_error, ERRCODE_MSG(so_error));

  TcpConnectionPtr guard(shared_from_this());
  on_connection_(guard);
  on_close_(guard);  // This must is last line.
}

void TcpConnection::handle_error() {
  int errcode = sockets::get_socket_error(channel_->fd());
  BELOG_ERROR("TcpConnection at sockfd {} has SO_ERROR, errno {}: {}",
              channel_->fd(), errcode, ERRCODE_MSG(errcode));
}

void TcpConnection::handle_error_with_code(int errcode) {
  int so_error = sockets::get_socket_error(channel_->fd());
  BELOG_ERROR("TcpConnection at sockfd {} error, errno {}: {}, SO_ERROR {}: {}",
              channel_->fd(), errcode, ERRCODE_MSG(errcode), so_error,
              ERRCODE_MSG(so_error));
}

const std::string& TcpConnection::state_as_string() const {
  static const std::string disconnected("Disconnected");
  static const std::string connecting("Connecting");
  static const std::string connected("Connected");
  static const std::string disconnecting("Disconnecting");
  static const std::string unknow("unknown state");
  switch (state_) {
    case State::Disconnected:
      return disconnected;
    case State::Connecting:
      return connecting;
    case State::Connected:
      return connected;
    case State::Disconnecting:
      return disconnecting;
    default:
      return unknow;
  }
}

void TcpConnection::BindConnectionCallback(const ConnectionCallback& cb) {
  on_connection_ = cb;
}

void TcpConnection::BindCloseCallback(const CloseCallback& cb) {
  on_close_ = cb;
}

void TcpConnection::BindMessageCallback(const MessageCallback& cb) {
  on_message_ = cb;
}

void TcpConnection::BindWriteCompleteCallback(const WriteCompleteCallback& cb) {
  on_write_complete_ = cb;
}

void TcpConnection::BindHighWaterMarkCallback(const HighWaterMarkCallback& cb,
                                              size_t high_water_mark) {
  on_high_water_mark_ = cb;
  high_water_mark_ = high_water_mark;
}

}  // namespace benet

/* Some Default Callbacks */
namespace benet {
void default_connection_callback(const TcpConnectionPtr& conn) {
  (void)conn;
  BELOG_TRACE("{} -> {} is {}", conn->LocalAddress().AsString(),
              conn->PeerAddress().AsString(),
              (conn->IsConnected() ? "UP" : "DOWN"));
}
void default_message_callback(const TcpConnectionPtr& conn, Buffer* buffer,
                              TimePoint recv_time) {
  (void)conn;
  (void)buffer;
  (void)recv_time;
  buffer->RetrieveAll();
}
}  // namespace benet
