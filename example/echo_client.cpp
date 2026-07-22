// echo_client.cpp - ASIO 压力测试客户端

#include <asio.hpp>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>

/// 每条消息的字节数。
std::atomic<size_t> g_msg_size{1024 * 64};

/// 设置为 true 可优雅停止客户端。
std::atomic<bool> g_stop{false};

// ============================================================

using namespace std::chrono_literals;

class EchoClient : public std::enable_shared_from_this<EchoClient> {
 public:
  EchoClient(asio::io_context& io, const asio::ip::tcp::endpoint& ep)
      : io_(io), socket_(io), endpoint_(ep), stats_timer_(io) {}

  void Start() {
    socket_.async_connect(
        endpoint_, [self = shared_from_this()](std::error_code ec) {
          if (ec) {
            std::cerr << "[client] connect error: " << ec.message()
                      << std::endl;
            return;
          }
          std::cout << "[client] connected to " << self->endpoint_ << std::endl;
          self->DoRead();
          self->DoSend();
          self->DoStats();
        });
  }

 private:
  // ---- 读取：丢弃所有回显数据 ----
  void DoRead() {
    auto buf = std::make_shared<std::array<char, 65536>>();
    socket_.async_read_some(
        asio::buffer(*buf),
        [self = shared_from_this(), buf](std::error_code ec, size_t len) {
          if (ec) {
            if (ec != asio::error::eof && ec != asio::error::operation_aborted)
              std::cerr << "[client] read error: " << ec.message() << std::endl;
            return;
          }
          // 丢弃数据，仅计数
          self->recv_bytes_.fetch_add(len);
          self->recv_msgs_.fetch_add(1);
          self->DoRead();
        });
  }

  // 发送
  void DoSend() {
    if (g_stop.load()) return;
    asio::async_write(
        socket_, asio::buffer(*data_),
        [self = shared_from_this()](std::error_code ec, size_t len) {
          if (ec) {
            if (ec != asio::error::operation_aborted)
              std::cerr << "[client] write error: " << ec.message()
                        << std::endl;
            return;
          }
          self->send_bytes_.fetch_add(len);
          self->send_msgs_.fetch_add(1);
          self->DoSend();
        });
  }

  // ---- 统计输出 ----
  void DoStats() {
    stats_timer_.expires_after(1s);
    stats_timer_.async_wait([self = shared_from_this()](std::error_code ec) {
      if (ec || g_stop.load()) return;
      auto sb = self->send_bytes_.exchange(0);
      auto sm = self->send_msgs_.exchange(0);
      auto rb = self->recv_bytes_.exchange(0);
      auto rm = self->recv_msgs_.exchange(0);

      std::cout << std::format(
          "[TX] {:.3f} MiB/s {:.3f} KiMsgs/s | "
          "[RX]  {:.3f} MiB/s {:.3f} KiMsgs/s\n",
          sb / 1024.0 / 1024.0, sm / 1024.0, rb / 1024.0 / 1024.0, rm / 1024.0);

      self->DoStats();
    });
  }

  asio::io_context& io_;
  asio::ip::tcp::socket socket_;
  asio::ip::tcp::endpoint endpoint_;
  asio::steady_timer stats_timer_;

  std::shared_ptr<std::string> data_{
      std::make_shared<std::string>(g_msg_size, 'X')};

  std::atomic<uint64_t> send_bytes_{0};
  std::atomic<uint64_t> send_msgs_{0};
  std::atomic<uint64_t> recv_bytes_{0};
  std::atomic<uint64_t> recv_msgs_{0};
};

int main() {
  asio::io_context io;

  // 连接本地 echo_server (默认监听 0.0.0.0:8080)
  asio::ip::tcp::endpoint ep(asio::ip::make_address("127.0.0.1"), 8080);

  auto client = std::make_shared<EchoClient>(io, ep);
  client->Start();

  std::cout << "[client] starting io_context..." << std::endl;
  io.run();

  return 0;
}
