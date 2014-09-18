#include "net_include.h"

void receiveFile(int loss_rate_percent);

int setFeedback(FEEDBACK **feedback, int good_index, int* nacks, int nack_count);

int main(int argc, char const *argv[])
{
  int loss_rate_percent;
  if (argc != 2) {
    printf("Usage: rcv <loss_rate_percent>\n");
    return 1;
  }
  loss_rate_percent = (int)strtol(argv[0], (char **)NULL, 10);
  receiveFile(loss_rate_percent);
  return 0;
}

void receiveFile(int loss_rate_percent) {
    struct sockaddr_in    name;
    struct sockaddr_in    send_addr;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    struct hostent        h_ent;
    struct hostent        *p_h_ent;
    char                  dest_file_name[NAME_LENGTH] = {'\0'};
    char                  my_name[NAME_LENGTH] = {'\0'};
    int                   host_num;
    int                   from_ip;
    int                   ss,sr;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   bytes;
    int                   num;
    struct timeval        timeout;

    /* Files and packets */
    int nwritten;
    FILE *fw;
    int packetSize;
    int feedbackSize;
    int i;
    PACKET *currentPacket = NULL;
    FEEDBACK *currentFeedback = NULL;

    sr = socket(AF_INET, SOCK_DGRAM, 0);  /* socket for receiving (udp) */
    if (sr<0) {
        perror("Ucast: socket");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if ( bind( sr, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Ucast: bind");
        exit(1);
    }

    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );

    for(;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                from_len = sizeof(from_addr);
                if (currentPacket) {
                    free(currentPacket);
                }
                packetSize = sizeof(PACKET) + sizeof(char) * BUF_SIZE;
                currentPacket = calloc(1, packetSize);
                bytes = recvfrom(sr, currentPacket, packetSize, 0,
                          (struct sockaddr *)&from_addr,
                          &from_len);
                from_ip = from_addr.sin_addr.s_addr;

                printf( "Received from (%d.%d.%d.%d): length %d\n",
                (htonl(from_ip) & 0xff000000)>>24,
                (htonl(from_ip) & 0x00ff0000)>>16,
                (htonl(from_ip) & 0x0000ff00)>>8,
                (htonl(from_ip) & 0x000000ff),
                bytes);

                if (currentPacket->type == packet_type_metadata) {
                    /* If current packet is metadata packet (contains host name),
                     * we set up the sending socket with the host name.
                     */
                    strcpy(dest_file_name, (char *)currentPacket->data);
                    printf("%s\n", dest_file_name);

                    /* socket for sending (udp) */
                    ss = socket(AF_INET, SOCK_DGRAM, 0);
                    if (ss<0) {
                        perror("Ucast: socket");
                        exit(1);
                    }

                    /* Send feedback packet */
                    feedbackSize = setFeedback(&currentFeedback, -1, NULL, 0);

                    from_addr.sin_port = htons(PORT);
                    int send_result = sendto(ss, currentFeedback, feedbackSize, 0, (struct sockaddr *)&from_addr, sizeof(from_addr));
                    if (send_result == -1) {
                        printf("send metadata feedback error\n");
                        exit(1);
                    }
                      /* Open or create the destination file for writing */
                    if((fw = fopen(dest_file_name, "wb")) == NULL) {
                        perror("fopen");
                        exit(0);
                    }
                } else if (currentPacket->type == packet_type_normal) {
                    if (fw == NULL) {
                        printf("Write stream not open caused by missing metadata packet.\n");
                        exit(0);
                    }
                    printf("Packet %d received of size %ld.\n", currentPacket->index, currentPacket->data_size);
                    nwritten = fwrite(currentPacket->data, sizeof(unsigned char), currentPacket->data_size, fw);
                    if (currentPacket->completed) {
                        fclose(fw);
                        /* free(currentPacket); */
                        printf("File transfer completed.\n");
                        break;
                    }
                }
            }
        } else {
          printf(".");
        }
    }
}

int setFeedback(FEEDBACK **feedback, int good_index, int *nacks, int nack_count) {
    int feedbackSize;
    int i;

    feedbackSize = sizeof(FEEDBACK) + sizeof(int) * nack_count;
    if (*feedback) free(*feedback);
    (*feedback) = calloc(1, feedbackSize);
    (*feedback)->good_index = good_index;
    (*feedback)->nack_count = nack_count;
    for (i = 0; i < nack_count; i++) {
        *((int *)(*feedback)->nacks + i) = nacks[i];
    }
    return feedbackSize;
}


