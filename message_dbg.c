#include "message_dbg.h"

ssize_t sendto_dbg(int socket, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len, int loss_rate_percent) {
  double roll = rand() * 1.0 / RAND_MAX;
  if (roll < loss_rate_percent / 100.0) {
    return -1;
  }
  return sendto(socket, buffer, length, flags, dest_addr, dest_len);
}

ssize_t recvfrom_dbg(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len, int loss_rate_percent) {
  double roll = rand() * 1.0 / RAND_MAX;
  if (roll < loss_rate_percent / 100.0) {
    printf("Receive fail %g / %g\n", roll, loss_rate_percent / 100.0);
    return -1;
  }
  return recvfrom(socket, buffer, length, flags, address, address_len);
}
