#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

#include "ftpscan.h"

int
drain_all(int fd)
{
	char recv_buffer[1024];
	int n;
	size_t total = 0;

	while(1) {
		if((n = recv(fd, recv_buffer, sizeof(recv_buffer), 0)) < 0) {
			error("recv() failed while draining data");
			close(fd);
			return -1;
		}
		if(n == 0) {
			debug("Drained %d bytes.", total);
			close(fd);
			return 0;
		}
		total += n;
	}
}

void
write_all(int fd, const void *buf, size_t count)
{
  int n;
  const char *p = buf;
  while(count) {
    if((n = write(fd, p, count)) < 0)
      fatal("write() failed.");
    count -= n;
    p += n;
  }
}
