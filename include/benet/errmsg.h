// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_ERRMSG_H_
#define KEUNLAS_BENET_ERRMSG_H_

#include <cerrno>
#include <system_error>

#define ERRCODE(n) std::error_code(n, std::generic_category())
#define ERRCODE_MSG(n) ERRCODE(n).message()
#define ERRNO_MSG ERRCODE_MSG(errno)

#endif  // !KEUNLAS_BENET_ERRMSG_H_
