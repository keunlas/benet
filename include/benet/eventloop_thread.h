// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_EVENTLOOP_THREAD_H_
#define KEUNLAS_BENET_EVENTLOOP_THREAD_H_

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
}  // namespace benet

namespace benet {

/**
 * @brief 事件循环线程
 *
 */
class EventLoopThread : NotCopyableOrMovable {
 public:
  EventLoopThread(const ThreadInitCallback& init_cb = ThreadInitCallback(),
                  const std::string& name = std::string());
  ~EventLoopThread();

  /**
   * @brief 线程开始执行
   *
   * @return EventLoop* 内部的事件循环指针
   */
  EventLoop* Start();

 private:
  EventLoop* loop_{nullptr};
  std::mutex mutex_{};
  std::condition_variable cond_{};

  std::string name_;
  ThreadInitCallback init_callback_;

  std::thread thread_{};
  bool exiting_{false};
};

}  // namespace benet

#endif  // !KEUNLAS_BENET_EVENTLOOP_THREAD_H_
