// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_TCP_CONNECTION_H_
#define KEUNLAS_BENET_TCP_CONNECTION_H_

#include <any>

#include "benet/buffer.h"
#include "benet/callbacks.h"
#include "benet/eventloop.h"
#include "benet/socket.h"
#include "benet/types.h"

namespace benet {
extern void default_connection_callback(const TcpConnectionPtr&);
extern void default_message_callback(const TcpConnectionPtr&, Buffer*,
                                     TimePoint);
}  // namespace benet

namespace benet {

/**
 * @brief TCP 连接
 *
 */
class TcpConnection : NotCopyableOrMovable,
                      public std::enable_shared_from_this<TcpConnection> {
 public:
  TcpConnection(EventLoop* loop, int sockfd, const InetAddress& local_addr,
                const InetAddress& peer_addr, const std::string& name = "");
  ~TcpConnection();

  inline EventLoop* loop() const { return loop_; }
  inline const std::string& name() const { return name_; }

  inline const InetAddress& LocalAddress() const { return local_addr_; }
  inline const InetAddress& PeerAddress() const { return peer_addr_; }

  inline std::string AsString() const {
    return local_addr_.AsString() + " <=> " + peer_addr_.AsString();
  }

  bool IsConnected() const { return state_.load() == State::Connected; }
  bool IsDisconnected() const { return state_.load() == State::Disconnected; }

  void Send(const void* msg, size_t len);
  void Send(const std::string_view& msg);
  void Send(Buffer* buf);

  void Shutdown();
  void ForceClose();
  void ForceCloseDelay(double sec);

  void SetTcpNoDelay(bool on);

  void StartRead();
  void StopRead();
  inline bool is_reading() const { return reading_.load(); }

  const std::any& context() const { return context_; }
  std::any* mutable_context() { return &context_; }
  void set_context(const std::any& context) { context_ = context; }

  void BindConnectionCallback(const ConnectionCallback&);
  void BindCloseCallback(const CloseCallback&);
  void BindMessageCallback(const MessageCallback&);
  void BindWriteCompleteCallback(const WriteCompleteCallback&);
  void BindHighWaterMarkCallback(const HighWaterMarkCallback&, size_t mark);

  inline Buffer* input_buffer() { return &input_buffer_; }
  inline Buffer* output_buffer() { return &output_buffer_; }

  // called only when accepts a new connection
  void ConnectionEstablished();

  // called only when conn removed from server's list
  void ConnectionDestroyed();

 private:
  enum class State { Disconnected, Connecting, Connected, Disconnecting };

  void send_in_loop(const std::string_view& msg);
  void send_in_loop(const void* msg, size_t len);

  void handle_read(TimePoint receive_time);
  void handle_write();
  void handle_close();
  void handle_error();

  void set_state(State s) { state_.store(s); }

  const std::string& state_as_string() const;

 private:
  EventLoop* loop_;
  std::string name_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddress local_addr_;
  const InetAddress peer_addr_;

  std::atomic<State> state_{State::Connecting};
  std::atomic_bool reading_{true};

  size_t high_water_mark_;
  HighWaterMarkCallback on_high_water_mark_;
  ConnectionCallback on_connection_;
  CloseCallback on_close_;
  MessageCallback on_message_;
  WriteCompleteCallback on_write_complete_;

  Buffer input_buffer_{};
  Buffer output_buffer_{};

  std::any context_{};
};

}  // namespace benet

#endif  // !KEUNLAS_BENET_TCP_CONNECTION_H_
