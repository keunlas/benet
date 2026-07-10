// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_COPY_MOVE_POLICY_H_
#define BENET_COPY_MOVE_POLICY_H_

namespace benet {

class Copyable {
 public:
  Copyable(const Copyable& other) = default;
  Copyable& operator=(const Copyable& other) = default;

  Copyable(Copyable&& other) = default;
  Copyable& operator=(Copyable&& other) = default;

 protected:
  Copyable() = default;
  ~Copyable() = default;
};

class MoveOnly {
 public:
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;

  MoveOnly(MoveOnly&& other) = default;
  MoveOnly& operator=(MoveOnly&& other) = default;

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

#endif  // !BENET_COPY_MOVE_POLICY_H_
