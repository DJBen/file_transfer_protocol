#include "net_include.h"
#include "queue.h"
#include "message_dbg.h"

int gethostname(char*,size_t);
void sendFile(char *file_name, char *dest_file_name, char *comp_name, int loss_rate_percent);
void readFeedback(FEEDBACK *feedback, queue *nacks, int *aru);

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

    /* Open file for reading */
    FILE *fr = NULL;
    int nread;

    /* Packets and feedback */
    unsigned char *temp_buf;
    PACKET *currentPacket = NULL;
    int packetSize;
    FEEDBACK *currentFeedback = NULL;
    int feedbackSize;
    int latestPacketIndex;
    int final_packet_index;
    PACKET_INFO *window;
    int aru;
    queue *nacks;
    PACKET_INFO temp_info;
    bool file_finished_reading;
    int sent_info;
    int makeup_index;

    file_finished_reading = false;
    nacks = malloc(sizeof(queue));
    init_queue(nacks);
    window = calloc(WINDOW_SIZE, sizeof(PACKET_INFO));
    aru = -1;

    temp_buf = malloc(sizeof(unsigned char) * BUF_SIZE);
    latestPacketIndex = 0;
    final_packet_index = INT_MAX;

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

    for (;;)
    {
        /* Send metadata packet first. */
        packetSize = sizeof(PACKET) + sizeof(char) * NAME_LENGTH;
        currentPacket = malloc(packetSize);
        currentPacket->type = packet_type_metadata;
        currentPacket->completed = false;
        currentPacket->index = -1;
        strcpy(currentPacket->data, dest_file_name);
        currentPacket->data_size = sizeof(char) * strlen(dest_file_name);
        sendto_dbg(ss, currentPacket, packetSize, 0, (struct sockaddr *)&send_addr, sizeof(send_addr), loss_rate_percent);

        temp_mask = mask;
        timeout.tv_sec = 0;
        timeout.tv_usec = 2;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                from_len = sizeof(from_addr);
                if (currentFeedback) free(currentFeedback);
                currentFeedback = calloc(1, sizeof(FEEDBACK) + sizeof(int) * WINDOW_SIZE);
                feedbackSize = recvfrom_dbg( sr, currentFeedback, sizeof(currentFeedback) + sizeof(int) * WINDOW_SIZE, 0, (struct sockaddr *)&from_addr, &from_len, loss_rate_percent);
                if (feedbackSize == -1) continue;
                from_ip = from_addr.sin_addr.s_addr;
                if (currentFeedback->aru == -1) {
                    /* Metadata feedback: ready for transmission */
                    /* Open the source file for reading */
                    if((fr = fopen(file_name, "rb")) == NULL) {
                      perror("fopen");
                      exit(0);
                    }
                    printf("Metadata acknowledgement received.\n");
                    break;
                }
            }
        }
    }

    for(;;)
    {
        temp_mask = mask;
        timeout.tv_sec = 0; /* original value was 10 */
        timeout.tv_usec = 2;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                from_len = sizeof(from_addr);
                if (currentFeedback) free(currentFeedback);
                currentFeedback = calloc(1, sizeof(FEEDBACK) + sizeof(int) * WINDOW_SIZE);
                feedbackSize = recvfrom_dbg( sr, currentFeedback, sizeof(currentFeedback) + sizeof(int) * WINDOW_SIZE, 0, (struct sockaddr *)&from_addr, &from_len, loss_rate_percent);
                /* If failed, just loop */
                if (feedbackSize == -1) continue;
                from_ip = from_addr.sin_addr.s_addr;
                /* printf( "Received from %d.%d.%d.%d: ",
                (htonl(from_ip) & 0xff000000)>>24,
                (htonl(from_ip) & 0x00ff0000)>>16,
                (htonl(from_ip) & 0x0000ff00)>>8,
                (htonl(from_ip) & 0x000000ff)); */

                if (currentFeedback->aru == -1) {
                    /* printf("Metadata received at normal session: ignore.\n"); */
                } else {
                    readFeedback(currentFeedback, nacks, &aru);
                    /* printf("ARU = %d, #NACK = %d\n", aru, nacks->count); */
                }
            }
        }

        if (nacks->count > 0) {
            /* Send missing packets first */
            while (nacks->count > 0) {
                makeup_index = dequeue(nacks);
                temp_info = window[makeup_index % WINDOW_SIZE];
                sent_info = sendto_dbg(ss, temp_info.packet, temp_info.packetSize, 0, (struct sockaddr *)&send_addr, sizeof(send_addr), loss_rate_percent);
                if (sent_info == -1) {
                    /* printf("Make up packet %d fails.\n", makeup_index); */
                }
            }
        } else {

            /* If no nacks and file finished reading: complete transmission */
            if (file_finished_reading) {
                if (aru >= final_packet_index) {
                    printf("File transmission completed.\n");
                    break;
                } else {
                    /* Recipient neither receives all packets nor reports any nacks. */
                    /* printf("NN aru = %d, final_packet_index = %d, latestPacketIndex = %d\n", aru, final_packet_index, latestPacketIndex); */
                    temp_info = window[(latestPacketIndex - 1) % WINDOW_SIZE];
                    sendto_dbg(ss, temp_info.packet, temp_info.packetSize, 0, (struct sockaddr *)&send_addr, sizeof(send_addr), loss_rate_percent);
                    continue;
                }
            }

            /* If sending too fast, rest for a while: stop going forward */
            if (latestPacketIndex - aru + 1 >= WINDOW_SIZE) {
                /* printf("Sending too fast... aru = %d, latestPacketIndex = %d\n", aru, latestPacketIndex); */
                temp_info = window[(latestPacketIndex - 1) % WINDOW_SIZE];
                sendto_dbg(ss, temp_info.packet, temp_info.packetSize, 0, (struct sockaddr *)&send_addr, sizeof(send_addr), loss_rate_percent);
                continue;
            }

            /* We read file and construct the packet */
            nread = fread(temp_buf, sizeof(unsigned char), BUF_SIZE, fr);
            packetSize = sizeof(PACKET) + nread * sizeof(unsigned char);
            currentPacket = malloc(packetSize);
            currentPacket->type = packet_type_normal;
            currentPacket->completed = nread < BUF_SIZE && feof(fr); /* EOF: true, otherwise false */
            currentPacket->index = latestPacketIndex;
            currentPacket->data_size = nread;
            memcpy(currentPacket->data, temp_buf, nread * sizeof(unsigned char));
            /*
             * Move buffer forward and clearing the previous packet buffer
             * from last time window.
             */
            if (window[latestPacketIndex % WINDOW_SIZE].packet != NULL) {
                free(window[latestPacketIndex % WINDOW_SIZE].packet);
                window[latestPacketIndex % WINDOW_SIZE].packet = NULL;
                window[latestPacketIndex % WINDOW_SIZE].packetSize = 0;
            }

            /* Save to send buffer so that we can resend at any time */
            temp_info.packet = currentPacket;
            temp_info.packetSize = packetSize;
            window[latestPacketIndex % WINDOW_SIZE] = temp_info;

            if (nread < BUF_SIZE && feof(fr)) {
                /* Did we reach the EOF? */
                printf("Finished reading.\n");
                fclose(fr);
                sendto_dbg(ss, currentPacket, packetSize, 0, (struct sockaddr *)&send_addr, sizeof(send_addr), loss_rate_percent);
                file_finished_reading = true;
                final_packet_index = latestPacketIndex;
                /* printf("Final packet index %d\n", final_packet_index); */
            } else if (nread < BUF_SIZE && !feof(fr)) {
                printf("fread error.\n");
                exit(0);
            } else {
                sendto_dbg(ss, currentPacket, packetSize, 0, (struct sockaddr *)&send_addr, sizeof(send_addr), loss_rate_percent);
            }
            latestPacketIndex++;
        }
    }
}

void readFeedback(FEEDBACK *feedback, queue *nacks, int *aru) {
    /* Append new nacks to the queue and update aru */
    int i;
    *aru = feedback->aru;
    for (i = 0; i < feedback->nack_count; i++) {
        if (queueContains(nacks, (int)*(feedback->nacks + i)) == 0) {
            enqueue(nacks, (int)*(feedback->nacks + i));
        }
    }
}
