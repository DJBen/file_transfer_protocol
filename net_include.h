#include <stdio.h>

#include <stdlib.h>

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>

#include <errno.h>

#define PORT	     10180

#define MAX_MESS_LEN 8192
#define BUF_SIZE 1400
#define NAME_LENGTH 80

typedef char packet_type;

packet_type packet_type_normal = 0;
packet_type packet_type_metadata = 1;

typedef struct PACKET
{
  packet_type type;
  unsigned int index;
  const void *data;
} PACKET;
