// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/timer.h"

namespace benet {

Timer::Timer(Functor callback, TimePoint when, double interval)
    : expiration_(when),
      repeat_(interval > 0.0),
      interval_(interval),
      callback_(std::move(callback)) {}

void Timer::Restart(TimePoint now) {
  if (repeat_) {
    auto interval_rep = static_cast<TimePoint::rep>(interval_ * 1e9);
    auto interval_nano = std::chrono::nanoseconds(interval_rep);
    expiration_ = now + interval_nano;
  } else {
    expiration_ = InvalidTimePoint();
  }
}

const TimePoint& Timer::InvalidTimePoint() {
  static TimePoint zero{TimePoint::duration::zero()};
  return zero;
}

}  // namespace benet
