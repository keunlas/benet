#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
//
#include "benet/logger.h"
#include "benet/tcp_server.h"
//
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace benet;

void InitBenet() {
  benet::Logger::AsyncConsoleLogger()->set_level(spdlog::level::warn);
}

class EchoServer {
 public:
  EchoServer(EventLoop* loop, const InetAddress& addr)
      : server_(loop, addr, false, "Echo"),
        logger_(
            spdlog::create_async<spdlog::sinks::stdout_color_sink_mt>("echo")) {
    logger_->set_level(spdlog::level::trace);

    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    server_.BindConnectionCallback(
        std::bind(&EchoServer::on_connection, this, _1));
    server_.BindMessageCallback(
        std::bind(&EchoServer::on_message, this, _1, _2, _3));

    server_.loop()->RunEvery(5,
                             std::bind(&EchoServer::print_throughput, this));
  }

  void Start(int n_threads) {
    SPDLOG_LOGGER_DEBUG(logger_, "EchoServer start with {} threads", n_threads);
    server_.InitThreadsNumber(n_threads);
    last_print_time_ = TimeClock::now();
    server_.Start();
  }

 private:
  void on_connection(const TcpConnectionPtr& conn) {
    if (conn->IsConnected()) {
      SPDLOG_LOGGER_DEBUG(logger_, "Connection {} is Connected",
                          conn->AsString());
    } else {
      SPDLOG_LOGGER_DEBUG(logger_, "Connection {} is Disconnected",
                          conn->AsString());
    }
  }

  void on_message(const TcpConnectionPtr& conn, Buffer* buf, TimePoint) {
    receive_msgs_.fetch_add(1);
    receive_bytes_.fetch_add(buf->ReadableBytes());
    conn->Send(buf);
  }

  void print_throughput() {
    auto now = TimeClock::now();
    auto bytes = receive_bytes_.exchange(0);
    auto msgs = receive_msgs_.exchange(0);
    auto time_gap = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_print_time_);

    SPDLOG_LOGGER_INFO(
        logger_, "{:.3f} MiB/s, {:.3f} KiMsgs/s, {:.0f} bytes/msg",
        static_cast<double>(bytes) / time_gap.count() / 1024 / 1024,
        static_cast<double>(msgs) / time_gap.count() / 1024,
        msgs == 0 ? 0 : static_cast<double>(bytes) / static_cast<double>(msgs));

    last_print_time_ = now;
  }

 private:
  TcpServer server_;
  std::shared_ptr<spdlog::logger> logger_;

  TimePoint last_print_time_;
  std::atomic_uint64_t receive_bytes_{0};
  std::atomic_uint64_t receive_msgs_{0};
};

int main() {
  InitBenet();
  EventLoop loop;
  InetAddress addr(8080);
  EchoServer svr(&loop, addr);
  svr.Start(/* n_threads: */ 2);
  loop.Start();
}
