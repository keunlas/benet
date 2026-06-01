// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_TCPCLIENT_H
#define BENET_TCPCLIENT_H

#include <functional>
#include <mutex>

#include "benet/callbacks.h"
#include "benet/connector.h"
#include "benet/eventloop.h"
#include "benet/inet_address.h"
#include "benet/tcp_connection.h"

namespace benet {

class TcpClient : NotCopyableOrMovable {
 public:
  TcpClient(EventLoop* loop, const InetAddress& server_addr);
  ~TcpClient();

  void Connect();
  void Disconnect();
  void Stop();

  TcpConnectionPtr connection();
  inline EventLoop* loop() const { return loop_; }

  inline bool IsRetry() const { return retry_.load(); }
  inline void EnableRetry() { retry_.store(true); }

  void BindConnectionCallback(ConnectionCallback cb);
  void BindMessageCallback(MessageCallback cb);
  void BindWriteCompleteCallback(WriteCompleteCallback cb);

 private:
  void new_connection(int sockfd);                       // must in loop
  void remove_connection(const TcpConnectionPtr& conn);  // must in loop

 private:
  EventLoop* loop_;
  ConnectorPtr connector_;

  ConnectionCallback connection_cb_{default_connection_callback};
  MessageCallback message_cb_{default_message_callback};
  WriteCompleteCallback write_complete_cb_;

  std::atomic_bool retry_{false};
  std::atomic_bool connect_{true};

  int next_conn_id_{1};
  std::mutex mutex_;
  TcpConnectionPtr connection_;
};

}  // namespace benet

#endif  // !BENET_TCPCLIENT_H
