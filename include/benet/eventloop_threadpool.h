// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_EVENTLOOP_THREADPOOL_H_
#define KEUNLAS_BENET_EVENTLOOP_THREADPOOL_H_

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "benet/callbacks.h"
#include "benet/copy_move_policy.h"

namespace benet {
class EventLoop;
class EventLoopThread;
}  // namespace benet

namespace benet {

/**
 * @brief 事件循环线程池
 *
 */
class EventLoopThreadPool : NotCopyableOrMovable {
 public:
  /**
   * @brief 构造事件循环线程池
   *
   * @param base 主事件循环
   * @param n_threads 线程池线程数（为零值时只使用 base 事件循环）
   * @param name 线程池名字
   */
  EventLoopThreadPool(EventLoop* base, int n_threads, const std::string& name);

  ~EventLoopThreadPool();

  void Start(const ThreadInitCallback& init_cb = ThreadInitCallback());

  EventLoop* GetNextLoop();  // round-robin

  std::vector<EventLoop*> GetAllLoops();

  bool IsStarted() const;

  const std::string& name() const;

 private:
  EventLoop* base_loop_;
  int num_threads_;
  std::string name_;

  bool started_{false};
  std::vector<std::unique_ptr<EventLoopThread>> threads_{};
  std::vector<EventLoop*> loops_{};
};

}  // namespace benet

#endif  // !KEUNLAS_BENET_EVENTLOOP_THREADPOOL_H_
