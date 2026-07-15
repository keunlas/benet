// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_TIMER_H_
#define BENET_TIMER_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

#include "benet/copy_move_policy.h"
#include "benet/timestamp.h"

namespace benet {

class Timer : NotCopyableOrMovable {
 public:
  Timer(std::function<void()> callback, TimePoint when, double interval);

  void Run() const;
  void Restart(TimePoint now);

  bool IsRepeat() const;

  TimePoint expiration() const;
  uint64_t sequence() const;

  static int64_t GetCreatedNumber();

 private:
  static std::atomic_uint64_t sCreatedNumber;

 private:
  TimePoint expiration_;
  const uint64_t sequence_;

  const bool repeat_;
  const double interval_;

  const std::function<void()> callback_;
};

}  // namespace benet

#endif  // !BENET_TIMER_H_
