/* Check that unsupported readlink calls don't cause the simulator to abort.
#progos: linux
#sim: --argv0 ./readlink5.c.x
#xerror:
#output: Unimplemented readlink syscall (*)\n
#output: program stopped with signal 4 (*).\n
*/
#include "readlink2.c"
