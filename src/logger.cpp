// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/logger.h"

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <mutex>
#include <string>

/// @brief 日志记录器名称
#define BENET_LOGGER_NAME "benet"

/// @brief 日志记录样式
#ifndef NDEBUG  // Debug: [time] [name] [level] [filename:line func]: msg
#define BENET_LOGGER_PATTERN "[%Y-%m-%d %T.%f] [%n] [%^%l%$] [%s:%# %!]: %v"
#else  // Release: [time] [name] [level]: msg
#define BENET_LOGGER_PATTERN "[%Y-%m-%d %T.%f] [%n] [%^%l%$]: %v"
#endif

/// @brief 异步控制台日志记录器
std::shared_ptr<spdlog::logger> benet::Logger::AsyncConsoleLogger() {
  static std::shared_ptr<spdlog::logger> logger = [](const char* name) {
    auto l = spdlog::create_async<spdlog::sinks::stdout_color_sink_mt>(name);
    l->set_pattern(BENET_LOGGER_PATTERN);
    l->set_level(spdlog::level::trace);
    l->flush_on(spdlog::level::err);
    return l;
  }(BENET_LOGGER_NAME);
  return logger;
}
