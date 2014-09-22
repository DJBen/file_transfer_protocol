#include "net_include.h"
#include "sendto_dbg.h"
#include <time.h>

void receiveFile(int loss_rate_percent);
int setFeedback(FEEDBACK **feedback, int aru, int* nacks, int nack_count);
void dump_nacks(int *nacks, int nack_count);

int main(int argc, char const *argv[])
{
  int loss_rate_percent;
  if (argc != 2) {
    printf("Usage: rcv <loss_rate_percent>\n");
    return 1;
  }
  loss_rate_percent = (int)strtol(argv[1], (char **)NULL, 10);
  sendto_dbg_init(loss_rate_percent);
  receiveFile(loss_rate_percent);
  return 0;
}

void receiveFile(int loss_rate_percent) {
    struct sockaddr_in    name;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    char                  dest_file_name[NAME_LENGTH] = {'\0'};
    int                   from_ip;
    int                   ss,sr;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   bytes;
    int                   num;
    struct timeval        timeout;

    /* Files and packets */
    int nwritten;
    FILE *fw = NULL;
    int packetSize;
    int feedbackSize;
    int i;
    PACKET *currentPacket = NULL;
    FEEDBACK *currentFeedback = NULL;
    int send_result;
    PACKET **window;
    int aru;
    int temp_nacks[WINDOW_SIZE];
    int temp_nack_size;
    int new_aru;
    int largest_packet_index;
    bool completed;

    /* Timing */
    clock_t begin,end;
    double time_spent;
    int packetsPer50M;
    int numberOf50M;

    begin= clock();
    packetsPer50M = 50 * 1024 * 1024 / BUF_SIZE;
    numberOf50M = 1;

    largest_packet_index = -1;
    window = calloc(WINDOW_SIZE, sizeof(PACKET *));
    aru = -1;
    completed = false;

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
                packetSize = sizeof(PACKET) + sizeof(char) * BUF_SIZE;
                currentPacket = malloc(packetSize);
                bytes = recvfrom(sr, currentPacket, packetSize, 0,
                          (struct sockaddr *)&from_addr,
                          &from_len);
                if (bytes == -1) continue;
                from_ip = from_addr.sin_addr.s_addr;

                /* printf( "Received from (%d.%d.%d.%d): length %d\n",
                (htonl(from_ip) & 0xff000000)>>24,
                (htonl(from_ip) & 0x00ff0000)>>16,
                (htonl(from_ip) & 0x0000ff00)>>8,
                (htonl(from_ip) & 0x000000ff),
                bytes); */

                if (currentPacket->type == packet_type_metadata) {
                    /* If current packet is metadata packet (contains host name),
                     * we set up the sending socket with the host name.
                     */
                    strcpy(dest_file_name, (char *)currentPacket->data);
                    /* printf("%s\n", dest_file_name); */

                    /* socket for sending (udp) */
                    ss = socket(AF_INET, SOCK_DGRAM, 0);
                    if (ss<0) {
                        perror("Ucast: socket");
                        exit(1);
                    }

                    /* Send feedback packet */
                    feedbackSize = setFeedback(&currentFeedback, -1, NULL, 0);

                    from_addr.sin_port = htons(PORT);

                    do {
                        send_result = sendto_dbg(ss, currentFeedback, feedbackSize, 0, (struct sockaddr *)&from_addr, sizeof(from_addr));
                        if (send_result == -1) {
                            printf("Send metadata feedback error\n");
                        }
                    } while (send_result == -1);

                    /* Open or create the destination file for writing */
                    /* Already opened: skip this step */
                    if (fw != NULL) continue;
                    if((fw = fopen(dest_file_name, "wb")) == NULL) {
                        perror("fopen");
                        exit(0);
                    }

                } else if (currentPacket->type == packet_type_normal) {
                    if (fw == NULL) {
                        printf("Write stream not open caused by missing metadata packet.\n");
                        exit(0);
                    }
                    if (currentPacket->index % 100 == 0) {
                        printf("Packet %d received of size %d.\n", currentPacket->index, currentPacket->data_size);
                    }
                    /* printf("Packet %d received of size %d.\n", currentPacket->index, currentPacket->data_size); */

                    /* Packet too old, discard */
                    if (abs(aru - currentPacket->index) > WINDOW_SIZE) continue;

                    if (largest_packet_index < currentPacket->index) {
                        largest_packet_index = currentPacket->index;
                    }

                    if (aru < currentPacket->index) {
                        /* Save packet to buffer, waiting for write operation */
                        window[currentPacket->index % WINDOW_SIZE] = currentPacket;
                    }

                    /* Increment new aru */
                    new_aru = aru;
                    for (i = aru + 1; i <= aru + WINDOW_SIZE; i++) {
                        if (window[i % WINDOW_SIZE] == NULL) break;
                        new_aru++;
                    }
                    /* printf("old aru: %d, new aru: %d\n", aru, new_aru); */

                    /* Print timing */
                    if (new_aru >= packetsPer50M*numberOf50M){
                        printf("%d%s\n",numberOf50M*50," Mb of file received.");
                        end = clock();
                        time_spent = (double)(end - begin)/ CLOCKS_PER_SEC;
                        printf("%s%f%s\n","Data Trasfer Speed: ",1400*new_aru/time_spent/1000000," Mb/s");
                        numberOf50M++;
                    }

                    /* Construct nacks */
                    temp_nack_size = 0;
                    for (i = new_aru + 1; i <= largest_packet_index; i++) {
                        if (window[i % WINDOW_SIZE] == NULL) {
                            temp_nacks[temp_nack_size++] = i;
                        }
                    }
                    /* dump_nacks(temp_nacks, temp_nack_size); */

                    /* Send feedback */
                    feedbackSize = setFeedback(&currentFeedback, new_aru, temp_nacks, temp_nack_size);
                    from_addr.sin_port = htons(PORT);
                    do
                    {
                        send_result = sendto_dbg(ss, currentFeedback, feedbackSize, 0, (struct sockaddr *)&from_addr, sizeof(from_addr));
                    } while (send_result == -1);
                    completed = completed || currentPacket->completed;

                    /* Write packets up to new ARU to the disk and free these in the buffer */
                    for (i = aru + 1; i <= new_aru; i++) {
                        nwritten = fwrite(window[i % WINDOW_SIZE]->data, sizeof(unsigned char), window[i % WINDOW_SIZE]->data_size, fw);
                        if (nwritten != window[i % WINDOW_SIZE]->data_size) {
                            perror("fwrite error.");
                            exit(0);
                        }
                        free(window[i % WINDOW_SIZE]);
                        window[i % WINDOW_SIZE] = NULL;
                    }
                    aru = new_aru;

                    if (completed && temp_nack_size == 0) {
                        fclose(fw);
                        fw = NULL;
                        printf("File transmission completed.\n");
                        end = clock();
                        time_spent = (double)(end-begin)/CLOCKS_PER_SEC;
                        printf("%s%f%s\n","Average Data Transfer Speed: ",1400*largest_packet_index/1000000/time_spent," Mb/s");
                        break;
                    }
                }
            }
        } else {
          printf(".");
        }
    }
}

int setFeedback(FEEDBACK **feedback, int aru, int *nacks, int nack_count) {
    int feedbackSize;
    int i;

    feedbackSize = sizeof(FEEDBACK) + sizeof(int) * nack_count;
    if (*feedback) free(*feedback);
    (*feedback) = calloc(1, feedbackSize);
    (*feedback)->aru = aru;
    (*feedback)->nack_count = nack_count;
    for (i = 0; i < nack_count; i++) {
        *((int *)(*feedback)->nacks + i) = nacks[i];
    }
    return feedbackSize;
}

void dump_nacks(int *nacks, int nack_count) {
    int i;
    printf("nack_dump (%d): ", nack_count);
    for (i = 0; i < nack_count; i++) {
        printf("%d ", nacks[i]);
    }
    printf("\n");
}

