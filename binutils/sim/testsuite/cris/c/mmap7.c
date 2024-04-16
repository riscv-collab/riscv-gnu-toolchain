/* Check that mmapping a page-aligned size, larger than the file,
   works.

#progos: linux
*/

/* Make sure we get an address where the size fits.  */
#define MMAP_SIZE1 ((sb.st_size + 8192) & ~8191)

/* If this ever fails because the file is a page-multiple, we'll deal
   with that then.  We want it larger than the file-size anyway.  */
#define MMAP_SIZE2 ((size + 8192) & ~8191)
#define MMAP_FLAGS2 MAP_DENYWRITE | MAP_PRIVATE | MAP_FIXED
#include "mmap5.c"
