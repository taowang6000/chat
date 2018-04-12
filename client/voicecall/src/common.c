
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

int  newline;
char msgbuf[MBS];

void need_newline()
{
	if (newline == 0) {
		fprintf(stderr, "\n");
		newline = 1;
	}
}

void print_msg_nl(char *msg)
{
	need_newline();
	if (msg)
		fprintf(stderr, "%s", msg);
}

void die(char *msg, int exit_code)
{
	print_msg_nl(msg);
	exit(exit_code);
}
