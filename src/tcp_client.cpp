// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/tcp_client.h"

#include "benet/logger.h"
#include "benet/socket_ops.h"

namespace benet {

TcpClient::TcpClient(EventLoop* loop, const InetAddress& server_addr)
    : loop_(loop), connector_(std::make_shared<Connector>(loop, server_addr)) {
  connector_->BindNewConnCallback(
      std::bind(&TcpClient::new_connection, this, std::placeholders::_1));

  BELOG_INFO("TcpClient construct with Connector {}",
             reinterpret_cast<void*>(connector_.get()));
}

TcpClient::~TcpClient() {
  BELOG_INFO("TcpClient deconstruct with Connector {}",
             reinterpret_cast<void*>(connector_.get()));

  TcpConnectionPtr conn;
  bool unique = false;
  {
    std::lock_guard lock(mutex_);
    unique = connection_.unique();
    conn = connection_;
  }

  if (conn) {
    assert(loop_ == conn->loop());

    CloseCallback cb = [this](const TcpConnectionPtr& conn) {
      loop_->QueueInLoop(std::bind(&TcpConnection::ConnectionDestroyed, conn));
    };

    loop_->RunInLoop(std::bind(&TcpConnection::BindCloseCallback, conn, cb));

    if (unique) {
      conn->ForceClose();
    }
  } else {
    connector_->Stop();
    loop_->RunAfter(1, [this]() {
      // [TODO]
      // remove connector
    });
  }
}

void TcpClient::Connect() {
  BELOG_INFO("TcpClient connecting to '{}'", connector_->addr().AsString());

  connect_.store(true);
  connector_->Start();
}

void TcpClient::Disconnect() {
  connect_.store(false);
  {
    std::lock_guard guard(mutex_);
    if (connection_) {
      connection_->Shutdown();
    }
  }
}

void TcpClient::Stop() {
  connect_.store(false);
  connector_->Stop();
}

void TcpClient::new_connection(int sockfd) {
  loop_->AssertInLoopThread();

  InetAddress peer_addr(sockets::get_peer_addr(sockfd));
  InetAddress local_addr(sockets::get_local_addr(sockfd));

  TcpConnectionPtr conn =
      std::make_shared<TcpConnection>(loop_, sockfd, local_addr, peer_addr);

  conn->BindConnectionCallback(connection_cb_);
  conn->BindMessageCallback(message_cb_);
  conn->BindWriteCompleteCallback(write_complete_cb_);
  conn->BindCloseCallback(
      std::bind(&TcpClient::remove_connection, this, std::placeholders::_1));

  {
    std::lock_guard guard(mutex_);
    connection_ = conn;
  }

  conn->ConnectionEstablished();
}

void TcpClient::remove_connection(const TcpConnectionPtr& conn) {
  loop_->AssertInLoopThread();
  assert(loop_ == conn->loop());

  {
    std::lock_guard guard(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  loop_->QueueInLoop(std::bind(&TcpConnection::ConnectionDestroyed, conn));

  if (retry_.load() && connect_.load()) {
    BELOG_INFO("TcpClient reconnecting to {}", connector_->addr().AsString());

    connector_->Restart();
  }
}

TcpConnectionPtr TcpClient::connection() {
  TcpConnectionPtr tmp;
  {
    std::lock_guard guard(mutex_);
    tmp = connection_;
  }
  return tmp;
}

void TcpClient::BindConnectionCallback(ConnectionCallback cb) {
  connection_cb_ = std::move(cb);
}

void TcpClient::BindMessageCallback(MessageCallback cb) {
  message_cb_ = std::move(cb);
}

void TcpClient::BindWriteCompleteCallback(WriteCompleteCallback cb) {
  write_complete_cb_ = std::move(cb);
}

}  // namespace benet
