#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

static void _prompt(const char *prompt);
int debug_flag = 0;

void
enable_debug(int val) 
{
	debug_flag = (val)?(1):(0);
}

void
fatal(const char *fmt, ...)
{
	int saved_errno = errno;
	va_list args;
	_prompt("X");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	if(saved_errno != 0) 
		fprintf(stderr, " : %s", strerror(saved_errno));

	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

void
warn(const char *fmt, ...)
{
	va_list args;
	_prompt("*");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void
error(const char *fmt, ...) 
{
	int saved_errno = errno;
	va_list args;
	_prompt("!");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	if(saved_errno != 0)
		fprintf(stderr, " : %s", strerror(saved_errno));
	fprintf(stderr, "\n");
}

void
debug(const char *fmt, ...)
{
	va_list args;
	if(!debug_flag)
		return;
	_prompt(">");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void
info(const char *fmt, ...)
{
        va_list args;
        _prompt(".");
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
}

void
xinfo(const char *prompt, const char *fmt, ...) 
{
	va_list args;
	_prompt(prompt);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}

static void
_prompt(const char *prompt)
{
	fprintf(stderr, "[%s] ", prompt);
}

