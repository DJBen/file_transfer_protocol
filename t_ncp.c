#include "net_include.h"

void sendFile(char *file_name, char *dest_file_name, char *comp_name);

int main(int argc, char const *argv[])
{
  char source_file_name[NAME_LENGTH];
  char dest_file_name[NAME_LENGTH];
  char comp_name[NAME_LENGTH];
  char *find_at_symbol_ptr;

  if (argc != 3) {
    printf("Usage: t_ncp <source_file_name> <dest_file_name>@<comp_name>\n");
    return 1;
  }
  strcpy(source_file_name, argv[1]);

  find_at_symbol_ptr = strchr(argv[2], '@');
  if (find_at_symbol_ptr == NULL) {
    printf("Usage: t_ncp <source_file_name> <dest_file_name>@<comp_name>\n");
    return 1;
  }
  *find_at_symbol_ptr = 0;
  strcpy(dest_file_name, argv[2]);
  strcpy(comp_name, find_at_symbol_ptr + 1);

  sendFile(source_file_name, dest_file_name, comp_name);

  return 0;
}

void sendFile(char *file_name, char *dest_file_name, char *comp_name)
{
    struct sockaddr_in host;
    struct hostent     h_ent, *p_h_ent;

    char               host_name[80];
    char               *c;

    int                s;
    int                ret;

    PACKET *currentPacket;
    int packetSize;
    int latestPacketIndex = 0;
    FILE *fr;
    int nread = 0;
    currentPacket = NULL;
    unsigned char *temp_buf;

    s = socket(AF_INET, SOCK_STREAM, 0); /* Create a socket (TCP) */
    if (s<0) {
        perror("Net_client: socket error");
        exit(1);
    }

    host.sin_family = AF_INET;
    host.sin_port   = htons(PORT);

    p_h_ent = gethostbyname(comp_name);

    if ( p_h_ent == NULL ) {
        printf("net_client: gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent) );
    memcpy( &host.sin_addr, h_ent.h_addr_list[0],  sizeof(host.sin_addr) );

    ret = connect(s, (struct sockaddr *)&host, sizeof(host) ); /* Connect! */
    if( ret < 0)
    {
        perror( "Net_client: could not connect to server");
        exit(1);
    }

    /* Send metadata packet first. */
        packetSize = sizeof(PACKET) + sizeof(char) * NAME_LENGTH;
        currentPacket = malloc(packetSize);
        currentPacket->type = packet_type_metadata;
        currentPacket->completed = false;
        currentPacket->index = -1;
        strcpy(currentPacket->data, dest_file_name);
        currentPacket->data_size = sizeof(char) * strlen(dest_file_name);

    ret = send( s, currentPacket, packetSize, 0);
    if(ret != packetSize)
    {
        perror( "Net_client: error in writing metadata");
        exit(1);
    }

    if((fr = fopen(file_name, "rb")) == NULL) {
        perror("fopen");
        exit(0);
    }

    for(;;)
    {
            /* We read file and construct the packet */
            temp_buf = malloc(sizeof(unsigned char) * BUF_SIZE);
            nread = fread(temp_buf, sizeof(unsigned char), BUF_SIZE, fr);
            packetSize = sizeof(PACKET) + nread * sizeof(unsigned char);
            if (currentPacket != NULL) free(currentPacket);
            currentPacket = malloc(packetSize);
            currentPacket->type = packet_type_normal;
            currentPacket->completed = nread < BUF_SIZE && feof(fr); /* EOF: true, otherwise false */
            currentPacket->index = latestPacketIndex++;
            currentPacket->data_size = nread;
            memcpy(currentPacket->data, temp_buf, nread * sizeof(unsigned char));
            free(temp_buf);

        ret = send( s, currentPacket, packetSize, 0);
        if(ret != packetSize)
        {
            perror( "Net_client: error in writing");
            exit(1);
        }

            if (nread < BUF_SIZE && feof(fr)) {
                free(currentPacket);
                fclose(fr);
                break;
            }

    }

}

