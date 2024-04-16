/*  This file is part of the program psim.

    Copyright (C) 1997,2008, Joel Sherrill <joel@OARcorp.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, see <http://www.gnu.org/licenses/>.
 
    */


#ifndef _HW_SEM_C_
#define _HW_SEM_C_

#include "device_table.h"

#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include <errno.h>

/* DEVICE


   sem - provide access to a unix semaphore


   DESCRIPTION


   This device implements an interface to a unix semaphore.


   PROPERTIES


   reg = <address> <size> (required)

   Determine where the memory lives in the parents address space.

   key = <integer> (required)

   This is the key of the unix semaphore.

   EXAMPLES


   Enable tracing of the sem:

   |  bash$ psim -t sem-device \


   Configure a UNIX semaphore using key 0x12345678 mapped into psim
   address space at 0xfff00000:

   |  -o '/sem@0xfff00000/reg 0xfff00000 0x80000' \
   |  -o '/sem@0xfff00000/key 0x12345678' \

   sim/ppc/run -o '/#address-cells 1' \
         -o '/sem@0xfff00000/reg 0xfff00000 12' \
         -o '/sem@0xfff00000/key 0x12345678' ../psim-hello/hello

   REGISTERS

   offset 0 - lock count
   offset 4 - lock operation
   offset 8 - unlock operation

   All reads return the current or resulting count.

   BUGS

   None known.

   */

#ifdef HAVE_SYSV_SEM

typedef struct _hw_sem_device {
  unsigned_word physical_address;
  key_t key;
  int id;
  int initial;
  int count;
} hw_sem_device;

#ifndef HAVE_UNION_SEMUN
union semun {
  int val;
  struct semid_ds *buf;
  unsigned short int *array;
#if defined(__linux__)
  struct seminfo *__buf;
#endif
};
#endif

static void
hw_sem_init_data(device *me)
{
  hw_sem_device *sem = (hw_sem_device*)device_data(me);
  const device_unit *d;
  int status;
  union semun help = {};

  /* initialize the properties of the sem */

  if (device_find_property(me, "key") == NULL)
    error("sem_init_data() required key property is missing\n");

  if (device_find_property(me, "value") == NULL)
    error("sem_init_data() required value property is missing\n");

  sem->key = (key_t) device_find_integer_property(me, "key");
  DTRACE(sem, ("semaphore key (%d)\n", sem->key) );

  sem->initial = (int) device_find_integer_property(me, "value");
  DTRACE(sem, ("semaphore initial value (%d)\n", sem->initial) );

  d = device_unit_address(me);
  sem->physical_address = d->cells[ d->nr_cells-1 ];
  DTRACE(sem, ("semaphore physical_address=0x%x\n", sem->physical_address));

  /* Now to initialize the semaphore */

  if ( sem->initial != -1 ) {

    sem->id = semget(sem->key, 1, IPC_CREAT | 0660);
    if (sem->id == -1)
      error("hw_sem_init_data() semget failed\n");

    help.val = sem->initial;
    status = semctl( sem->id, 0, SETVAL, help );
    if (status == -1)
      error("hw_sem_init_data() semctl -- set value failed\n");

  } else {
    sem->id = semget(sem->key, 1, 0660);
    if (sem->id == -1)
      error("hw_sem_init_data() semget failed\n");
  }

  sem->count = semctl( sem->id, 0, GETVAL, help );
  if (sem->count == -1)
    error("hw_sem_init_data() semctl -- get value failed\n");
  DTRACE(sem, ("semaphore OS value (%d)\n", sem->count) );
}

static void
hw_sem_attach_address_callback(device *me,
				attach_type attach,
				int space,
				unsigned_word addr,
				unsigned nr_bytes,
				access_type access,
				device *client) /*callback/default*/
{
  hw_sem_device *sem = (hw_sem_device*)device_data(me);

  if (space != 0)
    error("sem_attach_address_callback() invalid address space\n");

  if (nr_bytes == 12)
    error("sem_attach_address_callback() invalid size\n");

  sem->physical_address = addr;
  DTRACE(sem, ("semaphore physical_address=0x%x\n", addr));
}

static unsigned
hw_sem_io_read_buffer(device *me,
			 void *dest,
			 int space,
			 unsigned_word addr,
			 unsigned nr_bytes,
			 cpu *processor,
			 unsigned_word cia)
{
  hw_sem_device *sem = (hw_sem_device*)device_data(me);
  struct sembuf sb;
  int status;
  uint32_t u32;
  union semun help = {};

  /* do we need to worry about out of range addresses? */

  DTRACE(sem, ("semaphore read addr=0x%x length=%d\n", addr, nr_bytes));

  if (!(addr >= sem->physical_address && addr <= sem->physical_address + 11))
    error("hw_sem_io_read_buffer() invalid address - out of range\n");

  if ((addr % 4) != 0)
    error("hw_sem_io_read_buffer() invalid address - alignment\n");

  if (nr_bytes != 4)
    error("hw_sem_io_read_buffer() invalid length\n");

  switch ( (addr - sem->physical_address) / 4 ) {

    case 0:  /* OBTAIN CURRENT VALUE */
      break; 

    case 1:  /* LOCK */
      sb.sem_num = 0;
      sb.sem_op  = -1;
      sb.sem_flg = 0;

      status = semop(sem->id, &sb, 1);
      if (status == -1) {
        perror( "hw_sem.c: lock" );
        error("hw_sem_io_read_buffer() sem lock\n");
      }

      DTRACE(sem, ("semaphore lock %d\n", sem->count));
      break; 

    case 2: /* UNLOCK */
      sb.sem_num = 0;
      sb.sem_op  = 1;
      sb.sem_flg = 0;

      status = semop(sem->id, &sb, 1);
      if (status == -1) {
        perror( "hw_sem.c: unlock" );
        error("hw_sem_io_read_buffer() sem unlock\n");
      }
      DTRACE(sem, ("semaphore unlock %d\n", sem->count));
      break; 

    default:
      error("hw_sem_io_read_buffer() invalid address - unknown error\n");
      break; 
  }

  /* assume target is big endian */
  u32 = H2T_4(semctl( sem->id, 0, GETVAL, help ));

  DTRACE(sem, ("semaphore OS value (%d)\n", u32) );
  if (u32 == 0xffffffff) {
    perror( "hw_sem.c: getval" );
    error("hw_sem_io_read_buffer() semctl -- get value failed\n");
  }

  memcpy(dest, &u32, nr_bytes);
  return nr_bytes;

}

static device_callbacks const hw_sem_callbacks = {
  { generic_device_init_address, hw_sem_init_data },
  { hw_sem_attach_address_callback, }, /* address */
  { hw_sem_io_read_buffer, NULL }, /* IO */
  { NULL, }, /* DMA */
  { NULL, }, /* interrupt */
  { NULL, }, /* unit */
  NULL,
};

static void *
hw_sem_create(const char *name,
		 const device_unit *unit_address,
		 const char *args)
{
  hw_sem_device *sem = ZALLOC(hw_sem_device);
  return sem;
}

const device_descriptor hw_sem_device_descriptor[] = {
  { "sem", hw_sem_create, &hw_sem_callbacks },
  { NULL },
};

#else

const device_descriptor hw_sem_device_descriptor[] = {
  { NULL },
};

#endif /* HAVE_SYSV_SEM */

#endif /* _HW_SEM_C_ */
