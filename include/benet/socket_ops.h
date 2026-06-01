// Distributed under the MIT License that can be found in the LICENSE file.
// https://github.com/keunlas/be
//
// Author: Keunlas <keunlaz at gmail dot com>

#ifndef BENET_SOCKET_OPS_H_
#define BENET_SOCKET_OPS_H_

#include <arpa/inet.h>

namespace benet {
namespace sockets {

int create_nonblocking_or_die(sa_family_t family);

void bind_or_die(int sockfd, const sockaddr* addr);
void listen_or_die(int sockfd);

int connect(int sockfd, const sockaddr* addr);
int accept(int sockfd, sockaddr_storage* addr);

ssize_t read(int sockfd, void* buf, size_t count);
ssize_t readv(int sockfd, const iovec* iov, int iovcnt);
ssize_t write(int sockfd, const void* buf, size_t count);

void close(int sockfd);
void shutdown_write(int sockfd);

int get_socket_error(int sockfd);

sockaddr_storage get_local_addr(int sockfd);
sockaddr_storage get_peer_addr(int sockfd);

bool is_self_connect(int sockfd);

}  // namespace sockets
}  // namespace benet

#endif  // !BENET_SOCKET_OPS_H_
