#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  printf("pid %d\n", getpid());
  char pid_act[64];
  char * pid = &pid_act[64];

  sprintf(pid, "%ld", (long)getpid());
  printf("pid: %s\n", pid);
  return 0;
}
