// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_TIMER_QUEUE_H_
#define KEUNLAS_BENET_TIMER_QUEUE_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>

#include "benet/copy_move_policy.h"
#include "benet/types.h"

namespace benet {
class EventLoop;
class Channel;
class Timer;
using TimerPtr = std::shared_ptr<Timer>;
using TimerWeakPtr = std::weak_ptr<Timer>;
using TimerEntry = std::pair<TimePoint, TimerPtr>;
}  // namespace benet

namespace benet {

/**
 * @brief 定时器队列
 *
 */
class TimerQueue : NotCopyableOrMovable {
 public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  /**
   * @brief 添加一个新的定时器任务
   *
   * @param cb 定时任务的回调函数
   * @param when 定时器的激活时间
   * @param interval 重复执行定时任务的间隔（秒）零值代表不重复
   * @return TimerWeakPtr 指向定时器的弱指针
   */
  TimerWeakPtr AddTimer(Functor cb, TimePoint when, double interval = 0.0);

  /**
   * @brief 通过指向定时器的弱指针来取消定时任务
   *
   */
  void CancelTimer(TimerWeakPtr);

 private:
  /**
   * @brief 把新的定时器插入定时器队列
   *
   * @param timer 待插入的定时器指针
   * @return true 插入的定时器激活时间早于下次激活时间
   * @return false 插入的定时器激活时间晚于下次激活时间
   */
  bool insert(TimerPtr timer);

 private:
  EventLoop* loop_;
  const int timerfd_;
  std::unique_ptr<Channel> timerfd_channel_;
  std::set<TimerEntry> timers_{};
  std::atomic_bool calling_expired_timers_{false};
  std::set<TimerPtr> canceling_timers_{};
};

}  // namespace benet

#endif  // !KEUNLAS_BENET_TIMER_QUEUE_H_
