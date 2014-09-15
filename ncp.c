#include "net_include.h"

void sendFileName(char *file_name, char *comp_name);

int main(int argc, char const *argv[])
{
  int loss_rate_percent;
  char source_file_name[80];
  char dest_file_name[80];
  char comp_name[80];
  char *find_at_symbol_ptr;

  FILE *fr;
  int nread;

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

  sendFileName(dest_file_name, comp_name);

  /* Open the source file for reading */
  if((fr = fopen(source_file_name, "r")) == NULL) {
    perror("fopen");
    exit(0);
  }

  printf("%d %s %s %s\n", loss_rate_percent, source_file_name, dest_file_name, comp_name);
  return 0;
}

void sendFile(char *file_name, char *dest_file_name, char *comp_name, int loss_rate_percent) {
    struct sockaddr_in    name;
    struct sockaddr_in    send_addr;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    struct hostent        h_ent;
    struct hostent        *p_h_ent;
    char                  host_name[NAME_LENGTH] = {'\0'};
    char                  my_name[NAME_LENGTH] = {'\0'};
    int                   host_num;
    int                   from_ip;
    int                   ss,sr;
    fd_set                mask;
    fd_set                dummy_mask,temp_mask;
    int                   bytes;
    int                   num;
    char                  mess_buf[MAX_MESS_LEN];
    char                  input_buf[80];
    struct timeval        timeout;

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
    FD_SET( (long)0, &mask ); /* stdin */
}
