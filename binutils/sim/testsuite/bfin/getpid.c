/* Basic getpid tests.
# mach: bfin
# cc: -msim
*/

#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  pid_t pid = getpid();
  if (pid < 0) {
    perror("getpid failed");
    return 1;
  }
  puts("pass");
  return 0;
}
