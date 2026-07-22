// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_H_
#define KEUNLAS_BENET_H_

namespace benet::init {

/// @brief trace to critical is 0-5, off is 6
void InitLogLevel(int level);

/// @brief to ignore SIGPIPE
void IgnoreSigpipe();

}  // namespace benet::init

#endif  // !KEUNLAS_BENET_H_
