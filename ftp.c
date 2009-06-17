#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "ftpscan.h"

#define CRLF		"\r\n"
#define CRLF_LEN	2

static int ftp_read_banner(int fd);
static char *ftp_read_line(int fd);
static void strip(char *string);
static int response2code(const char *response);

static char *last_server_message;

int
ftp_login(int fd, const char *username, const char *password)
{
	char buffer[256];

	if(ftp_read_banner(fd))
		return -1;


	snprintf(buffer, sizeof(buffer), "USER %s", username);

	int code = ftp_exchange_command(fd, buffer);
	if(code == 230)
		return 0;

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
	return ftp_login(fd, "ftp", "IEUser@");
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
	char *response = ftp_read_line(fd);

	debug("<-- %s", response);

	if(strlen(response) > 4)
		last_server_message = response + 4;
	else
		last_server_message = "";

	return response2code(response);
}

char *
ftp_get_last_server_message()
{
	return last_server_message;
}

static char *
ftp_read_line(int fd)
{
	static char line_buffer[1024];
	char c;
	int n;
	size_t count = 0;
	size_t remaining = sizeof(line_buffer) - 1;

	while(remaining > 0) {
		if((n = recv(fd, &c, 1, 0)) < 0)
			fatal("recv() failed reading line from server.");

		if(n == 0)
			fatal("EOF reading line from server.");

		if(c == '\r')
			continue;
		if(c == '\n')
			break;
		line_buffer[count] = c;
		count++;
		remaining--;
	}
	if(remaining == 0)
		warn("No newline found in response after reading %d bytes.", count);

	line_buffer[count] = 0;
	strip(line_buffer);
	return line_buffer;
}

static int
ftp_read_banner(int fd)
{
	info("Received banner: %s", ftp_read_line(fd));
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
