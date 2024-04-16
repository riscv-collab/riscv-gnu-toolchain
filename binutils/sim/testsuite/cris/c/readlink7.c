/* Check that rare readlink calls don't cause the simulator to abort.
#progos: linux
#sim: --env-unset PWD
 */
#include "readlink2.c"
