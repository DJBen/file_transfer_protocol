CC=gcc

CFLAGS = -ansi -c -Wall -pedantic

all: ucast net_client net_server myip file_copy ncp rcv t_ncp t_rcv

rcv: rcv.o sendto_dbg.o
			$(CC) -o rcv rcv.o sendto_dbg.o

ncp: ncp.o queue.o sendto_dbg.o
			$(CC) -o ncp ncp.o queue.o sendto_dbg.o

net_server: net_server.o
	    $(CC) -o net_server net_server.o

net_client: net_client.o
	    $(CC) -o net_client net_client.o

ucast: ucast.o
	    $(CC) -o ucast ucast.o

myip: myip.o
	    $(CC) -o myip myip.o

file_copy: file_copy.o
	    $(CC) -o file_copy file_copy.o

t_ncp: t_ncp.o
			$(CC) -o t_ncp t_ncp.o

t_rcv: t_rcv.o
			$(CC) -o t_rcv t_rcv.o

clean:
	rm *.o
	rm net_server
	rm net_client
	rm ucast
	rm myip
	rm file_copy
	rm ncp
	rm rcv
	rm t_rcv
	rm t_ncp

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


