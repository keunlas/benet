// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/logger.h"

#include "spdlog/async.h"
#include "spdlog/async_logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace benet {

void InitAsyncLogger(spdlog::level::level_enum level) {
  spdlog::init_thread_pool(65536, 1);

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  spdlog::sinks_init_list sink_list{console_sink};
  console_sink->set_level(level);

  auto logger = std::make_shared<spdlog::async_logger>(
      "benet", sink_list, spdlog::thread_pool(),
      spdlog::async_overflow_policy::block);

  logger->set_pattern("[%Y-%m-%d %T.%f] [%P:%t] [%^%l%$]: %v [%s:%#][%!]");
  logger->set_level(level);
  spdlog::set_default_logger(logger);
}

void ShutdownLogger() {
  spdlog::default_logger()->flush();
  spdlog::shutdown();
}

}  // namespace benet
