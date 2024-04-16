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


#ifndef _HW_SHM_C_
#define _HW_SHM_C_

#include "device_table.h"

#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>


/* DEVICE


   shm - map unix shared memory into psim address space


   DESCRIPTION


   This device implements an area of memory which is mapped into UNIX
   shared memory.


   PROPERTIES


   reg = <address> <size> (required)

   Determine where the memory lives in the parents address space.
   The SHM area is assumed to be of the same length.

   key = <integer> (required)

   This is the key of the unix shared memory area.

   EXAMPLES


   Enable tracing of the shm:

   |  bash$ psim -t shm-device \


   Configure a 512 kilobytes of UNIX shared memory with the key 0x12345678
   mapped into psim address space at 0x0c000000.

   |  -o '/shm@0x0c000000/reg 0x0c000000 0x80000' \
   |  -o '/shm@0x0c000000/key 0x12345678' \

   sim/ppc/run -o '/#address-cells 1' \
         -o '/shm@0x0c000000/reg 0x0c000000 0x80000' \
         -o '/shm@0x0c000000/key 0x12345678' ../psim-hello/hello

   BUGS

   None known.

   */

#ifdef HAVE_SYSV_SHM

typedef struct _hw_shm_device {
  unsigned_word physical_address;
  char *shm_address;
  unsigned sizeof_memory;
  key_t key;
  int id;
} hw_shm_device;

static void
hw_shm_init_data(device *me)
{
  hw_shm_device *shm = (hw_shm_device*)device_data(me);
  reg_property_spec reg;
  int i;

  /* Obtain the Key Value */
  if (device_find_property(me, "key") == NULL)
    error("shm_init_data() required key property is missing\n");

  shm->key = (key_t) device_find_integer_property(me, "key");
  DTRACE(shm, ("shm key (0x%08x)\n", shm->key) );
  
  /* Figure out where this memory is in address space and how long it is */
  if ( !device_find_reg_array_property(me, "reg", 0, &reg) )
    error("hw_shm_init_data() no address registered\n");

  /* Determine the address and length being as paranoid as possible */
  shm->physical_address = 0xffffffff;
  shm->sizeof_memory = 0xffffffff;

  for ( i=0 ; i<reg.address.nr_cells; i++ ) {
    if (reg.address.cells[0] == 0 && reg.size.cells[0] == 0)
      continue;

    if ( shm->physical_address != 0xffffffff )
      device_error(me, "Only single celled address ranges supported\n");

    shm->physical_address = reg.address.cells[i];
    DTRACE(shm, ("shm physical_address=0x%x\n", shm->physical_address));

    shm->sizeof_memory = reg.size.cells[i];
    DTRACE(shm, ("shm length=0x%x\n", shm->sizeof_memory));
  }

  if ( shm->physical_address == 0xffffffff )
    device_error(me, "Address not specified\n" );

  if ( shm->sizeof_memory == 0xffffffff )
    device_error(me, "Length not specified\n" );

  /* Now actually attach to or create the shared memory area */
  shm->id = shmget(shm->key, shm->sizeof_memory, IPC_CREAT | 0660);
  if (shm->id == -1)
    error("hw_shm_init_data() shmget failed\n");

  shm->shm_address = shmat(shm->id, (char *)0, SHM_RND);
  if (shm->shm_address == (void *)-1)
    error("hw_shm_init_data() shmat failed\n");
}

static void
hw_shm_attach_address_callback(device *me,
				attach_type attach,
				int space,
				unsigned_word addr,
				unsigned nr_bytes,
				access_type access,
				device *client) /*callback/default*/
{
  if (space != 0)
    error("shm_attach_address_callback() invalid address space\n");

  if (nr_bytes == 0)
    error("shm_attach_address_callback() invalid size\n");
}


static unsigned
hw_shm_io_read_buffer(device *me,
			 void *dest,
			 int space,
			 unsigned_word addr,
			 unsigned nr_bytes,
			 cpu *processor,
			 unsigned_word cia)
{
  hw_shm_device *shm = (hw_shm_device*)device_data(me);

  /* do we need to worry about out of range addresses? */

  DTRACE(shm, ("read %p %x %x %x\n", \
     shm->shm_address, shm->physical_address, addr, nr_bytes) );

  memcpy(dest, &shm->shm_address[addr - shm->physical_address], nr_bytes);
  return nr_bytes;
}


static unsigned
hw_shm_io_write_buffer(device *me,
			  const void *source,
			  int space,
			  unsigned_word addr,
			  unsigned nr_bytes,
			  cpu *processor,
			  unsigned_word cia)
{
  hw_shm_device *shm = (hw_shm_device*)device_data(me);

  /* do we need to worry about out of range addresses? */

  DTRACE(shm, ("write %p %x %x %x\n", \
     shm->shm_address, shm->physical_address, addr, nr_bytes) );

  memcpy(&shm->shm_address[addr - shm->physical_address], source, nr_bytes);
  return nr_bytes;
}

static device_callbacks const hw_shm_callbacks = {
  { generic_device_init_address, hw_shm_init_data },
  { hw_shm_attach_address_callback, }, /* address */
  { hw_shm_io_read_buffer,
    hw_shm_io_write_buffer }, /* IO */
  { NULL, }, /* DMA */
  { NULL, }, /* interrupt */
  { NULL, }, /* unit */
  NULL,
};

static void *
hw_shm_create(const char *name,
		 const device_unit *unit_address,
		 const char *args)
{
  hw_shm_device *shm = ZALLOC(hw_shm_device);
  return shm;
}



const device_descriptor hw_shm_device_descriptor[] = {
  { "shm", hw_shm_create, &hw_shm_callbacks },
  { NULL },
};

#else

const device_descriptor hw_shm_device_descriptor[] = {
  { NULL },
};

#endif /* HAVE_SYSV_SHM */

#endif /* _HW_SHM_C_ */
