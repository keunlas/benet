add_library(benet ${${PROJECT_NAME}_LIBRARY_TYPE})
target_include_directories(benet PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_sources(benet PRIVATE
  logger.cpp
  channel.cpp
  eventloop.cpp
  poller.cpp
  timer.cpp
  timer_queue.cpp
  inet_address.cpp
  socket_ops.cpp
  socket.cpp
  buffer.cpp
  acceptor.cpp
  eventloop_threadpool.cpp
  tcp_connection.cpp
  tcp_server.cpp
  connector.cpp
  tcp_client.cpp
)
