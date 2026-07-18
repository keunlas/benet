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
   * @param name 线程池名字
   */
  EventLoopThreadPool(EventLoop* base, const std::string& name);

  ~EventLoopThreadPool();

  /// @brief 初始化线程池线程数（为零值时使用 base 事件循环单线程）
  void InitThreadsNumber(int n_threads);

  /// @brief 启动线程池
  void Start(const ThreadInitCallback& init_cb = ThreadInitCallback());

  /// @brief 获取一个事件循环指针
  EventLoop* GetNextLoop();  // round-robin

  /// @brief 获取目前所有的事件循环指针
  std::vector<EventLoop*> GetAllLoops();

  /// @brief 获知是否线程池已启动
  bool IsStarted() const;

  /// @brief 获取线程池的名称
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

#endif  // !KEUNLAS_BENET_EVENTLOOP_THREADPOOL_H_
