// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_ACCEPTOR_H
#define BENET_ACCEPTOR_H

#include <functional>

#include "benet/copy_move_type.h"
#include "benet/eventloop.h"
#include "benet/socket.h"

namespace benet {

class Acceptor : NotCopyableOrMovable {
 public:
  using NewConnCallback = std::function<void(int sockfd, const InetAddress&)>;

 public:
  Acceptor(EventLoop* loop, const InetAddress& addr, bool reuseport);
  ~Acceptor();

  void Listen();

  bool IsListening() const { return listening_; }

  void BindNewConnCallback(const NewConnCallback& cb);

 private:
  void handle_new_conn();

 private:
  EventLoop* loop_;
  Socket accept_socket_;
  Channel accept_channel_;
  NewConnCallback on_new_connection_;
  bool listening_{false};

  int idle_fd_;
};

}  // namespace benet

#endif  // !BENET_ACCEPTOR_H