

#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <termio.h>
#include <fcntl.h>
#include <thread>
int main(){
  
    pid_t pid;

  int status;
  char *_argv[4];
  pid = fork();
  _argv[0] = "./voice\0";
  _argv[1] = "-a\0";
  _argv[2] = "-l\0";
  _argv[3] = NULL;

  if (pid == 0) {
	if (execvp(_argv[0], _argv) == -1) {
         perror("myvoice");
	}
     exit(EXIT_FAILURE);
  } else if (pid < 0) {
    perror("myvoice");
  } //else {
   // do {
	//	waitpid(pid, &status, WUNTRACED);
//	}while (!WIFEXITED(status) && !WIFSIGNALED(status));
  

  return 0;
}
