/*
#progos: linux
*/

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

int main (int argc, char *argv[])
{
  int fd = open (argv[0], O_RDONLY);
  struct stat sb;
  int size;
  void *a;
  void *b;
  const char *str = "a string you'll only find in the program";

  if (fd == -1)
    {
      perror ("open");
      abort ();
    }

  if (fstat (fd, &sb) < 0)
    {
      perror ("fstat");
      abort ();
    }

  size = 8192;
#ifdef MMAP_SIZE1
  size = MMAP_SIZE1;
#endif

#ifndef MMAP_PROT1
#define MMAP_PROT1 PROT_READ | PROT_WRITE | PROT_EXEC
#endif

#ifndef MMAP_FLAGS1
#define MMAP_FLAGS1 MAP_PRIVATE | MAP_ANONYMOUS
#endif

  /* Get a page, any page.  */
  b = mmap (NULL, size, MMAP_PROT1, MMAP_FLAGS1, -1, 0);
  if (b == MAP_FAILED)
    abort ();

  /* Remember it, unmap it.  */
#ifndef NO_MUNMAP
  if (munmap (b, size) != 0)
    abort ();
#endif

#ifdef MMAP_ADDR2
  b = MMAP_ADDR2;
#endif

#ifndef MMAP_PROT2
#define MMAP_PROT2 PROT_READ | PROT_EXEC
#endif

#ifndef MMAP_FLAGS2
#define MMAP_FLAGS2 MAP_DENYWRITE | MAP_FIXED | MAP_PRIVATE
#endif

  size = sb.st_size;
#ifdef MMAP_SIZE2
  size = MMAP_SIZE2;
#endif

#define MMAP_TEST_BAD_ORIG \
 (a == MAP_FAILED || memmem (a, size, str, strlen (str) + 1) == NULL)
#ifndef MMAP_TEST_BAD
#define MMAP_TEST_BAD MMAP_TEST_BAD_ORIG
#endif

  /* Try mapping the now non-mapped page fixed.  */
  a = mmap (b, size, MMAP_PROT2, MMAP_FLAGS2, fd, 0);

  if (MMAP_TEST_BAD)
    abort ();

  printf ("pass\n");
  exit (0);
}
