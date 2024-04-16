/* Check that mmapping 0 using MAP_FIXED works, both with/without
   there being previously mmapped contents.
#progos: linux
*/
#define MMAP_FLAGS1 MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED
#define NO_MUNMAP
#define MMAP_SIZE2 8192
#define MMAP_TEST_BAD (a != b || a != 0)
#include "mmap5.c"
