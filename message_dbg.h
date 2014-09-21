#ifndef MESSAGE_DBG_
#define MESSAGE_DBG_

#include "net_include.h"

extern ssize_t sendto_dbg(int socket, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len, int loss_rate_percent);

extern ssize_t recvfrom_dbg(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len, int loss_rate_percent);

#endif
