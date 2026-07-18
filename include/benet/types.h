// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_TYPES_H_
#define KEUNLAS_BENET_TYPES_H_

#include <chrono>
#include <functional>

namespace benet {

/// @brief 函数调用子
using Functor = std::function<void()>;

/// @brief 时钟
using TimeClock = std::chrono::system_clock;

/// @brief 时刻
using TimePoint = TimeClock::time_point;

}  // namespace benet

#endif  // !KEUNLAS_BENET_TYPES_H_
