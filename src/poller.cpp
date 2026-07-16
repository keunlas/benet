// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/poller.h"

#include "benet/channel.h"
#include "benet/eventloop.h"
#include "benet/logger.h"

namespace {
static constexpr int kEventsSize{32};
static constexpr int kNewChannel = -1;
static constexpr int kAddedChannel = 1;
static constexpr int kDeletedChannel = 2;
}  // namespace

namespace benet::details {
int create_epollfd() {
  auto epollfd = ::epoll_create1(EPOLL_CLOEXEC);
  if (epollfd < 0) {
    BELOG_CRITICAL("Failed to create epollfd: {}", ERRNO_MSG);
  }
  return epollfd;
}
void close_epollfd(int epollfd) { ::close(epollfd); }
}  // namespace benet::details

namespace benet {

EPoller::EPoller(EventLoop* loop)
    : Poller(loop),
      epoll_fd_(details::create_epollfd()),
      events_(kEventsSize) {}

EPoller::~EPoller() { details::close_epollfd(epoll_fd_); }

TimePoint EPoller::Poll(int timeout_ms, std::vector<Channel*>& actives) {
  BELOG_TRACE("Current fd counts is {}", channels_.size());

  int n_readys = ::epoll_wait(epoll_fd_, events_.data(),
                              static_cast<int>(events_.size()), timeout_ms);

  auto now = TimeClock::now();

  if (n_readys > 0) {
    BELOG_TRACE("EPoller polled {} events", n_readys);
    fill_actives(n_readys, actives);
    if (events_.size() == static_cast<size_t>(n_readys)) {
      events_.resize(static_cast<size_t>(1.5 * n_readys));
    }
  } else if (n_readys == 0) {
    BELOG_TRACE("EPoller timeout, nothing happened");
  } else {
    if (errno != EINTR) {
      BELOG_CRITICAL("Failed to epoll wait: {}", ERRNO_MSG);
    }
  }

  return now;
}

void EPoller::fill_actives(int n_events, std::vector<Channel*>& actives) const {
  for (int i = 0; i < n_events; ++i) {
    auto channel = static_cast<Channel*>(events_[i].data.ptr);
    channel->set_revents(events_[i].events);
    actives.push_back(channel);
  }
}

void EPoller::UpdateChannel(Channel* channel) {
  const int fd = channel->fd();
  const int index = channel->index();
  BELOG_TRACE("Update channel fd {}, events {}, index {}", fd,
              channel->events(), index);

  // add channel to ChannelMap.
  if (index == kNewChannel || index == kDeletedChannel) {
    if (index == kNewChannel) {
      channels_[fd] = channel;
    }
    update_epoll_operation(EPOLL_CTL_ADD, channel);
    channel->set_index(kAddedChannel);
  }
  // channel already in ChannelMap, update it.
  else {
    if (channel->IsNoneEvent()) {
      update_epoll_operation(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeletedChannel);
    } else {
      update_epoll_operation(EPOLL_CTL_MOD, channel);
    }
  }
}

void EPoller::RemoveChannel(Channel* channel) {
  const int fd = channel->fd();
  const int index = channel->index();
  BELOG_TRACE("Remove channel fd {}, index {}", fd, index);

  channels_.erase(fd);
  if (index == kAddedChannel) {
    update_epoll_operation(EPOLL_CTL_DEL, channel);
  }

  channel->set_index(kNewChannel);
}

void EPoller::update_epoll_operation(int operation, Channel* channel) {
  epoll_event event;
  std::memset(&event, 0, sizeof(event));
  event.events = channel->events();
  event.data.ptr = channel;

  int fd = channel->fd();
  int ret = ::epoll_ctl(epoll_fd_, operation, fd, &event);

  if (ret < 0) {
    if (operation == EPOLL_CTL_DEL) {
      BELOG_ERROR("Failed to EPOLL_CTL_DEL fd {}: {}", fd, ERRNO_MSG);
    } else {
      BELOG_CRITICAL("Failed to EPOLL_CTL_ADD/MOD fd {}: {}", fd, ERRNO_MSG);
    }
  }
}

bool Poller ::HasChannel(Channel* channel) const {
  auto it = channels_.find(channel->fd());
  return it != channels_.end() && it->second == channel;
}

void Poller::AssertInLoopThread() const { loop_->AssertInLoopThread(); }

}  // namespace benet
