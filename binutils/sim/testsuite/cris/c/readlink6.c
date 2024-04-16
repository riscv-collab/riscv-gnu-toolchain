/* Check that rare readlink calls don't cause the simulator to abort.
#progos: linux
#sim: --argv0 $pwd/readlink6.c.x
*/
#include "readlink2.c"
