// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_BUFFER_H
#define BENET_BUFFER_H

#include <endian.h>
#include <sys/uio.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "benet/copy_move_type.h"

#ifndef BENET_BUFFER_INIT_SIZE
#define BENET_BUFFER_INIT_SIZE (1024)  // default is 1 KiB
#endif

#ifndef BENET_BUFFER_EXTRA_SIZE
#define BENET_BUFFER_EXTRA_SIZE (1024 * 1024 * 1)  // default is 1 MiB
#endif

#ifndef BENET_BUFFER_CHEAP_PREPEND_SIZE
#define BENET_BUFFER_CHEAP_PREPEND_SIZE (8)  // default is 8 Bytes
#endif

namespace benet {

///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=     reader_index_  <=  writer_index_  <=  buffer_.size()
/// @endcode
///

class Buffer : Copyable {
 public:
  static constexpr size_t kCheapPrepend = BENET_BUFFER_CHEAP_PREPEND_SIZE;
  static constexpr size_t kInitialSize = BENET_BUFFER_INIT_SIZE;
  static constexpr size_t kExtraBufSize = BENET_BUFFER_EXTRA_SIZE;
  static constexpr auto kCRLF = "\r\n";

 public:
  explicit Buffer(size_t init_size = kInitialSize)
      : buffer_(kCheapPrepend + init_size),
        reader_index_(kCheapPrepend),
        writer_index_(kCheapPrepend) {}

  inline size_t ReadableBytes() const { return writer_index_ - reader_index_; }
  inline size_t WritableBytes() const { return buffer_.size() - writer_index_; }
  inline size_t PrependableBytes() const { return reader_index_; }

  inline const char* Peek() const { return begin_read(); }

  inline int64_t PeekInt64() const {
    assert(ReadableBytes() >= sizeof(int64_t));
    int64_t be64 = 0;
    std::memcpy(&be64, Peek(), sizeof(be64));
    return be64toh(be64);
  }

  inline int32_t PeekInt32() const {
    assert(ReadableBytes() >= sizeof(int32_t));
    int32_t be32 = 0;
    std::memcpy(&be32, Peek(), sizeof(be32));
    return be32toh(be32);
  }

  inline int16_t PeekInt16() const {
    assert(ReadableBytes() >= sizeof(int16_t));
    int16_t be16 = 0;
    std::memcpy(&be16, Peek(), sizeof(be16));
    return be16toh(be16);
  }

  inline int8_t PeekInt8() const {
    assert(ReadableBytes() >= sizeof(int8_t));
    int8_t x = *Peek();
    return x;
  }

  inline const char* FindCRLF() const {
    auto crlf = reinterpret_cast<const char*>(
        ::memmem(Peek(), ReadableBytes(), kCRLF, 2));
    return crlf == begin_write() ? nullptr : crlf;
  }

  inline const char* FindCRLF(const char* start) const {
    assert(Peek() <= start);
    assert(start <= begin_write());
    auto crlf = reinterpret_cast<const char*>(
        ::memmem(start, ReadableBytes(), kCRLF, 2));
    return crlf == begin_write() ? nullptr : crlf;
  }

  inline const char* FindEOL() const {
    const void* eol = std::memchr(Peek(), '\n', ReadableBytes());
    return static_cast<const char*>(eol);
  }

  inline const char* FindEOL(const char* start) const {
    assert(Peek() <= start);
    assert(start <= begin_write());
    const void* eol = std::memchr(start, '\n', begin_write() - start);
    return static_cast<const char*>(eol);
  }

  inline void Retrieve(size_t len) {
    assert(len >= ReadableBytes());
    reader_index_ += len;
  }

  inline void RetrieveInt64() { Retrieve(sizeof(int64_t)); }
  inline void RetrieveInt32() { Retrieve(sizeof(int32_t)); }
  inline void RetrieveInt16() { Retrieve(sizeof(int16_t)); }
  inline void RetrieveInt8() { Retrieve(sizeof(int8_t)); }

  inline void RetrieveUntil(const char* end) {
    assert(Peek() <= end);
    assert(end <= begin_write());
    Retrieve(end - Peek());
  }

  inline void RetrieveAll() {
    reader_index_ = kCheapPrepend;
    writer_index_ = kCheapPrepend;
  }

  inline std::string RetrieveAsString(size_t len) {
    assert(len >= ReadableBytes());
    std::string result(Peek(), len);
    Retrieve(len);
    return result;
  }

  inline std::string RetrieveAllAsString() {
    return RetrieveAsString(ReadableBytes());
  }

  inline int64_t RetrieveAsInt64() {
    int64_t result = PeekInt64();
    RetrieveInt64();
    return result;
  }

  inline int32_t RetrieveAsInt32() {
    int32_t result = PeekInt32();
    RetrieveInt32();
    return result;
  }

  inline int16_t RetrieveAsInt16() {
    int16_t result = PeekInt16();
    RetrieveInt16();
    return result;
  }

  inline int8_t RetrieveAsInt8() {
    int8_t result = PeekInt8();
    RetrieveInt8();
    return result;
  }

  // increase buffer size if writable bytes less than LEN.
  inline void EnsureWriteableBytes(size_t len) {
    if (len > WritableBytes()) {
      make_space(len);
    }
  }

  inline void Append(const std::string_view& sv) {
    Append(sv.data(), sv.size());
  }

  inline void Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
  }

  inline void Append(const char* data, size_t len) {
    EnsureWriteableBytes(len);
    std::copy(data, data + len, begin_write());
    writer_index_ += len;
  }

  inline void AppendInt64(int64_t x) {
    int64_t be64 = htobe64(x);
    Append(&be64, sizeof(be64));
  }

  inline void AppendInt32(int32_t x) {
    int32_t be32 = htobe32(x);
    Append(&be32, sizeof(be32));
  }

  inline void AppendInt16(int16_t x) {
    int16_t be16 = htobe16(x);
    Append(&be16, sizeof(be16));
  }

  inline void AppendInt8(int8_t x) { Append(&x, sizeof(x)); }

  inline void Prepend(const void* data, size_t len) {
    assert(len <= PrependableBytes());
    const char* p = static_cast<const char*>(data);
    auto t = begin_read() - len;
    std::copy(p, p + len, t);
    reader_index_ -= len;
  }

  inline void Prepend(const std::string_view& sv) {
    Prepend(sv.data(), sv.size());
  }

  // read data from FD to buffer.
  ssize_t ReadFd(int fd) {
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

 private:
  inline char* begin() { return buffer_.data(); }
  inline const char* begin() const { return buffer_.data(); }
  inline char* begin_write() { return begin() + writer_index_; }
  inline const char* begin_write() const { return begin() + writer_index_; }
  inline char* begin_read() { return begin() + reader_index_; }
  inline const char* begin_read() const { return begin() + reader_index_; }

  inline void make_space(size_t len) {
    if (WritableBytes() + PrependableBytes() < len + kCheapPrepend) {
      /* make space outside */
      buffer_.resize(writer_index_ + len);
    } else {
      /* make space inside */
      size_t readable = ReadableBytes();
      std::copy(begin() + reader_index_, begin() + writer_index_,
                begin() + kCheapPrepend);
      reader_index_ = kCheapPrepend;
      writer_index_ = reader_index_ + readable;
    }
  }

 private:
  std::vector<char> buffer_;
  size_t reader_index_;
  size_t writer_index_;
};

}  // namespace benet

#endif  // !BENET_BUFFER_H
