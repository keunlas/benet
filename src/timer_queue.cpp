// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/timer_queue.h"

#include <sys/timerfd.h>

#include <chrono>
#include <cstring>
#include <ctime>

#include "benet/eventloop.h"
#include "benet/logger.h"
#include "benet/timer.h"

namespace benet {
namespace details {
int create_timerfd() {
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
  if (timerfd < 0) {
    BELOG_CRITICAL("Failed to create timerfd, errno {}: {}", errno, ERRNO_MSG);
  }
  return timerfd;
}

timespec how_long_from_now(const TimePoint& time) {
  auto timestamp = time.time_since_epoch().count() -
                   TimeClock::now().time_since_epoch().count();

  timespec val;
  std::memset(&val, 0, sizeof(val));

  if (timestamp < 0) {
    val.tv_nsec = 1;
    return val;
  }

  auto second_part = timestamp / 1'000'000'000;
  auto nano_part = timestamp - second_part * 1'000'000'000;
  val.tv_sec = static_cast<time_t>(second_part);
  val.tv_nsec = static_cast<time_t>(nano_part);
  return val;
}

void reset_timerfd(int fd, TimePoint exp) {
  itimerspec val;
  std::memset(&val, 0, sizeof(val));
  val.it_value = how_long_from_now(exp);
  int err = ::timerfd_settime(fd, 0, &val, nullptr);
  if (err) {
    BELOG_CRITICAL("Failed to timerfd_settime, errno {}: {}", errno, ERRNO_MSG);
  }
}

void read_timerfd(int timerfd) {
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
  if (n != sizeof(howmany)) {
    SPDLOG_ERROR("From timerfd reads {} bytes instead of 8", n);
  }
}
}  // namespace details

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(details::create_timerfd()),
      timerfd_channel_(loop, timerfd_) {
  timerfd_channel_.BindReadCallback(
      std::bind(&TimerQueue::handle_timerfd, this));
  timerfd_channel_.EnableReadEvent();
}

TimerQueue::~TimerQueue() {
  timerfd_channel_.DisableAllEvent();
  timerfd_channel_.RemoveFromLoop();
  ::close(timerfd_);
}

TimerWeakPtr TimerQueue::AddTimer(std::function<void()> cb, TimePoint when,
                                  double interval) {
  TimerPtr timer = std::make_shared<Timer>(std::move(cb), when, interval);
  loop_->RunInLoop([this, timer]() {
    bool earliest_changed = insert(timer);
    if (earliest_changed) {
      details::reset_timerfd(timerfd_, timer->expiration());
    }
  });
  return TimerWeakPtr(timer);
}

void TimerQueue::CancelTimer(TimerWeakPtr timer_wk) {
  loop_->RunInLoop([this, timer_wk]() {
    auto timer = timer_wk.lock();
    if (!timer) {
      return;
    }

    if (timers_.find({timer->expiration(), timer}) != timers_.end()) {
      size_t n = timers_.erase(TimerEntry(timer->expiration(), timer));
      assert(n == 1);
      (void)n;
    } else if (calling_expired_timers_.load() == true) {
      canceling_timers_.insert({timer, timer->sequence()});
    }
  });
}

void TimerQueue::handle_timerfd() {
  loop_->AssertInLoopThread();

  auto now = TimeClock::now();
  details::read_timerfd(timerfd_);

  auto expired_timer = get_expired_timer(now);
  calling_expired_timers_.store(true);
  canceling_timers_.clear();
  for (auto&& it : expired_timer) {
    it.second->Run();
  }
  calling_expired_timers_.store(false);

  reset_expired_timer(expired_timer, now);
}

TimerQueue::TimerVector TimerQueue::get_expired_timer(TimePoint now) {
  TimerVector expired_timer;
  TimerEntry anchor(now, nullptr);
  auto tmp_end = timers_.lower_bound(anchor);
  assert(tmp_end == timers_.end() || now < tmp_end->first);
  std::copy(timers_.begin(), tmp_end, std::back_inserter(expired_timer));
  timers_.erase(timers_.begin(), tmp_end);
  return expired_timer;
}

void TimerQueue::reset_expired_timer(const TimerVector& exp, TimePoint now) {
  TimePoint next_expired_time = kInvalidTimePoint;

  for (auto&& item : exp) {
    bool repeat = item.second->IsRepeat();
    if (!repeat) {
      continue;
    }

    ActiveTimerEntry curr(item.second, item.second->sequence());
    bool cancelled = canceling_timers_.find(curr) == canceling_timers_.end();
    if (cancelled) {
      item.second->Restart(now);
      insert(item.second);
    }
  }

  if (!timers_.empty()) {
    next_expired_time = timers_.begin()->second->expiration();
  }

  if (next_expired_time != kInvalidTimePoint) {
    details::reset_timerfd(timerfd_, next_expired_time);
  }
}

bool TimerQueue::insert(TimerPtr timer) {
  loop_->AssertInLoopThread();

  bool earliest_changed = false;
  TimePoint when = timer->expiration();
  auto it = timers_.begin();
  if (it == timers_.end() || when < it->first) {
    earliest_changed = true;
  }
  {
    auto result = timers_.insert(TimerEntry(when, timer));
    assert(result.second);
    (void)result;
  }

  return earliest_changed;
}

}  // namespace benet
