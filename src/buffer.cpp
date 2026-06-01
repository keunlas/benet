// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#include "benet/buffer.h"

#include <sys/uio.h>
#include <unistd.h>

#include <array>

namespace {
static constexpr size_t kExtraBufSize = 1 * 1024 * 1024;
}  // namespace

namespace benet {

ssize_t Buffer::ReadFd(int fd) {
  std::unique_ptr<std::array<char, kExtraBufSize>> extra_buf;
  size_t writable = WritableBytes();

  int iovcnt = 1;
  if (writable < kExtraBufSize) {
    extra_buf = std::make_unique<std::array<char, kExtraBufSize>>();
    iovcnt = 2;
  }

  std::array<iovec, 2> buffers;
  buffers[0].iov_base = begin_write();
  buffers[0].iov_len = writable;
  if (iovcnt > 1) {
    buffers[1].iov_base = extra_buf->data();
    buffers[1].iov_len = extra_buf->size();
  }

  const ssize_t n = ::readv(fd, buffers.data(), iovcnt);

  if (n >= 0 && n <= static_cast<ssize_t>(writable)) {
    writer_index_ += n;
  } else {
    writer_index_ = buffer_.size();
    Append(extra_buf->data(), n - writable);
  }

  return n;
}

}  // namespace benet
