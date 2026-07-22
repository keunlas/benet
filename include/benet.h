// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_H_
#define KEUNLAS_BENET_H_

namespace benet::init {

/// @brief trace-0 debug-1 info-2 watn-3 error-4 critical-5 off-6
void InitLogLevel(int level);

/// @brief to ignore SIGPIPE (not thread-safe)
void IgnoreSigpipe();

}  // namespace benet::init

#endif  // !KEUNLAS_BENET_H_
