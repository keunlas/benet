// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_COPY_MOVE_TYPE_H_
#define BENET_COPY_MOVE_TYPE_H_

namespace benet {

class Copyable {
 public:
  Copyable(const Copyable& other) = default;
  Copyable& operator=(const Copyable& other) = default;

 protected:
  Copyable() = default;
  ~Copyable() = default;
};

class MoveOnly {
 public:
  MoveOnly(MoveOnly&& other) = default;
  MoveOnly& operator=(MoveOnly&& other) = default;

  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;

 protected:
  MoveOnly() = default;
  ~MoveOnly() = default;
};

class NotCopyableOrMovable {
 public:
  NotCopyableOrMovable(const NotCopyableOrMovable&) = delete;
  NotCopyableOrMovable& operator=(NotCopyableOrMovable&) = delete;

  NotCopyableOrMovable(NotCopyableOrMovable&&) = delete;
  NotCopyableOrMovable& operator=(NotCopyableOrMovable&&) = delete;

 protected:
  NotCopyableOrMovable() = default;
  ~NotCopyableOrMovable() = default;
};

}  // namespace benet

#endif  // !BENET_COPY_MOVE_TYPE_H_
