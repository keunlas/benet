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
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
}  // namespace benet

namespace benet {

/// @brief Acceptor 用于处理新连接的 connfd 的回调函数类型
using NewConnCallback =
    std::function<void(int /* connfd */, const InetAddress& /* addr */)>;

/// @brief EventLoopThread 用于初始化的回调函数类型
using ThreadInitCallback = std::function<void(EventLoop* /* eventloop */)>;

/// @brief TcpConnection 连接建立或准备关闭时回调函数
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;

/// @brief TcpConnection 连接关闭回调
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

/// @brief TcpConnection 消息读回调
using MessageCallback =
    std::function<void(const TcpConnectionPtr&, Buffer*, TimePoint)>;

/// @brief TcpConnection 消息写完成回调
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

/// @brief TcpConnection 高水位回调
using HighWaterMarkCallback =
    std::function<void(const TcpConnectionPtr&, size_t)>;

}  // namespace benet

#endif  // !KEUNLAS_BENET_CALLBACKS_H_
