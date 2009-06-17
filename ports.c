#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ftpscan.h"

#define MAX_PORT		65535
static char port_map[MAX_PORT + 1];
static int nxt_port = 1;

static void process_tok(char *);
static void process_port(int n);
static void process_range(int, int);
int
ports_initialize(char *ports_string)
{
	char *tok;
	for(tok = strtok(ports_string, ","); tok; tok = strtok(NULL, ",")) 
		process_tok(tok);

	return 0;
}

static void
process_tok(char *tok)
{
	char *p;

	if((p = index(tok, '-')) != NULL) {
		*p++ = 0;
		process_range(atoi(tok), atoi(p));
		return;
	}
	process_port(atoi(tok));
		
}

static void
process_port(int n)
{
	if(n > 0 && n <= MAX_PORT)
		port_map[n] = 1;
	debug("added port %d", n);
}

static void
process_range(int low, int high)
{
	int i;

	for(i = low; i <= high; i++)
		process_port(i);
}

int
next_port()
{
	while(1) {
		if(nxt_port > MAX_PORT)
			return 0;
		if(port_map[nxt_port]) 
			return nxt_port++;
	
		nxt_port++;
	}
}
