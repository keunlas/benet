// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_CHANNEL_H_
#define KEUNLAS_BENET_CHANNEL_H_

#include <functional>
#include <memory>
#include <string>

#include "benet/copy_move_policy.h"
#include "benet/types.h"

namespace benet {
class EventLoop;
}  // namespace benet

namespace benet {

/**
 * @brief 通道，用于封装 fd 并处理其事件
 *
 */
class Channel : NotCopyableOrMovable {
 public:
  Channel(EventLoop* loop, int fd);
  ~Channel();

  void HandleEvent(TimePoint recv_time);

  inline const EventLoop* loop() const { return loop_; }

  inline const int fd() const { return fd_; }
  inline const int events() const { return events_; }

  inline const int index() const { return index_; }
  inline void set_index(int index) { index_ = index; }

  inline const int revents() const { return revents_; }
  inline void set_revents(int revt) { revents_ = revt; }

  void EnableReadEvent();
  void DisableReadEvent();
  void EnableWriteEvent();
  void DisableWriteEvent();
  void DisableAllEvent();

  bool IsNoneEvent() const;
  bool IsReadEvent() const;
  bool IsWriteEvent() const;

  void BindReadCallback(std::function<void(TimePoint)> cb);
  void BindWriteCallback(std::function<void()> cb);
  void BindCloseCallback(std::function<void()> cb);
  void BindErrorCallback(std::function<void()> cb);

  void RemoveFromLoop();

  void Tie(const std::shared_ptr<void>& obj);

 private:
  void update_events();
  std::string events_as_string(int ev);

 private:
  EventLoop* loop_;
  const int fd_;
  int events_{0};
  int revents_{0};
  int index_{-1};

  bool tied_{false};
  std::weak_ptr<void> tie_{};

  bool event_handing_;
  bool added_in_loop_;

  std::function<void(TimePoint)> on_read_;
  std::function<void()> on_write_;
  std::function<void()> on_close_;
  std::function<void()> on_error_;
};

}  // namespace benet

#endif  // !KEUNLAS_BENET_CHANNEL_H_
