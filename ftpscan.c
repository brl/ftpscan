#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "ftpscan.h"

#define DEFAULT_TIMEOUT 	3000

#define TEST_SUCCESS		0
#define TEST_TIMEOUT		1
#define TEST_ERROR			3

static in_addr_t parse_target(const char *);
static int test_port(int fd, in_port_t port, int timeout);
static in_addr_t get_local_address(int);
static void run_scan();
static void create_port_command(char *, size_t, in_addr_t, in_port_t);
static void create_eprt_command(char *, size_t, in_addr_t, in_port_t);

static in_addr_t local_address;
static in_addr_t target_address;
static in_port_t target_port = 21;
static int use_eprt = 0;
static int verbose = 0;
static int connect_timeout = DEFAULT_TIMEOUT;

static void
usage(const char *progname)
{
	fprintf(stderr, "Usage: %s [options] target_ip port_range\n", progname);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, " -t timeout         Time (in milliseconds) to wait for data connection from server. [default %u]\n", DEFAULT_TIMEOUT);
	fprintf(stderr, " -p target_port     Target FTP port [default 21]\n");
	fprintf(stderr, " -x                 Use EPRT command instead of PORT\n");
	fprintf(stderr, " -v                 Enable verbose output\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	int ch;
	char *progname = argv[0];

	while((ch = getopt(argc, argv, "p:t:vx")) != -1) {
		switch(ch) {
			case 'v':
				verbose++;
				enable_debug(1);
				break;
			case 'p':
				target_port = atoi(optarg);
				break;
			case 't':
				connect_timeout = atoi(optarg);
			case 'x':
				use_eprt = 1;
				break;
			case '?':
			default:
				usage(argv[0]);
		}
	}
	argc -= optind;
	argv += optind;

	if(argc < 2)
		usage(progname);

	target_address = parse_target(argv[0]);
	ports_initialize(argv[1]);
	run_scan();
	return 0;
}

static in_addr_t
parse_target(const char *target)
{
	in_addr_t target_ip = inet_addr(target);
	if(target_ip != INADDR_NONE)
		return target_ip;

	struct hostent *hp = gethostbyname(target);
	if(hp == NULL) {
		errno = 0;
		fatal("Could not resolve '%s'", target);
	}

	memcpy(&target_ip, hp->h_addr, sizeof(target_ip));

	struct in_addr in;
	in.s_addr = target_ip;
	debug("Resolved %s to %s", target, inet_ntoa(in));

	return target_ip;
}

static void
run_scan()
{
	int fd = connect_server(target_address, target_port);
	if(fd < 0)
		exit(EXIT_FAILURE);

	local_address = get_local_address(fd);

	if(ftp_anon_login(fd)) {
		warn("Login failed.");
		close(fd);
		exit(EXIT_FAILURE);
	}
	info("Logged in.");
	int i;
	for(i = next_port(); i != 0; i = next_port()) {
		fprintf(stderr, "[+] Testing port %d\t", i);
		if(verbose) fprintf(stderr, "...\n");
		switch(test_port(fd, i, connect_timeout)) {
		case TEST_SUCCESS:
			fprintf(stderr, "OPEN!\n");
			break;
		case TEST_ERROR:
			fprintf(stderr, "ERROR (%s)\n", ftp_get_last_server_message());
			break;
		case TEST_TIMEOUT:
			switch(test_port(fd, i, connect_timeout * 2)) {
			case TEST_SUCCESS:
				fprintf(stderr, "OPEN!\n");
				break;
			case TEST_ERROR:
				fprintf(stderr, "ERROR (%s)\n", ftp_get_last_server_message());
				break;
			case TEST_TIMEOUT:
				fprintf(stderr, "BLOCKED (time out)\n");
				break;
			}
		}
	}
	ftp_exchange_command(fd, "QUIT");
}

static in_addr_t
get_local_address(int fd)
{
	struct sockaddr_in sin;
	socklen_t sinlen = sizeof(sin);

	if(getsockname(fd, (struct sockaddr *)&sin, &sinlen) < 0)
		fatal("getsockname() failed");

	debug("Local IP address is %s", inet_ntoa(sin.sin_addr));

	return sin.sin_addr.s_addr;
}

static int
test_port(int fd, in_port_t port, int timeout)
{
	char buffer[256];

	if(use_eprt)
		create_eprt_command(buffer, sizeof(buffer), local_address, port);
	else
		create_port_command(buffer, sizeof(buffer), local_address, port);

	int code = ftp_exchange_command(fd, buffer);

	if(!ftp_code_okay(code))
		return TEST_ERROR;

	int s = listen_port(port);

	code = ftp_exchange_command(fd, "LIST");

	if(code != 150)
		warn("Unexpected response code from LIST: %d", code);

	int s2 = wait_accept(s, timeout);
	close(s);

	if(s2 == 0)
		return TEST_TIMEOUT;

	if(s2 == -1)
		return TEST_ERROR;

	drain_all(s2);
	ftp_command_response(fd);
	ftp_drain_extra_responses(fd);
	return TEST_SUCCESS;
}


static void
create_port_command(char *buffer, size_t size, in_addr_t address, in_port_t port)
{
	int h1 = address & 0xFF;
	int h2 = (address >> 8) & 0xFF;
	int h3 = (address >> 16) & 0xFF;
	int h4 = (address >> 24) & 0xFF;
	int p1 = (port >> 8) & 0xFF;
	int p2 = port & 0xFF;
	snprintf(buffer, size, "PORT %u,%u,%u,%u,%u,%u", h1,h2,h3,h4,p1,p2);
}

static void
create_eprt_command(char *buffer, size_t size, in_addr_t address, in_port_t port)
{
	struct in_addr in;
	in.s_addr = address;
	snprintf(buffer, size, "EPRT |1|%s|%u|", inet_ntoa(in), port);
}
