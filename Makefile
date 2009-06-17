all: ftpscan

OBJS=ftpscan.o socket.o msg.o io.o ftp.o ports.o

LD=gcc
CC=gcc
CFLAGS=-Wall

.c.o:
	$(CC) $(CFLAGS) -c $<

ftpscan: $(OBJS)
	$(LD) -o $@ $(OBJS)

clean:
	rm -f *.o ftpscan
