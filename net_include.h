#ifndef NET_INCLUDE_
#define NET_INCLUDE_

#include <stdio.h>
#include <limits.h>
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

#define packet_type_normal 0
#define packet_type_metadata 1

#define TRUE 1
#define FALSE 0

typedef struct
{
  char type;
  bool completed;
  int index;
  int data_size;
  unsigned char data[1];
} PACKET;

typedef struct
{
  int aru;
  int nack_count;
  int nacks[1];
} FEEDBACK;

typedef struct
{
  PACKET *packet;
  int packetSize;
} PACKET_INFO;

#endif

