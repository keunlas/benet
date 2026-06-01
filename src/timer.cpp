// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/timer.h"

namespace benet {

std::atomic_uint64_t Timer::sCreatedNumber = 0;

int64_t Timer::GetCreatedNumber() { return sCreatedNumber; }

Timer::Timer(std::function<void()> callback, TimePoint when, double interval)
    : expiration_(when),
      sequence_(sCreatedNumber++),
      repeat_(interval > 0.0),
      interval_(interval),
      callback_(std::move(callback)) {}

void Timer::Run() const { callback_(); }

void Timer::Restart(TimePoint now) {
  if (repeat_) {
    auto interval_rep = static_cast<TimePoint::rep>(interval_ * 1e9);
    auto interval_nano = std::chrono::nanoseconds(interval_rep);
    expiration_ = now + interval_nano;
  } else {
    expiration_ = kInvalidTimePoint;
  }
}

bool Timer::IsRepeat() const { return repeat_; }

TimePoint Timer::expiration() const { return expiration_; }

uint64_t Timer::sequence() const { return sequence_; }

}  // namespace benet
