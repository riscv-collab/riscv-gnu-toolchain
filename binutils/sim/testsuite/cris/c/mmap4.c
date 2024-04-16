/* Just check that MAP_DENYWRITE is "honored" (ignored).
#progos: linux
*/
#define MMAP_FLAGS (MAP_PRIVATE|MAP_DENYWRITE)
#include "mmap1.c"
