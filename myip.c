#include "net_include.h"

#define NAME_LENGTH 80

int gethostname(char*,size_t);

int main()
{
    struct hostent        h_ent;
    struct hostent        *p_h_ent;
    char                  my_name[NAME_LENGTH] = {'\0'};
    int                   my_ip;

    gethostname(my_name, NAME_LENGTH );
    printf("My host name is %s.\n", my_name);

    p_h_ent = gethostbyname(my_name);
    if ( p_h_ent == NULL ) {
        printf("myip: gethostbyname error.\n");
        exit(1);
    }

    memcpy( &h_ent, p_h_ent, sizeof(h_ent));
    memcpy( &my_ip, h_ent.h_addr_list[0], sizeof(my_ip) );

    printf("My IP address is: %d.%d.%d.%d\n", (htonl(my_ip) & 0xff000000)>>24, 
					      (htonl(my_ip) & 0x00ff0000)>>16,
					      (htonl(my_ip) & 0x0000ff00)>>8,
					      (htonl(my_ip) & 0x000000ff) );

    return(0);
}

