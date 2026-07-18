// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/tcp_server.h"

#include <format>
#include <map>

#include "benet/logger.h"
#include "benet/socket_ops.h"

namespace benet {

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listen_addr,
                     bool reuse_port, const std::string& name)
    : loop_(loop),
      listen_addr_(listen_addr),
      name_(name),
      acceptor_(std::make_unique<Acceptor>(loop_, listen_addr_, reuse_port)),
      thread_pool_(
          std::make_shared<EventLoopThreadPool>(loop, name_ + ".IoThreads")) {
  acceptor_->BindNewConnCallback(std::bind(&TcpServer::new_connection, this,
                                           std::placeholders::_1,
                                           std::placeholders::_2));
}

TcpServer::~TcpServer() {
  loop_->AssertInLoopThread();
  started_.store(false);
  for (auto&& item : connections_) {
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    conn->loop()->RunInLoop(
        std::bind(&TcpConnection::ConnectionDestroyed, conn));
  }
}

void TcpServer::InitThreadsNumber(int n_threads) {
  thread_pool_->InitThreadsNumber(n_threads);
}

void TcpServer::Start() {
  started_.store(true);
  thread_pool_->Start(thread_init_cb_);
  assert(!acceptor_->IsListening());
  loop_->RunInLoop(std::bind(&Acceptor::Listen, acceptor_.get()));
}

void TcpServer::BindThreadInitCallback(
    const std::function<void(EventLoop*)>& cb) {
  thread_init_cb_ = cb;
}

void TcpServer::BindConnectionCallback(const ConnectionCallback& cb) {
  connection_cb_ = cb;
}

void TcpServer::BindMessageCallback(const MessageCallback& cb) {
  message_cb_ = cb;
}

void TcpServer::BindWriteCompleteCallback(const WriteCompleteCallback& cb) {
  write_complete_cb_ = cb;
}

void TcpServer::new_connection(int sockfd, const InetAddress& peer_addr) {
  loop_->AssertInLoopThread();
  EventLoop* io_loop = thread_pool_->GetNextLoop();

  static uint64_t next_conn_id_{0};

  auto conn_name =
      std::format("{}-{}#{}", name_, listen_addr_.AsString(), next_conn_id_++);

  BELOG_TRACE("TcpServer [{}] get new connection [{}] from '{}'", name_,
              conn_name, peer_addr.AsString());

  InetAddress local_addr(sockets::get_local_addr(sockfd));
  auto conn = std::make_shared<TcpConnection>(io_loop, sockfd, local_addr,
                                              peer_addr, conn_name);
  connections_[conn_name] = conn;
  conn->BindConnectionCallback(connection_cb_);
  conn->BindMessageCallback(message_cb_);
  conn->BindWriteCompleteCallback(write_complete_cb_);
  conn->BindCloseCallback(std::bind(&TcpServer::remove_connection, this,
                                    std::placeholders::_1));  // unsafe
  io_loop->RunInLoop(std::bind(&TcpConnection::ConnectionEstablished, conn));
}

void TcpServer::remove_connection(const TcpConnectionPtr& conn) {
  loop_->RunInLoop([this, conn]() {
    BELOG_TRACE("TcpServer [{}] remove connection [{}]", name_, conn->name());
    size_t n = connections_.erase(conn->name());
    assert(n == 1);
    EventLoop* io_loop = conn->loop();
    io_loop->QueueInLoop(std::bind(&TcpConnection::ConnectionDestroyed, conn));
  });
}

}  // namespace benet
