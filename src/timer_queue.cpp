// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/timer_queue.h"

#include <sys/timerfd.h>

#include <chrono>
#include <cstring>
#include <ctime>

#include "benet/channel.h"
#include "benet/eventloop.h"
#include "benet/logger.h"
#include "benet/timer.h"

namespace benet::details {
int create_timerfd() {
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
  if (timerfd < 0) {
    BELOG_CRITICAL("Failed to create timerfd, errno {}: {}", errno, ERRNO_MSG);
  }
  BELOG_TRACE("Created timerfd {}", timerfd);
  return timerfd;
}

timespec how_long_from_now(const TimePoint& time) {
  auto now = TimeClock::now();
  if (time <= now) return {0, 1};

  auto diff = time - now;
  auto s = std::chrono::duration_cast<std::chrono::seconds>(diff);
  auto n = std::chrono::duration_cast<std::chrono::nanoseconds>(diff - s);
  return {static_cast<time_t>(s.count()), static_cast<long>(n.count())};
}

void set_timerfd(int fd, TimePoint exp) {
  itimerspec val{0, 0};
  val.it_value = how_long_from_now(exp);
  int err = ::timerfd_settime(fd, 0, &val, nullptr);
  if (err) {
    BELOG_CRITICAL("Failed to timerfd_settime, errno {}: {}", errno, ERRNO_MSG);
  }
}

void reset_timerfd(int timerfd) {
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
  if (n != sizeof(howmany)) {
    SPDLOG_ERROR("From timerfd reads {} bytes instead of 8", n);
  }
}
}  // namespace benet::details

namespace benet {

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(details::create_timerfd()),
      timerfd_channel_(std::make_unique<Channel>(loop, timerfd_)) {
  timerfd_channel_->BindReadCallback([this](TimePoint) {
    loop_->AssertInLoopThread();
    auto now = TimeClock::now();
    details::reset_timerfd(timerfd_);

    // Get expired timer
    std::vector<TimerEntry> expired_timer;
    {
      TimerEntry anchor(now, nullptr);
      auto tmp_end = timers_.lower_bound(anchor);
      assert(tmp_end == timers_.end() || now < tmp_end->first);
      std::copy(timers_.begin(), tmp_end, std::back_inserter(expired_timer));
      timers_.erase(timers_.begin(), tmp_end);
    }

    calling_expired_timers_.store(true);
    canceling_timers_.clear();
    for (auto&& timer_entry : expired_timer) {
      timer_entry.second->Run();
    }
    calling_expired_timers_.store(false);

    // Update expired timer
    TimePoint next_expired_time = Timer::InvalidTimePoint();
    for (auto&& timer_entry : expired_timer) {
      auto timer = timer_entry.second;
      if (!timer->IsRepeat()) continue;
      bool cancelled = canceling_timers_.find(timer) != canceling_timers_.end();
      if (!cancelled) {
        timer->Restart(now);
        insert(timer);
      }
    }

    if (!timers_.empty()) {
      next_expired_time = timers_.begin()->second->expiration();
    }

    if (next_expired_time != Timer::InvalidTimePoint()) {
      details::set_timerfd(timerfd_, next_expired_time);
    }
  });
  timerfd_channel_->EnableReadEvent();
}

TimerQueue::~TimerQueue() {
  timerfd_channel_->DisableAllEvent();
  timerfd_channel_->RemoveFromLoop();
  ::close(timerfd_);
}

TimerWeakPtr TimerQueue::AddTimer(std::function<void()> cb, TimePoint when,
                                  double interval) {
  TimerPtr timer = std::make_shared<Timer>(std::move(cb), when, interval);
  loop_->RunInLoop([this, timer]() {
    bool earliest_changed = insert(timer);
    if (earliest_changed) details::set_timerfd(timerfd_, timer->expiration());
  });
  return TimerWeakPtr(timer);
}

void TimerQueue::CancelTimer(TimerWeakPtr timer_wk) {
  loop_->RunInLoop([this, timer_wk]() {
    auto timer = timer_wk.lock();
    if (!timer) return;
    if (timers_.find({timer->expiration(), timer}) != timers_.end()) {
      size_t n = timers_.erase(TimerEntry(timer->expiration(), timer));
      assert(n == 1);
    } else if (calling_expired_timers_.load() == true) {
      canceling_timers_.insert(timer);
    }
  });
}

bool TimerQueue::insert(TimerPtr timer) {
  loop_->AssertInLoopThread();

  bool earliest_changed = false;
  TimePoint when = timer->expiration();
  auto it = timers_.begin();
  if (it == timers_.end() || when < it->first) {
    earliest_changed = true;
  }

  auto result = timers_.insert({when, timer});
  assert(result.second);

  return earliest_changed;
}

}  // namespace benet
