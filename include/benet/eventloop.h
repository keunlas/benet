// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_EVENTLOOP_H_
#define BENET_EVENTLOOP_H_

#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "benet/channel.h"
#include "benet/copy_move_type.h"
#include "benet/logger.h"
#include "benet/poller.h"
#include "benet/timer_queue.h"
#include "benet/timestamp.h"

namespace benet {

class EventLoop : NotCopyableOrMovable {
 public:
  EventLoop();
  ~EventLoop();

  void Start();
  void Stop();

  void RunInLoop(std::function<void()> cb);
  void QueueInLoop(std::function<void()> cb);

  TimerWeakPtr RunAt(TimePoint time, std::function<void()> cb);
  TimerWeakPtr RunAfter(double delay, std::function<void()> cb);
  TimerWeakPtr RunEvery(double interval, std::function<void()> cb);
  void CancelTimer(TimerWeakPtr timer_wk);

  void WakeUp();

  void UpdateChannel(Channel* channel);
  void RemoveChannel(Channel* channel);
  bool HasChannel(Channel* channel);

 public:
  static EventLoop* CurrentThreadEventLoop();
  void AssertInLoopThread() const;
  bool IsInLoopThread() const;

 private:
  void handle_wakeup();

 private:
  std::atomic_bool running_{false};
  std::atomic_bool quitting_{false};
  std::thread::id thread_id_{std::this_thread::get_id()};

  std::atomic_bool event_handing_{false};
  std::unique_ptr<Poller> poller_{std::make_unique<EPoller>(this)};
  TimePoint poll_return_time_{};
  std::vector<Channel*> active_channels_{};
  Channel* current_active_channel_{nullptr};

  std::atomic_bool calling_pendings_{false};
  std::vector<std::function<void()>> pendings_{};
  std::mutex pendings_mutex_{};

  int wakeup_fd_;
  std::unique_ptr<Channel> wakeup_channel_;

  std::unique_ptr<TimerQueue> timer_queue_;
};

}  // namespace benet

#endif  // !BENET_EVENTLOOP_H_
