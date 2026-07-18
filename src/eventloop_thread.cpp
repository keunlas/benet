
// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/eventloop_thread.h"

#include "benet/eventloop.h"
#include "benet/logger.h"

namespace benet {

EventLoopThread::EventLoopThread(const ThreadInitCallback& init_cb,
                                 const std::string& name)
    : name_(name), init_callback_(init_cb) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;

  // not 100% race-free, eg. threadFunc could be running init_callback_.
  if (loop_ != nullptr) {
    // still a tiny chance to call destructed object, if threadFunc exits just
    // now. but when EventLoopThread destructs, usually programming is exiting
    // anyway.
    loop_->Stop();
  }

  if (thread_.joinable()) thread_.join();
}

EventLoop* EventLoopThread::Start() {
  // Thread Entry
  thread_ = std::thread([this]() {
    EventLoop loop;
    if (init_callback_) init_callback_(&loop);

    {
      std::unique_lock lock(mutex_);
      loop_ = &loop;
    }
    cond_.notify_one();

    loop.Start();  // blocking here

    {
      std::lock_guard lock(mutex_);
      loop_ = nullptr;
    }
  });

  // Return loop's ptr
  EventLoop* loop = nullptr;
  {
    std::unique_lock lock(mutex_);
    while (!loop_) cond_.wait_for(lock, std::chrono::milliseconds(128));
    loop = loop_;
  }
  return loop;
}

}  // namespace benet
