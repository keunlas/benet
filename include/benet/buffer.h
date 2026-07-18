// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_BUFFER_H
#define KEUNLAS_BENET_BUFFER_H

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

#include "benet/copy_move_policy.h"

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
  static constexpr size_t kCheapPrepend = 8;      // 8Bytes
  static constexpr size_t kInitialSize = 1024;    // 1KiB
  static constexpr size_t kExtraBufSize = 65536;  // 64KiB
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

  template <typename T>
  inline T PeekInt() const {
    static_assert(std::is_integral_v<T>, "T must be integral type");
    assert(ReadableBytes() >= sizeof(T));
    std::make_unsigned_t<T> val = 0;
    std::memcpy(&val, Peek(), sizeof(T));
    if constexpr (sizeof(T) == 8) {
      return static_cast<T>(be64toh(val));
    } else if constexpr (sizeof(T) == 4) {
      return static_cast<T>(be32toh(val));
    } else if constexpr (sizeof(T) == 2) {
      return static_cast<T>(be16toh(val));
    } else if constexpr (sizeof(T) == 1) {
      return static_cast<T>(val);
    } else {
      static_assert(sizeof(T) == 0, "Unsupported size");
    }
  }

  inline auto PeekInt64() const { return PeekInt<int64_t>(); }
  inline auto PeekInt32() const { return PeekInt<int32_t>(); }
  inline auto PeekInt16() const { return PeekInt<int16_t>(); }
  inline auto PeekInt8() const { return PeekInt<int8_t>(); }

  inline const char* FindCRLF() const {
    auto crlf = reinterpret_cast<const char*>(
        ::memmem(Peek(), ReadableBytes(), kCRLF, 2));
    return crlf;
  }

  inline const char* FindCRLF(const char* start) const {
    assert(Peek() <= start);
    assert(start <= begin_write());
    auto crlf = reinterpret_cast<const char*>(
        ::memmem(start, begin_write() - start, kCRLF, 2));
    return crlf;
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
    assert(ReadableBytes() >= len);
    reader_index_ += len;
  }

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
    assert(ReadableBytes() >= len);
    std::string result(Peek(), len);
    Retrieve(len);
    return result;
  }

  inline std::string RetrieveAllAsString() {
    return RetrieveAsString(ReadableBytes());
  }

  template <typename T>
  inline T RetrieveAsInt() {
    static_assert(std::is_integral_v<T>, "T must be integral type");
    T result = PeekInt<T>();
    Retrieve(sizeof(T));
    return result;
  }

  inline auto RetrieveAsInt64() { return RetrieveAsInt<int64_t>(); }
  inline auto RetrieveAsInt32() { return RetrieveAsInt<int32_t>(); }
  inline auto RetrieveAsInt16() { return RetrieveAsInt<int16_t>(); }
  inline auto RetrieveAsInt8() { return RetrieveAsInt<int8_t>(); }

  // increase buffer size if writable bytes less than LEN.
  inline void EnsureWriteableBytes(size_t len) {
    if (len > WritableBytes()) make_space(len);
  }

  inline void Append(std::string_view view) {
    Append(view.data(), view.size());
  }

  inline void Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
  }

  inline void Append(const char* data, size_t len) {
    EnsureWriteableBytes(len);
    std::copy(data, data + len, begin_write());
    writer_index_ += len;
  }

  template <typename T>
  inline void AppendInt(T num) {
    static_assert(std::is_integral_v<T>, "T must be integral type");
    if constexpr (sizeof(T) == 8) {
      num = htobe64(num);
    } else if constexpr (sizeof(T) == 4) {
      num = htobe32(num);
    } else if constexpr (sizeof(T) == 2) {
      num = htobe16(num);
    } else if constexpr (sizeof(T) == 1) {
      (void)num;
    } else {
      static_assert(sizeof(T) == 0, "Unsupported size");
    }
    Append(&num, sizeof(T));
  }

  inline void AppendInt64(int64_t x) { AppendInt<int64_t>(x); }
  inline void AppendInt32(int32_t x) { AppendInt<int32_t>(x); }
  inline void AppendInt16(int16_t x) { AppendInt<int16_t>(x); }
  inline void AppendInt8(int8_t x) { AppendInt<int8_t>(x); }

  inline void Prepend(std::string_view view) {
    Prepend(view.data(), view.size());
  }

  inline void Prepend(const void* data, size_t len) {
    Prepend(static_cast<const char*>(data), len);
  }

  inline void Prepend(const char* data, size_t len) {
    assert(len <= PrependableBytes());
    std::copy_backward(data, data + len, begin_read());
    reader_index_ -= len;
  }

  template <typename T>
  inline void PrependInt(T num) {
    static_assert(std::is_integral_v<T>, "T must be integral type");
    if constexpr (sizeof(T) == 8) {
      num = htobe64(num);
    } else if constexpr (sizeof(T) == 4) {
      num = htobe32(num);
    } else if constexpr (sizeof(T) == 2) {
      num = htobe16(num);
    } else if constexpr (sizeof(T) == 1) {
      (void)num;
    } else {
      static_assert(sizeof(T) == 0, "Unsupported size");
    }
    Prepend(&num, sizeof(T));
  }

  inline void PrependInt64(int64_t x) { PrependInt<int64_t>(x); }
  inline void PrependInt32(int32_t x) { PrependInt<int32_t>(x); }
  inline void PrependInt16(int16_t x) { PrependInt<int16_t>(x); }
  inline void PrependInt8(int8_t x) { PrependInt<int8_t>(x); }

  // read data from FD to buffer.
  ssize_t ReadFd(int fd) {
    std::array<iovec, 2> buffers;

    const size_t writable = WritableBytes();
    buffers[0].iov_base = begin_write();
    buffers[0].iov_len = writable;

    std::array<char, kExtraBufSize> extra_buf;
    buffers[1].iov_base = extra_buf.data();
    buffers[1].iov_len = extra_buf.size();

    const int iovcnt = (writable < kExtraBufSize) ? 2 : 1;
    const ssize_t n = ::readv(fd, buffers.data(), iovcnt);

    if (n >= 0 && n <= static_cast<ssize_t>(writable)) {
      writer_index_ += n;
    } else if (n > static_cast<ssize_t>(writable)) {
      writer_index_ = buffer_.size();
      Append(extra_buf.data(), n - writable);
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
      std::copy(begin_read(), begin_write(), begin() + kCheapPrepend);
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

#endif  // !KEUNLAS_BENET_BUFFER_H
