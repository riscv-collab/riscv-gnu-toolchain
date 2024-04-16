/* As the included file, but specifying silent ENOSYS.
#progos: linux
#cc: additional_flags=-pthread
#sim: --cris-unknown-syscall=enosys-quiet
#output: ENOSYS\n
#output: xyzzy\n
*/

#include "sigreturn2.c"
