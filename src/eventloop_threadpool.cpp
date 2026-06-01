
// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/eventloop_threadpool.h"

#include "benet/eventloop.h"

/* EventLoopThreadPool */
namespace benet {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop,
                                         const std::string& name)
    : base_loop_(base_loop), name_(name) {}

EventLoopThreadPool::~EventLoopThreadPool() {
  // Don't delete loop, it's stack variable in EventLoopThread's entry function.
}

void EventLoopThreadPool::Start(
    const std::function<void(EventLoop*)>& init_cb) {
  assert(!started_);
  base_loop_->AssertInLoopThread();
  started_ = true;

  for (int i = 0; i < num_threads_; ++i) {
    auto thread_name = name_ + std::to_string(i);
    EventLoopThread* thread = new EventLoopThread(init_cb, thread_name);
    threads_.push_back(std::unique_ptr<EventLoopThread>(thread));
    loops_.push_back(thread->Start());
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
    loop = loops_[next_index_++];
    if (next_index_ >= loops_.size()) {
      next_index_ = 0;
    }
  }
  return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::GetAllLoops() {
  base_loop_->AssertInLoopThread();
  assert(started_);
  if (loops_.empty()) {
    return std::vector<EventLoop*>(1, base_loop_);
  } else {
    return loops_;
  }
}

void EventLoopThreadPool::SetThreadNumber(int n_threads) {
  num_threads_ = n_threads;
}

bool EventLoopThreadPool::IsStarted() const { return started_; }

const std::string& EventLoopThreadPool::name() const { return name_; }

}  // namespace benet

/* EventLoopThread */
namespace benet {

EventLoopThread::EventLoopThread(const std::function<void(EventLoop*)>& init_cb,
                                 const std::string& name)
    : name_(name), callback_(init_cb) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;

  // not 100% race-free, eg. threadFunc could be running  callback_.
  if (loop_ != nullptr) {
    // still a tiny chance to call destructed object, if threadFunc exits just
    // now. but when EventLoopThread destructs, usually programming is exiting
    // anyway.
    loop_->Stop();
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

EventLoop* EventLoopThread::Start() {
  thread_ = std::thread(std::bind(&EventLoopThread::thread_entry, this));
  EventLoop* loop = nullptr;
  {
    std::unique_lock lock(mutex_);
    while (loop_ == nullptr) {
      cond_.wait_for(lock, std::chrono::milliseconds(1000));
    }
    loop = loop_;
  }
  return loop;
}

void EventLoopThread::thread_entry() {
  EventLoop loop;
  if (callback_) {
    callback_(&loop);
  }
  {
    std::unique_lock lock(mutex_);
    loop_ = &loop;
    cond_.notify_one();
  }
  loop.Start();
  {
    std::lock_guard lock(mutex_);
    loop_ = nullptr;
  }
}

}  // namespace benet
