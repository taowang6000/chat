
#ifndef COMMON_H
#define COMMON_H

#define MBS 256

extern int  newline;
extern char msgbuf[MBS];

void need_newline();
void print_msg_nl(char *msg);
void die(char *msg, int exit_code);

#endif
