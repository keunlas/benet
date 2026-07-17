// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_TCP_SERVER_H_
#define KEUNLAS_BENET_TCP_SERVER_H_

#include <map>

#include "benet/acceptor.h"
#include "benet/callbacks.h"
#include "benet/eventloop.h"
#include "benet/eventloop_threadpool.h"
#include "benet/tcp_connection.h"

namespace benet {

class TcpServer : NotCopyableOrMovable {
 public:
  TcpServer(EventLoop* loop, const InetAddress& listen_addr,
            bool reuse_port = false, const std::string& name = "");
  ~TcpServer();

  inline const InetAddress& addr() const { return listen_addr_; }
  inline const std::string& name() const { return name_; }
  inline EventLoop* loop() const { return loop_; }
  inline auto threadpool() { return thread_pool_; }

  void InitThreadsNumber(int n_threads);
  void Start();

  void BindThreadInitCallback(const std::function<void(EventLoop*)>& cb);
  void BindConnectionCallback(const ConnectionCallback& cb);
  void BindMessageCallback(const MessageCallback& cb);
  void BindWriteCompleteCallback(const WriteCompleteCallback& cb);

 private:
  void new_connection(int sockfd, const InetAddress& peer_addr);
  void remove_connection(const TcpConnectionPtr& conn);

  EventLoop* loop_;
  const InetAddress listen_addr_;
  const std::string name_;

  std::unique_ptr<Acceptor> acceptor_;
  std::shared_ptr<EventLoopThreadPool> thread_pool_;

  std::function<void(EventLoop*)> thread_init_cb_{};
  ConnectionCallback connection_cb_{default_connection_callback};
  MessageCallback message_cb_{default_message_callback};
  WriteCompleteCallback write_complete_cb_{};

  std::atomic_bool started_{false};
  std::map<std::string, TcpConnectionPtr> connections_;
};

}  // namespace benet

#endif  // !KEUNLAS_BENET_TCP_SERVER_H_
