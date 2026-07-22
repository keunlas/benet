// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet.h"

#include <csignal>

#include "benet/logger.h"

namespace benet::init {

/// @brief trace to critical is 0-5, off is 6
void InitLogLevel(int level) {
  auto l = static_cast<spdlog::level::level_enum>(level);
  benet::Logger::AsyncConsoleLogger()->set_level(l);
}

/// @brief to ignore SIGPIPE (not thread-safe)
void IgnoreSigpipe() {
  // [TODO] use sigaction to make here thread-safe
  std::signal(SIGPIPE, SIG_IGN);
}

}  // namespace benet::init
