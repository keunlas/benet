add_library(benet_interface INTERFACE)
target_include_directories(benet_interface INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_sources(benet_interface INTERFACE
  benet.cpp
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

add_library(benet ${${PROJECT_NAME}_LIBRARY_TYPE})
target_link_libraries(benet PRIVATE benet_interface)
