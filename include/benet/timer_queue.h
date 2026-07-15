// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_TIMER_QUEUE_H_
#define BENET_TIMER_QUEUE_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>

#include "benet/channel.h"
#include "benet/copy_move_type.h"
#include "benet/timestamp.h"

namespace benet {

class EventLoop;
class Timer;
using TimerPtr = std::shared_ptr<Timer>;
using TimerWeakPtr = std::weak_ptr<Timer>;

class TimerQueue : NotCopyableOrMovable {
 public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  TimerWeakPtr AddTimer(std::function<void()> cb, TimePoint when,
                        double interval = 0.0);

  void CancelTimer(TimerWeakPtr);

 private:
  using TimerEntry = std::pair<TimePoint, TimerPtr>;
  using TimerVector = std::vector<TimerEntry>;
  using TimerSet = std::set<TimerEntry>;
  using ActiveTimerEntry = std::pair<TimerPtr, int64_t>;
  using ActiveTimerSet = std::set<ActiveTimerEntry>;

  void handle_timerfd();

  TimerVector get_expired_timer(TimePoint now);
  void reset_expired_timer(const TimerVector& expired, TimePoint now);

  bool insert(TimerPtr timer);

 private:
  EventLoop* loop_;
  const int timerfd_;
  Channel timerfd_channel_;

  TimerSet timers_{};
  std::atomic_bool calling_expired_timers_{false};
  ActiveTimerSet canceling_timers_{};
};

}  // namespace benet

#endif  // !BENET_TIMER_QUEUE_H_
