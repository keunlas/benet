// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_TIMESTAMP_H_
#define BENET_TIMESTAMP_H_

#include <chrono>

namespace benet {

using TimeClock = std::chrono::system_clock;
using TimePoint = TimeClock::time_point;

static constexpr TimePoint kInvalidTimePoint{TimePoint::duration(0)};

}  // namespace benet

#endif  // !BENET_TIMESTAMP_H_
