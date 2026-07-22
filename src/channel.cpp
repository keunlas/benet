// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/channel.h"

#include <sys/epoll.h>
#include <sys/fcntl.h>

#include <string>

#include "benet/eventloop.h"
#include "benet/logger.h"

namespace {
static constexpr int kNoneEvent{0};
static constexpr int kReadEvent{EPOLLIN | EPOLLPRI};
static constexpr int kWriteEvent{EPOLLOUT};
}  // namespace

namespace benet {

Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd) {}

Channel::~Channel() {
  assert(!event_handing_);
  assert(!added_in_loop_);
  if (loop_->IsInLoopThread()) {
    assert(!loop_->HasChannel(this));
  }
}

void Channel::HandleEvent(TimePoint recv_time) {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();
    if (!guard) return;
  }

  event_handing_ = true;
  BELOG_TRACE("events '{}' occured in fd {}", events_as_string(revents_), fd_);
  {
    // Conduct error (errno will be set)
    if (revents_ & EPOLLERR) {
      BELOG_WARN("EPOLLERR occured in fd {}", fd_);
      if (on_error_) on_error_();
    }

    // Peer are shutdown or shutdown write
    if ((revents_ & (EPOLLHUP | EPOLLRDHUP)) && !(revents_ & EPOLLIN)) {
      if (on_close_) on_close_();
    }

    // Read Event (normal, priority, peer shutdown write)
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
      if (on_read_) on_read_(recv_time);
    }

    // Write Event
    if (revents_ & EPOLLOUT) {
      if (on_write_) on_write_();
    }
  }
  event_handing_ = false;
}

std::string Channel::events_as_string(int ev) {
  std::string str;
  if (ev & EPOLLIN) str.append("IN ");
  if (ev & EPOLLPRI) str.append("PRI ");
  if (ev & EPOLLOUT) str.append("OUT ");
  if (ev & EPOLLHUP) str.append("HUP ");
  if (ev & EPOLLRDHUP) str.append("RDHUP ");
  if (ev & EPOLLERR) str.append("ERR ");
  if (!str.empty() && str.back() == ' ') str.pop_back();
  return str;
}

void Channel::BindReadCallback(std::function<void(TimePoint)> cb) {
  on_read_ = std::move(cb);
};

void Channel::BindWriteCallback(std::function<void()> cb) {
  on_write_ = std::move(cb);
};

void Channel::BindCloseCallback(std::function<void()> cb) {
  on_close_ = std::move(cb);
};

void Channel::BindErrorCallback(std::function<void()> cb) {
  on_error_ = std::move(cb);
};

void Channel::EnableReadEvent() {
  events_ |= kReadEvent;
  update_events();
};

void Channel::DisableReadEvent() {
  events_ &= ~kReadEvent;
  update_events();
};

void Channel::EnableWriteEvent() {
  events_ |= kWriteEvent;
  update_events();
};

void Channel::DisableWriteEvent() {
  events_ &= ~kWriteEvent;
  update_events();
};

void Channel::DisableAllEvent() {
  events_ = kNoneEvent;
  update_events();
};

bool Channel::IsNoneEvent() const { return events_ == kNoneEvent; };

bool Channel::IsReadEvent() const { return events_ & kReadEvent; };

bool Channel::IsWriteEvent() const { return events_ & kWriteEvent; };

void Channel::RemoveFromLoop() {
  assert(IsNoneEvent());
  added_in_loop_ = false;
  loop_->RemoveChannel(this);
}

void Channel::update_events() {
  added_in_loop_ = true;
  loop_->UpdateChannel(this);
}

void Channel::Tie(const std::shared_ptr<void>& obj) {
  tie_ = obj;
  tied_ = true;
}

}  // namespace benet
