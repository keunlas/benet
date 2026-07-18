#include "benet/tcp_server.h"

using namespace benet;

int main() {
  EventLoop loop;
  InetAddress addr(8080);
  TcpServer svr(&loop, addr, false, "Echo");
  svr.BindMessageCallback(
      [](const TcpConnectionPtr& conn, Buffer* buf, TimePoint) {
        // Send all received back
        conn->Send(buf);
      });
  svr.InitThreadsNumber(2);
  svr.Start();
  loop.Start();
}
