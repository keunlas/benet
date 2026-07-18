// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_LOGGER_H_
#define KEUNLAS_BENET_LOGGER_H_

/**
 * @brief 日志等级以 SPDLOG_ACTIVE_LEVEL 宏为准，
 * 若没有定义此宏，则
 * Debug 模式下默认为 SPDLOG_LEVEL_TRACE，
 * Release 模式下默认为 SPDLOG_LEVEL_ERROR 或 BENET_LOGGER_LEVEL_VALUE（若定义）
 *
 */
#ifndef SPDLOG_ACTIVE_LEVEL
#ifndef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#ifndef BENET_LOGGER_LEVEL_VALUE
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_ERROR
#else
#define SPDLOG_ACTIVE_LEVEL BENET_LOGGER_LEVEL_VALUE
#endif
#endif
#endif

/**
 * @brief 使用 std::format，需要 CXX20 及以上标准。
 *
 */
#ifndef SPDLOG_USE_STD_FORMAT
#define SPDLOG_USE_STD_FORMAT
#endif

#include "benet/errmsg.h"
#include "spdlog/spdlog.h"

namespace benet {
/**
 * @brief 异步日志记录使用 spdlog 默认全局线程池
 *
 * @attention 注意此 Logger 仅为 benet 库使用，
 * 因为其日志记录器的名称已经固定为 benet。
 */
class Logger {
 public:
  /// @brief 异步控制台日志记录器
  static std::shared_ptr<spdlog::logger> AsyncConsoleLogger();
};
}  // namespace benet

/// @brief 当前使用的日志记录器
#define BENET_ACTIVE_LOGGER benet::Logger::AsyncConsoleLogger()

/// @brief 日志记录宏 TRACE
#define BELOG_TRACE(...) SPDLOG_LOGGER_TRACE(BENET_ACTIVE_LOGGER, __VA_ARGS__)

/// @brief 日志记录宏 DEBUG
#define BELOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(BENET_ACTIVE_LOGGER, __VA_ARGS__)

/// @brief 日志记录宏 INFO
#define BELOG_INFO(...) SPDLOG_LOGGER_INFO(BENET_ACTIVE_LOGGER, __VA_ARGS__)

/// @brief 日志记录宏 WARN
#define BELOG_WARN(...) SPDLOG_LOGGER_WARN(BENET_ACTIVE_LOGGER, __VA_ARGS__)

/// @brief 日志记录宏 ERROR
#define BELOG_ERROR(...) SPDLOG_LOGGER_ERROR(BENET_ACTIVE_LOGGER, __VA_ARGS__)

/// @brief 日志记录宏 CRITICAL
#define BELOG_CRITICAL(...)                                 \
  SPDLOG_LOGGER_CRITICAL(BENET_ACTIVE_LOGGER, __VA_ARGS__); \
  spdlog::shutdown();                                       \
  std::exit(2);

#endif  // !KEUNLAS_BENET_LOGGER_H_
