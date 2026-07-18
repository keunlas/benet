// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_TIMER_H_
#define KEUNLAS_BENET_TIMER_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

#include "benet/copy_move_policy.h"
#include "benet/types.h"

namespace benet {

/**
 * @brief 定时器
 *
 */
class Timer : NotCopyableOrMovable {
 public:
  /**
   * @brief 构造一个新的定时器
   *
   * @param callback 定时器回调
   * @param when 定时器首次激活时间
   * @param interval 定时器再次激活的间隔（秒）
   */
  Timer(Functor callback, TimePoint when, double interval);

  /// @brief 执行定时器的回调函数
  void Run() const { callback_(); }

  /**
   * @brief 更新定时器状态，
   * 重复定时器会更新下次激活的时间，
   * 非重复定时器会失效。
   * @param now 定时器上次激活的时间
   */
  void Restart(TimePoint now);

  /// @brief 获知是否是可重复定时器
  bool IsRepeat() const { return repeat_; }

  /// @brief 定时器的激活时间
  TimePoint expiration() const { return expiration_; }

  /// @brief 获取一个无效的时刻（零时刻）
  static const TimePoint& InvalidTimePoint();

 private:
  TimePoint expiration_;    // 激活时间
  const bool repeat_;       // 是否重复激活
  const double interval_;   // 重复激活间隔
  const Functor callback_;  // 激活时回调函数
};

}  // namespace benet

#endif  // !KEUNLAS_BENET_TIMER_H_
