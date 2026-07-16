// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/benet
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef KEUNLAS_BENET_CALLBACKS_H_
#define KEUNLAS_BENET_CALLBACKS_H_

#include <functional>
#include <memory>

#include "benet/types.h"

namespace benet {
class Buffer;
class EventLoop;
class InetAddress;
}  // namespace benet

namespace benet {

/// @brief Acceptor 用于处理新连接的 connfd 的回调函数类型
using NewConnCallback =
    std::function<void(int /* connfd */, const InetAddress& /* addr */)>;

/// @brief EventLoopThread 用于初始化的回调函数类型
using ThreadInitCallback = std::function<void(EventLoop*)>;

// class TcpConnection;
// using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

// using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
// using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
// using MessageCallback =
//     std::function<void(const TcpConnectionPtr&, Buffer*, TimePoint)>;
// using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
// using HighWaterMarkCallback =
//     std::function<void(const TcpConnectionPtr&, size_t)>;

// extern void default_connection_callback(const TcpConnectionPtr& conn);
// extern void default_message_callback(const TcpConnectionPtr& conn,
//                                      Buffer* buffer, TimePoint receive_time);

}  // namespace benet

#endif  // !KEUNLAS_BENET_CALLBACKS_H_