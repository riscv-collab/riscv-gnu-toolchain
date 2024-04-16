/* As the included file, but specifying silent ENOSYS.
#progos: linux
#sim: --cris-unknown-syscall=enosys-quiet
#output: ENOSYS\n
#output: xyzzy\n
*/

#include "syscall2.c"
