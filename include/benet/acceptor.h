// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_ACCEPTOR_H_
#define KEUNLAS_BENET_ACCEPTOR_H_

#include <functional>
#include <memory>

#include "benet/copy_move_policy.h"
#include "benet/socket.h"

namespace benet {
class EventLoop;
class Channel;
}  // namespace benet

namespace benet {

class Acceptor : NotCopyableOrMovable {
 public:
  using NewConnCallback =
      std::function<void(int /* sockfd */, const InetAddress& /* addr */)>;

 public:
  Acceptor(EventLoop* loop, const InetAddress& addr, bool reuseport);
  ~Acceptor();

  void Listen();

  bool IsListening() const { return listening_; }

  void BindNewConnCallback(const NewConnCallback& cb);

 private:
  /// @brief 当打开的 fd 太多时，利用 idlefd 去关闭过剩的 fd。
  void close_excessfd_with_idlefd(int excessfd);

 private:
  bool listening_{false};
  EventLoop* loop_;
  Socket accept_socket_;
  std::unique_ptr<Channel> accept_channel_;
  NewConnCallback on_new_connection_;

  int idle_fd_;
};

}  // namespace benet

#endif  // !KEUNLAS_BENET_ACCEPTOR_H_
