// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/eventloop.h"

#include <sys/eventfd.h>

#include "benet/logger.h"

/// @brief 当前线程的事件循环指针
thread_local benet::EventLoop* tThreadEventLoop{nullptr};

/// @brief 轮询超时时间
static constexpr int kPollTimeoutMs = 8192;

namespace benet::details {
int create_eventfd() {
  // non-block fd
  int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (eventfd < 0) {
    BELOG_CRITICAL("Failed to create eventfd: {}", ERRNO_MSG);
  }
  return eventfd;
}
void close_eventfd(int fd) { ::close(fd); }
}  // namespace benet::details

namespace benet {

EventLoop::EventLoop()
    : wakeup_fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
      wakeup_channel_(std::make_unique<Channel>(this, wakeup_fd_)),
      timer_queue_(std::make_unique<TimerQueue>(this)) {
  BELOG_DEBUG("EventLoop {} created", reinterpret_cast<const void*>(this));
  if (tThreadEventLoop) {
    BELOG_CRITICAL("A EventLoop {} already existed in this thread",
                   reinterpret_cast<const void*>(tThreadEventLoop));
  } else {
    tThreadEventLoop = this;
  }

  wakeup_channel_->BindReadCallback(std::bind(&EventLoop::handle_wakeup, this));
  wakeup_channel_->EnableReadEvent();
}

EventLoop::~EventLoop() {
  assert(running_.load() == false);
  wakeup_channel_->DisableAllEvent();
  wakeup_channel_->RemoveFromLoop();
  ::close(wakeup_fd_);
  tThreadEventLoop = nullptr;
}

void EventLoop::Start() {
  AssertInLoopThread();
  assert(running_.load() == false);
  assert(quitting_.load() == false);

  running_.store(true);
  while (quitting_.load() == false) {
    active_channels_.clear();
    poll_return_time_ = poller_->Poll(kPollTimeoutMs, active_channels_);

    event_handing_.store(true);
    {
      for (Channel* channel : active_channels_) {
        current_active_channel_ = channel;
        channel->HandleEvent(poll_return_time_);
      }
      current_active_channel_ = nullptr;
    }
    event_handing_.store(false);

    calling_pendings_.store(true);
    {
      std::vector<std::function<void()>> tmp_pendings;
      {
        std::lock_guard<std::mutex> guard(pendings_mutex_);
        tmp_pendings.swap(pendings_);
      }
      for (auto&& pending : tmp_pendings) {
        pending();
      }
    }
    calling_pendings_.store(false);
  }
  running_.store(false);
}

void EventLoop::Stop() {
  quitting_.store(true);
  if (!IsInLoopThread()) {
    WakeUp();
  }
  BELOG_DEBUG("EventLoop {} quited", reinterpret_cast<const void*>(this));
}

void EventLoop::RunInLoop(std::function<void()> cb) {
  if (IsInLoopThread()) {
    cb();
  } else {
    QueueInLoop(std::move(cb));
  }
}

void EventLoop::QueueInLoop(std::function<void()> cb) {
  {
    std::lock_guard<std::mutex> guard(pendings_mutex_);
    pendings_.push_back(std::move(cb));
  }

  if (!IsInLoopThread() || calling_pendings_) {
    WakeUp();
  }
}

void EventLoop::WakeUp() {
  uint64_t one = 1;
  auto n = ::write(wakeup_fd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    SPDLOG_ERROR("Wakeup fd {} writes {} bytes instead of 8", wakeup_fd_, n);
  }
}

void EventLoop::handle_wakeup() {
  uint64_t one = 1;
  auto n = ::read(wakeup_fd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    BELOG_ERROR("Wakeup fd {} reads {} bytes instead of 8", wakeup_fd_, n);
  }
}

// void EventLoop::UpdateChannel(Channel* channel) {
//   assert(channel->loop() == this);
//   AssertInLoopThread();
//   poller_->UpdateChannel(channel);
// }

// void EventLoop::RemoveChannel(Channel* channel) {
//   assert(channel->loop() == this);
//   AssertInLoopThread();
//   if (event_handing_.load()) {
//     assert(current_active_channel_ == channel ||
//            std::find(active_channels_.begin(), active_channels_.end(),
//                      channel) == active_channels_.end());
//   }
//   poller_->RemoveChannel(channel);
// }

// bool EventLoop::HasChannel(Channel* channel) {
//   return poller_->HasChannel(channel);
// }

// TimerWeakPtr EventLoop::RunAt(TimePoint time, std::function<void()> cb) {
//   return timer_queue_->AddTimer(std::move(cb), time, 0.0);
// }

// TimerWeakPtr EventLoop::RunAfter(double delay, std::function<void()> cb) {
//   auto now = TimeClock::now();
//   now += std::chrono::nanoseconds(static_cast<int64_t>(delay * 1e9));
//   return timer_queue_->AddTimer(std::move(cb), now, 0.0);
// }

// TimerWeakPtr EventLoop::RunEvery(double interval, std::function<void()> cb) {
//   auto now = TimeClock::now();
//   now += std::chrono::nanoseconds(static_cast<int64_t>(interval * 1e9));
//   return timer_queue_->AddTimer(std::move(cb), now, interval);
// }

// void EventLoop::CancelTimer(TimerWeakPtr timer_wk) {
//   timer_queue_->CancelTimer(timer_wk);
// }

/// @brief 获取当前线程的事件循环指针
EventLoop* EventLoop::CurrentThreadEventLoop() { return tThreadEventLoop; }

/// @brief 若当前线程的事件循环不是自己时崩溃掉程序
void EventLoop::AssertInLoopThread() const {
  if (!IsInLoopThread()) {
    BELOG_CRITICAL("Current thread is not this loop thread");
  }
}

/// @brief 获知当前线程的事件循环是否是自己
bool EventLoop::IsInLoopThread() const {
  return thread_id_ == std::this_thread::get_id();
}

}  // namespace benet
