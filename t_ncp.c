#include "net_include.h"

void sendFile(char *file_name, char *dest_file_name, char *comp_name);

int main(int argc, char const *argv[])
{
  int loss_rate_percent;
  char source_file_name[NAME_LENGTH];
  char dest_file_name[NAME_LENGTH];
  char comp_name[NAME_LENGTH];
  char *find_at_symbol_ptr;

  if (argc != 4) {
    printf("Usage: t_ncp <loss_rate_percent> <source_file_name> <dest_file_name>@<comp_name>\n");
    return 1;
  }
  loss_rate_percent = (int)strtol(argv[1], (char **)NULL, 10);
  strcpy(source_file_name, argv[2]);

  find_at_symbol_ptr = strchr(argv[3], '@');
  if (find_at_symbol_ptr == NULL) {
    printf("Usage: t_ncp <loss_rate_percent> <source_file_name> <dest_file_name>@<comp_name>\n");
    return 1;
  }
  *find_at_symbol_ptr = 0;
  strcpy(dest_file_name, argv[3]);
  strcpy(comp_name, find_at_symbol_ptr + 1);

  sendto_dbg_init(loss_rate_percent);
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
    int packetIndex;

    s = socket(AF_INET, SOCK_STREAM, 0); /* Create a socket (TCP) */
    if (s<0) {
        perror("Net_client: socket error");
        exit(1);
    }

    host.sin_family = AF_INET;
    host.sin_port   = htons(PORT);

    printf("Enter the server name:\n");
    if ( fgets(host_name,80,stdin) == NULL ) {
        perror("net_client: Error reading server name.\n");
        exit(1);
    }
    c = strchr(host_name,'\n'); /* remove new line */
    if ( c ) *c = '\0';
    c = strchr(host_name,'\r'); /* remove carriage return */
    if ( c ) *c = '\0';
    printf("Your server is %s\n",host_name);

    p_h_ent = gethostbyname(host_name);
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
    ret = send( s, mess_buf, mess_len, 0);
    if(ret != mess_len)
    {
        perror( "Net_client: error in writing");
        exit(1);
    }

    for(;;)
    {
        printf("enter message: ");
        scanf("%s",neto_mess_ptr);
        mess_len = strlen(neto_mess_ptr) + sizeof(mess_len);
        memcpy( mess_buf, &mess_len, sizeof(mess_len) );

        ret = send( s, mess_buf, mess_len, 0);
        if(ret != mess_len)
        {
            perror( "Net_client: error in writing");
            exit(1);
        }
    }

    return 0;

}

