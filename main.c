
#include "defs.h"

extern void trek(void); // from trek.o

void main(void) {
  com_init();
  trek();
  halt();
}