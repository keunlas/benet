// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_LOGGER_H_
#define BENET_LOGGER_H_

#include <cerrno>
#include <system_error>

#define ERRNO_MSG std::error_code(errno, std::generic_category()).message()
#define ERRCODE_MSG(code) std::error_code(code, std::generic_category()).message()

#ifndef BE_NOT_USE_SPDLOG

#ifndef SPDLOG_ACTIVE_LEVEL
# ifndef NDEBUG
#   define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
# else
#   ifndef BE_LOGGER_LEVEL_VALUE
#     define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_ERROR
#   else
#     define SPDLOG_ACTIVE_LEVEL BE_LOGGER_LEVEL_VALUE
#   endif
# endif
#endif

#include "spdlog/spdlog.h"

#else

namespace spdlog {
namespace level {
enum level_enum : int {
  trace = 0,
  debug = 1,
  info = 2,
  warn = 3,
  err = 4,
  critical = 5,
  off = 6,
  n_levels
};
}  // namespace level
}  // namespace spdlog

#endif  // !BE_NOT_USE_SPDLOG

namespace benet {

void InitAsyncLogger(spdlog::level::level_enum level = 
#ifndef BE_NOT_USE_SPDLOG
  spdlog::level::trace
#else
  spdlog::level::err
#endif
);

void ShutdownLogger();

}  // namespace benet

#ifndef BE_NOT_USE_SPDLOG

#define BELOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define BELOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define BELOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define BELOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define BELOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define BELOG_CRITICAL(...)     \
  SPDLOG_CRITICAL(__VA_ARGS__); \
  benet::ShutdownLogger();      \
  std::exit(2);

#endif  // !BE_NOT_USE_SPDLOG

#endif  // !BENET_LOGGER_H_
