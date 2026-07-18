// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_POLLER_H_
#define KEUNLAS_BENET_POLLER_H_

#include <sys/epoll.h>

#include <unordered_map>
#include <vector>

#include "benet/copy_move_policy.h"
#include "benet/types.h"

namespace benet {
class Channel;
class EventLoop;
}  // namespace benet

namespace benet {
/**
 * @brief Poller
 *
 */
class Poller : NotCopyableOrMovable {
 public:
  Poller(EventLoop* loop) : loop_(loop) {}
  virtual ~Poller() = default;

  /**
   * @brief 轮询主函数
   *
   * @param timeout_ms 轮询超时时间（毫秒）
   * @param actives [OUT] 激活状态的 Channel
   * @return TimePoint 轮询返回时刻
   */
  virtual TimePoint Poll(int timeout_ms, std::vector<Channel*>& actives) = 0;

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

#endif  // !KEUNLAS_BENET_POLLER_H_
