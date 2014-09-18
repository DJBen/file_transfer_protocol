#include "net_include.h"

int gethostname(char*,size_t);
void sendFile(char *file_name, char *dest_file_name, char *comp_name, int loss_rate_percent);
void readFeedbackNacks(FEEDBACK *feedback, int **nacks);

int main(int argc, char const *argv[])
{
  int loss_rate_percent;
  char source_file_name[NAME_LENGTH];
  char dest_file_name[NAME_LENGTH];
  char comp_name[NAME_LENGTH];
  char *find_at_symbol_ptr;

  if (argc != 4) {
    printf("Usage: ncp <loss_rate_percent> <source_file_name> <dest_file_name>@<comp_name>\n");
    return 1;
  }
  loss_rate_percent = (int)strtol(argv[1], (char **)NULL, 10);
  strcpy(source_file_name, argv[2]);

  find_at_symbol_ptr = strchr(argv[3], '@');
  if (find_at_symbol_ptr == NULL) {
    printf("Usage: ncp <loss_rate_percent> <source_file_name> <dest_file_name>@<comp_name>\n");
    return 1;
  }
  *find_at_symbol_ptr = 0;
  strcpy(dest_file_name, argv[3]);
  strcpy(comp_name, find_at_symbol_ptr + 1);

  sendFile(source_file_name, dest_file_name, comp_name, loss_rate_percent);

  return 0;
}

void sendFile(char *file_name, char *dest_file_name, char *comp_name, int loss_rate_percent) {
    struct sockaddr_in    name;
    struct sockaddr_in    send_addr;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    struct hostent        h_ent;
    struct hostent        *p_h_ent;
    char                  my_name[NAME_LENGTH] = {'\0'};
    int                   host_num;
    int                   from_ip;
    int                   ss,sr;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    struct timeval        timeout;
    int                   num;
    int i;

    /* Open file for reading */
    FILE *fr = NULL;
    int nread;

    /* Packets and feedback */
    unsigned char *temp_buf;
    PACKET *currentPacket = NULL;
    int packetSize;
    FEEDBACK *currentFeedback = NULL;
    int feedbackSize;
    int *nacks;
    int packetIndex;

    nacks = malloc(sizeof(int));
    temp_buf = malloc(sizeof(unsigned char) * BUF_SIZE);
    packetIndex = 0;

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

    ss = socket(AF_INET, SOCK_DGRAM, 0); /* socket for sending (udp) */
    if (ss<0) {
        perror("Ucast: socket");
        exit(1);
    }

    gethostname(my_name, NAME_LENGTH);

    p_h_ent = gethostbyname(comp_name);
    if ( p_h_ent == NULL ) {
        printf("Ucast: gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent));
    memcpy( &host_num, h_ent.h_addr_list[0], sizeof(host_num) );

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = host_num;
    send_addr.sin_port = htons(PORT);

    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );

    /* Send metadata packet first. */
    packetSize = sizeof(PACKET) + sizeof(char) * NAME_LENGTH;
    currentPacket = malloc(packetSize);
    currentPacket->type = packet_type_metadata;
    currentPacket->completed = false;
    currentPacket->index = -1;
    strcpy(currentPacket->data, dest_file_name);
    currentPacket->data_size = sizeof(char) * strlen(dest_file_name);
    sendto(ss, currentPacket, packetSize, 0, (struct sockaddr *)&send_addr, sizeof(send_addr));

    for(;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 1; /* original value was 10 */
        timeout.tv_usec = 0;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                from_len = sizeof(from_addr);
                if (currentFeedback) free(currentFeedback);
                currentFeedback = calloc(1, sizeof(FEEDBACK) + sizeof(int) * WINDOW_SIZE);
                feedbackSize = recvfrom( sr, currentFeedback, sizeof(currentFeedback) + sizeof(int) * WINDOW_SIZE, 0, (struct sockaddr *)&from_addr, &from_len);
                from_ip = from_addr.sin_addr.s_addr;
                printf( "Received from %d.%d.%d.%d - (%d, %ld)\n",
                (htonl(from_ip) & 0xff000000)>>24,
                (htonl(from_ip) & 0x00ff0000)>>16,
                (htonl(from_ip) & 0x0000ff00)>>8,
                (htonl(from_ip) & 0x000000ff), currentFeedback->good_index,
                currentFeedback->nack_count);

                if (currentFeedback->good_index == -1) {
                    /* Metadata feedback: ready for transmission */
                    /* Open the source file for reading */
                    if((fr = fopen(file_name, "rb")) == NULL) {
                      perror("fopen");
                      exit(0);
                    }
                    printf("Metadata sent.\n");
                } else {
                    readFeedbackNacks(currentFeedback, &nacks);
                    for (i = 0; i < currentFeedback->nack_count; i++) {
                        printf("%d, ", nacks[i]);
                    }
                    printf("\n");
                }
            }
        }
        if (fr == NULL) continue;
        nread = fread(temp_buf, sizeof(unsigned char), BUF_SIZE, fr);
        packetSize = sizeof(PACKET) + nread * sizeof(unsigned char);
        currentPacket = malloc(packetSize);
        currentPacket->type = packet_type_normal;
        currentPacket->completed = false;
        currentPacket->index = packetIndex++;
        currentPacket->data_size = nread;
        memcpy(currentPacket->data, temp_buf, nread * sizeof(unsigned char));
        if (nread > 0) {
            if (nread < BUF_SIZE && feof(fr)) {
                /* Did we reach the EOF? */
                printf("Finished reading.\n");
                currentPacket->completed = true;
                fclose(fr);
                sendto(ss, currentPacket, packetSize, 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
                free(currentPacket);
                break;
            } else {
                sendto(ss, currentPacket, packetSize, 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
                free(currentPacket);
            }
        }
        if (nread < BUF_SIZE && !feof(fr)) {
            printf("An error occurred...\n");
            exit(0);
        }
    }
}

void readFeedbackNacks(FEEDBACK *feedback, int **nacks) {
    int i;
    if (*nacks) free(*nacks);
    (*nacks) = calloc(1, sizeof(int) * feedback->nack_count);
    for (i = 0; i < feedback->nack_count; i++) {
      *((*nacks) + i) = (int)*(feedback->nacks + i);
    }
}
