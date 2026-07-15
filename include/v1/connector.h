// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_CONNECTOR_H
#define BENET_CONNECTOR_H

#include <atomic>
#include <functional>
#include <memory>

#include "benet/channel.h"
#include "benet/copy_move_policy.h"
#include "benet/inet_address.h"

namespace benet {

class Channel;
class EventLoop;

class Connector : NotCopyableOrMovable,
                  public std::enable_shared_from_this<Connector> {
 public:
  Connector(EventLoop* loop, const InetAddress& server_addr);
  ~Connector();

  void BindNewConnCallback(const std::function<void(int sockfd)>& cb);

  void Start();    // can be called in any thread
  void Restart();  // must be called in loop thread
  void Stop();     // can be called in any thread

  const InetAddress& addr() const { return server_addr_; }

 private:
  enum class States { Disconnected, Connecting, Connected };

  static constexpr double kMaxRetryDelayMs = 30.0 * 1000.0;
  static constexpr double kInitRetryDelayMs = 500.0;

  void set_state(States s) { state_.store(s); }

  void starting();

  void connect();
  void connecting(int sockfd);
  void retry(int sockfd);

  void handle_write();
  void handle_error();

  int remove_and_reset_channel();
  void reset_channel() { channel_.reset(); }

 private:
  EventLoop* loop_;
  InetAddress server_addr_;

  std::atomic_bool connect_{false};
  std::atomic<States> state_{States::Disconnected};

  std::unique_ptr<Channel> channel_;
  std::function<void(int sockfd)> new_connection_cb_;

  double retry_delay_ms_{kInitRetryDelayMs};
};

using ConnectorPtr = std::shared_ptr<Connector>;

}  // namespace benet

#endif  // !BENET_CONNECTOR_H
