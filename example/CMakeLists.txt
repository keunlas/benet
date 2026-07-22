add_executable(echo_server echo_server.cpp)
target_link_libraries(echo_server PRIVATE benet_interface)

add_executable(echo_client echo_client.cpp)
