// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/eventloop_threadpool.h"

#include <format>

#include "benet/eventloop.h"
#include "benet/eventloop_thread.h"
#include "benet/logger.h"

namespace benet {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base,
                                         const std::string& name)
    : base_loop_(base), name_(name) {}

EventLoopThreadPool::~EventLoopThreadPool() {
  // Don't delete EventLoop,
  // it's stack variable in EventLoopThread's entry function.
}

void EventLoopThreadPool::InitThreadsNumber(int n_threads) {
  assert(!started_);
  num_threads_ = n_threads;
}

void EventLoopThreadPool::Start(const ThreadInitCallback& init_cb) {
  assert(!started_);
  base_loop_->AssertInLoopThread();
  started_ = true;

  for (int i = 0; i < num_threads_; ++i) {
    auto thread_name = std::format("{}/{}", name_, i);
    auto* thread_ptr = new EventLoopThread(init_cb, thread_name);
    threads_.emplace_back(thread_ptr);
    loops_.push_back(thread_ptr->Start());
  }

  if (num_threads_ == 0 && init_cb) {
    init_cb(base_loop_);
  }
}

EventLoop* EventLoopThreadPool::GetNextLoop() {
  base_loop_->AssertInLoopThread();
  assert(started_);
  static std::size_t next_index_{0};
  EventLoop* loop = base_loop_;
  if (!loops_.empty()) {
    loop = loops_[next_index_ % loops_.size()];
    next_index_ += 1;
  }
  return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::GetAllLoops() {
  base_loop_->AssertInLoopThread();
  assert(started_);
  if (loops_.empty()) {
    return {base_loop_};
  } else {
    return loops_;
  }
}

bool EventLoopThreadPool::IsStarted() const { return started_; }

const std::string& EventLoopThreadPool::name() const { return name_; }

}  // namespace benet
