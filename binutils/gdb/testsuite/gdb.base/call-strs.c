#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unbuffer_output.c"

char buf[100];
char bigbuf[1000];
char * s;

char * str_func1(char *s1)
{
  printf("first string arg is: %s\n", s1);
  strcpy(bigbuf, s1);
  return bigbuf;
}

char * str_func(
char * s1, 
char * s2,
char * s3,
char * s4,
char * s5,
char * s6,
char * s7)
{
  printf("first string arg is: %s\n", s1);
  printf("second string arg is: %s\n", s2);
  printf("third string arg is: %s\n", s3);
  printf("fourth string arg is: %s\n", s4);
  printf("fifth string arg is: %s\n", s5);
  printf("sixth string arg is: %s\n", s6);
  printf("seventh string arg is: %s\n", s7);
  strcpy(bigbuf, s1);
  strcat(bigbuf, s2);
  strcat(bigbuf, s3);
  strcat(bigbuf, s4);
  strcat(bigbuf, s5);
  strcat(bigbuf, s6);
  strcat(bigbuf, s7);
  return bigbuf;
}

char *
link_malloc ()
{
  return (char*) malloc (1);
}

int main()
{
  gdb_unbuffer_output ();

  s = &buf[0];
  strcpy(buf, "test string");
  str_func("abcd", "efgh", "ijkl", "mnop", "qrst", "uvwx", "yz12");
  str_func1("abcd");
  return 0;
}

