/* Application Data Integrity (ADI) test in sparc64.

   Copyright 2017-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <malloc.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <setjmp.h>
#include "adi.h"

#define	ONEKB		1024
#define PAT 		0xdeadbeaf

#define MAPSIZE 8192
#define SHMSIZE 102400
#ifndef PROT_ADI
#define PROT_ADI 0x10
#endif

static int
memory_fill (char *addr, size_t size, int pattern)
{
  long *aligned_addr = (long *) addr;
  long i;
  for (i = 0; i < size / sizeof (long); i += ONEKB) 
    { 
      *aligned_addr = pattern; 
      aligned_addr = aligned_addr + ONEKB;
    }
  return (0);
}

int main ()
{
  char *haddr;
  caddr_t vaddr;
  int	version;

  /* Test ISM. */
  int shmid = shmget (IPC_PRIVATE, SHMSIZE, IPC_CREAT | 0666);
  if (shmid == -1) 
    exit(1);
  char *shmaddr = (char *)shmat (shmid, NULL, 0x666 | SHM_RND);
  if (shmaddr == (char *)-1) 
    { 
      shmctl (shmid, IPC_RMID, NULL); 
      exit(1);
    }
  /* Enable ADI on ISM segment. */
  if (mprotect (shmaddr, SHMSIZE, PROT_READ|PROT_WRITE|PROT_ADI)) 
    { 
      perror ("mprotect failed"); 
      goto err_out; 
    }
  if (memory_fill (shmaddr, SHMSIZE, PAT) != 0) /* line breakpoint here */
    {
      exit(1); 
    }
  adi_clr_version (shmaddr, SHMSIZE);
  caddr_t vshmaddr = adi_set_version (shmaddr, SHMSIZE, 0x8);
  if (vshmaddr == 0) 
    exit(1);
  /* Test mmap. */ 
  int fd = open ("/dev/zero", O_RDWR);
  if (fd < 0)
    exit(1);
  char *maddr = (char *)mmap (NULL, MAPSIZE, PROT_READ|PROT_WRITE, 
                              MAP_PRIVATE, fd, 0);
  if (maddr == (char *)-1) 
    exit(1); 
  /* Enable ADI. */
  if (mprotect (shmaddr, MAPSIZE, PROT_READ|PROT_WRITE|PROT_ADI)) 
    { 
      perror ("mprotect failed"); 
      goto err_out;
   
    }
  if (memory_fill (maddr, MAPSIZE, PAT) != 0) 
    exit(1); 
  caddr_t vmaddr = adi_set_version (maddr, MAPSIZE, 0x8);

  /* Test heap. */
  haddr = (char*) memalign (MAPSIZE, MAPSIZE);
  /* Enable ADI. */
  if (mprotect (shmaddr, MAPSIZE, PROT_READ|PROT_WRITE|PROT_ADI)) 
    { 
      perror ("mprotect failed"); 
      goto err_out;
    }

  if (memory_fill (haddr, MAPSIZE, PAT) != 0) 
    exit(1);
  adi_clr_version (haddr, MAPSIZE);
  /* Set some ADP version number. */
  caddr_t vaddr1, vaddr2, vaddr3, vaddr4;
  vaddr = adi_set_version (haddr, 64*2, 0x8);
  vaddr1 = adi_set_version (haddr+64*2, 64*2, 0x9);
  vaddr2 = adi_clr_version (haddr+64*4, 64*2);
  vaddr3 = adi_set_version (haddr+64*6, 64*2, 0xa);
  vaddr4 = adi_set_version (haddr+64*8, 64*10, 0x3);
  if (vaddr == 0) 
    exit(1);
  char *versioned_p = vaddr;
  *versioned_p = 'a';
  char *uvp = haddr;	// unversioned pointer
  *uvp = 'b';		// version mismatch trap

  return (0);
err_out: 
  if (shmdt ((const void *)shmaddr) != 0) 
    perror ("Detach failure");
  shmctl (shmid, IPC_RMID, NULL);
  exit (1);
}
