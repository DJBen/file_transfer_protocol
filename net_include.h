#include <stdio.h>

#include <stdlib.h>
#include <stdbool.h>
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
#define WINDOW_SIZE 100
typedef char packet_type;

packet_type packet_type_normal = 0;
packet_type packet_type_metadata = 1;

typedef struct
{
  packet_type type;
  bool completed;
  int index;
  size_t data_size;
  unsigned char data[1];
} PACKET;

typedef struct
{
  int aru;
  size_t nack_count;
  int nacks[1];
} FEEDBACK;

typedef struct
{
  PACKET *packet;
  int packetSize;
} PACKET_INFO;

