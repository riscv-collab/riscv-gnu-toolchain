#include "stdio.h"

int foo(){
  return 11;
}

int bar(){
  return 12;
}

int bar(int){
  printf ("bar(int)\n");
  return 13;
}

int bar(char){
  printf ("bar(char)\n");
  return 14;
}

int (*p1)() = &foo;
int (*p2)() = &bar;
int (*p[2])() = {p1,p2};

int (*p3)(int) = &bar;
int (*p4)(char) = &bar;

int main ()
{
  p1 ();
  p2 ();

  p[0]();
  p[1]();

  p3 ('a');
  p4 (1);

  return 0;
}
