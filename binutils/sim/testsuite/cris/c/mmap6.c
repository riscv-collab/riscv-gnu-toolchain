/* Check that mmapping specifying a previously mmapped address without
   MAP_FIXED works; that we just don't get the same address.
#progos: linux
*/
#define NO_MUNMAP
#define MMAP_FLAGS2 MAP_PRIVATE
#define MMAP_TEST_BAD (a == b || MMAP_TEST_BAD_ORIG)
#include "mmap5.c"
