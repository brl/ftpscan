#ifndef _FTPSCAN_H_
#define _FTPSCAN_H_

#include <sys/types.h>
#include <arpa/inet.h>

int connect_server(in_addr_t address, in_port_t port);
int listen_port(in_port_t port);
int set_nonblocking(int fd);

int drain_all(int fd);
void write_all(int fd, const void *buffer, size_t count);

int ftp_login(int fd, const char *username, const char *password);
int ftp_anon_login(int fd);
int ftp_code_okay(int code);
int ftp_exchange_command(int fd, const char *command);
int ftp_command_response(int fd);

int ports_initialize(char *ports_string);
int next_port();

void enable_debug(int val);
void fatal(const char *fmt, ...);
void warn(const char *fmt, ...);
void error(const char *fmt, ...);
void debug(const char *fmt, ...);
void info(const char *fmt, ...);
void xinfo(const char *prompt, const char *fmt, ...);
#endif
