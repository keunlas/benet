// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_EVENTLOOP_H_
#define KEUNLAS_BENET_EVENTLOOP_H_

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
#include "benet/poller.h"
#include "benet/timer_queue.h"
#include "benet/timestamp.h"

namespace benet {

class EventLoop : NotCopyableOrMovable {
 public:
  /// @brief 构造函数
  EventLoop();
  /// @brief 析构函数
  ~EventLoop();

  /// @brief 开始事件循环
  void Start();
  /// @brief 结束事件循环
  void Stop();

  void RunInLoop(std::function<void()> cb);
  void QueueInLoop(std::function<void()> cb);

  // TimerWeakPtr RunAt(TimePoint time, std::function<void()> cb);
  // TimerWeakPtr RunAfter(double delay, std::function<void()> cb);
  // TimerWeakPtr RunEvery(double interval, std::function<void()> cb);
  // void CancelTimer(TimerWeakPtr timer_wk);

  // void WakeUp();

  // void UpdateChannel(Channel* channel);
  // void RemoveChannel(Channel* channel);
  // bool HasChannel(Channel* channel);

 public:
  /// @brief 获取当前线程的事件循环指针
  static EventLoop* CurrentThreadEventLoop();
  /// @brief 若当前线程的事件循环不是自己时崩溃掉程序
  void AssertInLoopThread() const;
  /// @brief 获知当前线程的事件循环是否是自己
  bool IsInLoopThread() const;

 private:
  /// @brief 立刻唤醒事件循环内部的循环
  void set_wakeup();
  /// @brief 重置用于唤醒循环的文件描述符
  void reset_wakeup();

 private:
  // std::atomic_bool running_{false};
  // std::atomic_bool quitting_{false};
  // std::thread::id thread_id_{std::this_thread::get_id()};

  // std::atomic_bool event_handing_{false};
  // std::unique_ptr<Poller> poller_{std::make_unique<EPoller>(this)};
  // TimePoint poll_return_time_{};
  // std::vector<Channel*> active_channels_{};
  // Channel* current_active_channel_{nullptr};

  // std::atomic_bool calling_pendings_{false};
  // std::vector<std::function<void()>> pendings_{};
  // std::mutex pendings_mutex_{};

  // int wakeup_fd_;
  // std::unique_ptr<Channel> wakeup_channel_;

  // std::unique_ptr<TimerQueue> timer_queue_;
};

}  // namespace benet

#endif  // !KEUNLAS_BENET_EVENTLOOP_H_
