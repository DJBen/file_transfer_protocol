#include "net_include.h"

int main(int argc, char const *argv[])
{
    struct sockaddr_in name;
    int                s;
    fd_set             mask;
    int                recv_s;
    fd_set             dummy_mask,temp_mask;
    int                i,j,num;
    long               on=1;

    PACKET *currentPacket = NULL;
    int packetSize;
    char dest_file_name[NAME_LENGTH];
    FILE *fw;
    int nwritten;

  if (argc != 1) {
    printf("Usage: t_rcv\n");
    return 1;
  }

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s<0) {
        perror("Net_server: socket");
        exit(1);
    }

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        perror("Net_server: setsockopt error \n");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if ( bind( s, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Net_server: bind");
        exit(1);
    }

    if (listen(s, 4) < 0) {
        perror("Net_server: listen");
        exit(1);
    }

    i = 0;
    FD_ZERO(&mask);
    FD_ZERO(&dummy_mask);
    FD_SET(s,&mask);
    for(;;)
    {
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, NULL);
        if (num > 0) {
            if ( FD_ISSET(s,&temp_mask) ) {
                recv_s = accept(s, 0, 0) ;
                FD_SET(recv_s, &mask);
            }
            if ( FD_ISSET(recv_s,&temp_mask) ) {
                packetSize = sizeof(PACKET) + sizeof(char) * BUF_SIZE;
                currentPacket = malloc(packetSize);
                recv(recv_s, currentPacket, packetSize, 0);
                if (currentPacket->type == packet_type_metadata) {
                    strcpy(dest_file_name, (char *)currentPacket->data);
                    if((fw = fopen(dest_file_name, "wb")) == NULL) {
                        perror("fopen");
                        exit(0);
                    }
                } else if (currentPacket->type == packet_type_normal) {
                    nwritten = fwrite(currentPacket->data, sizeof(unsigned char), currentPacket->data_size, fw);
                    if (nwritten != currentPacket->data_size) {
                        perror("fwrite error.");
                        exit(0);
                    }
                    if (currentPacket->completed) {
                        fclose(fw);
                        free(currentPacket);
                        currentPacket = NULL;
                        break;
                    }
                    free(currentPacket);
                    currentPacket = NULL;
                }
            }

        }
    }

    return 0;

}
