#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "ftpscan.h"

#define CRLF		"\r\n"
#define CRLF_LEN	2

static int ftp_read_banner(int fd);
static void strip(char *string);
static int response2code(const char *response);

int
ftp_login(int fd, const char *username, const char *password)
{
	char buffer[256];

	if(ftp_read_banner(fd))
		return -1;


	snprintf(buffer, sizeof(buffer), "USER %s", username);

	int code = ftp_exchange_command(fd, buffer);
	if(code != 331) {
		warn("Login failed.");
		return -1;
	}

	snprintf(buffer, sizeof(buffer), "PASS %s", password);

	if(!ftp_code_okay(ftp_exchange_command(fd, buffer))) {
		warn("Login failed.");
		return -1;
	}

	return 0;
}

int
ftp_anon_login(int fd)
{
	return ftp_login(fd, "ftp", "x@");
}

int
ftp_code_okay(int code)
{
	return code >= 200 && code < 300;
}

int
ftp_exchange_command(int fd, const char *command)
{
	debug("--> %s", command);
	write_all(fd, command, strlen(command));
	write_all(fd, CRLF, CRLF_LEN);

	return ftp_command_response(fd);

}

int
ftp_command_response(int fd)
{
	char response[1024];
	int n;

	if((n = recv(fd, response, sizeof(response), 0)) < 0) {
		error("recv() failed reading response from command.");
		return -1;
	}

	if(n == 0) {
		warn("EOF reading command response.");
		return -1;
	}

	response[n] = 0;
	strip(response);

	debug("<-- %s", response);

	return response2code(response);
}

static int 
ftp_read_banner(int fd)
{
	char buffer[1024];
	int n;

	if((n = recv(fd, buffer, sizeof(buffer), 0)) < 0) {
		error("recv() failed reading banner");
		return -1;
	}

	if(n == 0) {
		warn("Remote server closed connection.");
		return -1;
	}

	buffer[n] = 0;
	strip(buffer);

	info("Received banner: %s", buffer);

	return 0;
	
}

static void
strip(char *string)
{
	while(1) {
		int len = strlen(string);
		if(len > 0 && isspace(string[len - 1]))
			string[len - 1] = 0;
		else
			return;
	}
}

static int
response2code(const char *response)
{
	int code = 0;
	int i;

	for(i = 0; i < 3; i++) {
		code *= 10;
		if(!isdigit(response[i]))
			return -1;
		code += response[i] - '0';
	}

	if(response[3] != ' ')
		return -1;

	return code;
}
