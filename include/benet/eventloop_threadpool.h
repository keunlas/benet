
// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_EVENTLOOP_THREADPOOL_H_
#define BENET_EVENTLOOP_THREADPOOL_H_

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "benet/copy_move_policy.h"

namespace benet {

class EventLoop;

class EventLoopThread : NotCopyableOrMovable {
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThread(const std::function<void(EventLoop*)>& init_cb =
                      std::function<void(EventLoop*)>(),
                  const std::string& name = std::string());
  ~EventLoopThread();
  EventLoop* Start();

 private:
  void thread_entry();

 private:
  std::string name_;
  ThreadInitCallback callback_;
  bool exiting_{false};

  EventLoop* loop_{nullptr};

  std::thread thread_{};
  std::mutex mutex_{};
  std::condition_variable cond_{};
};

class EventLoopThreadPool : NotCopyableOrMovable {
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThreadPool(EventLoop* base_loop, const std::string& name);
  ~EventLoopThreadPool();

  void SetThreadNumber(int n_threads);
  void Start(const std::function<void(EventLoop*)>& init_cb =
                 std::function<void(EventLoop*)>());

  EventLoop* GetNextLoop();  // round-robin
  std::vector<EventLoop*> GetAllLoops();

  bool IsStarted() const;

  const std::string& name() const;

 private:
  EventLoop* base_loop_;
  std::string name_;
  bool started_{false};
  int num_threads_{0};
  std::vector<std::unique_ptr<EventLoopThread>> threads_{};
  std::vector<EventLoop*> loops_{};
};

}  // namespace benet

#endif  // !BENET_EVENTLOOP_THREADPOOL_H_
