// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_CALLBACKS_H_
#define BENET_CALLBACKS_H_

#include <functional>
#include <memory>

#include "benet/buffer.h"
#include "benet/timestamp.h"

namespace benet {

class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, TimePoint)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;

extern void default_connection_callback(const TcpConnectionPtr& conn);
extern void default_message_callback(const TcpConnectionPtr& conn, Buffer* buffer, TimePoint receive_time);

}  // namespace benet

#endif  // !BENET_CALLBACKS_H_