// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_POLLER_H_
#define BENET_POLLER_H_

#include <sys/epoll.h>

#include <unordered_map>
#include <vector>

#include "benet/copy_move_type.h"
#include "benet/logger.h"
#include "benet/timestamp.h"

namespace benet {

class Channel;
class EventLoop;

class Poller : NotCopyableOrMovable {
 public:
  Poller(EventLoop* loop) : loop_(loop) {}
  virtual ~Poller() = default;

  virtual TimePoint Poll(int timeout_ms, std::vector<Channel*>& actives_) = 0;

  virtual void UpdateChannel(Channel* channel) = 0;
  virtual void RemoveChannel(Channel* channel) = 0;

  bool HasChannel(Channel* channel) const;

  void AssertInLoopThread() const;

 protected:
  std::unordered_map<int, Channel*> channels_;

 private:
  EventLoop* loop_;
};

class EPoller : public Poller {
 public:
  EPoller(EventLoop* loop);
  ~EPoller() override;

  TimePoint Poll(int timeout_ms, std::vector<Channel*>& actives) override;
  void UpdateChannel(Channel* channel) override;
  void RemoveChannel(Channel* channel) override;

 private:
  void fill_actives(int n_events, std::vector<Channel*>& actives) const;
  void update_epoll_operation(int operation, Channel* channel);

 private:
  int epoll_fd_;
  std::vector<epoll_event> events_;
};

}  // namespace benet

#endif  // !BENET_POLLER_H_
