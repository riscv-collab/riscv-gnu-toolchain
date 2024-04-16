/* Common Flash Memory Interface (CFI) model.
   http://www.spansion.com/Support/AppNotes/CFI_Spec_AN_03.pdf
   http://www.spansion.com/Support/AppNotes/cfi_100_20011201.pdf

   Copyright (C) 2010-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

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

/* TODO: support vendor query tables.  */

/* This must come before any other includes.  */
#include "defs.h"

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "sim-main.h"
#include "hw-base.h"
#include "hw-main.h"
#include "dv-cfi.h"

/* Flashes are simple state machines, so here we cover all the
   different states a device might be in at any particular time.  */
enum cfi_state
{
  CFI_STATE_READ,
  CFI_STATE_READ_ID,
  CFI_STATE_CFI_QUERY,
  CFI_STATE_PROTECT,
  CFI_STATE_STATUS,
  CFI_STATE_ERASE,
  CFI_STATE_WRITE,
  CFI_STATE_WRITE_BUFFER,
  CFI_STATE_WRITE_BUFFER_CONFIRM,
};

/* This is the structure that all CFI conforming devices must provided
   when asked for it.  This allows a single driver to dynamically support
   different flash geometries without having to hardcode specs.

   If you want to start mucking about here, you should just grab the
   CFI spec and review that (see top of this file for URIs).  */
struct cfi_query
{
  /* This is always 'Q' 'R' 'Y'.  */
  unsigned char qry[3];
  /* Primary vendor ID.  */
  unsigned char p_id[2];
  /* Primary query table address.  */
  unsigned char p_adr[2];
  /* Alternate vendor ID.  */
  unsigned char a_id[2];
  /* Alternate query table address.  */
  unsigned char a_adr[2];
  union
  {
    /* Voltage levels.  */
    unsigned char voltages[4];
    struct
    {
      /* Normal min voltage level.  */
      unsigned char vcc_min;
      /* Normal max voltage level.  */
      unsigned char vcc_max;
      /* Programming min volage level.  */
      unsigned char vpp_min;
      /* Programming max volage level.  */
      unsigned char vpp_max;
    };
  };
  union
  {
    /* Operational timeouts.  */
    unsigned char timeouts[8];
    struct
    {
      /* Typical timeout for writing a single "unit".  */
      unsigned char timeout_typ_unit_write;
      /* Typical timeout for writing a single "buffer".  */
      unsigned char timeout_typ_buf_write;
      /* Typical timeout for erasing a block.  */
      unsigned char timeout_typ_block_erase;
      /* Typical timeout for erasing the chip.  */
      unsigned char timeout_typ_chip_erase;
      /* Max timeout for writing a single "unit".  */
      unsigned char timeout_max_unit_write;
      /* Max timeout for writing a single "buffer".  */
      unsigned char timeout_max_buf_write;
      /* Max timeout for erasing a block.  */
      unsigned char timeout_max_block_erase;
      /* Max timeout for erasing the chip.  */
      unsigned char timeout_max_chip_erase;
    };
  };
  /* Flash size is 2^dev_size bytes.  */
  unsigned char dev_size;
  /* Flash device interface description.  */
  unsigned char iface_desc[2];
  /* Max length of a single buffer write is 2^max_buf_write_len bytes.  */
  unsigned char max_buf_write_len[2];
  /* Number of erase regions.  */
  unsigned char num_erase_regions;
  /* The erase regions would now be an array after this point, but since
     it is dynamic, we'll provide that from "struct cfi" when requested.  */
  /*unsigned char erase_region_info;*/
};

/* Flashes may have regions with different erase sizes.  There is one
   structure per erase region.  */
struct cfi_erase_region
{
  unsigned blocks;
  unsigned size;
  unsigned start;
  unsigned end;
};

struct cfi;

/* Flashes are accessed via commands -- you write a certain number to
   a special address to change the flash state and access info other
   than the data.  Diff companies have implemented their own command
   set.  This structure abstracts the different command sets so that
   we can support multiple ones with just a single sim driver.  */
struct cfi_cmdset
{
  unsigned id;
  void (*setup) (struct hw *me, struct cfi *cfi);
  bool (*write) (struct hw *me, struct cfi *cfi, const void *source,
		 unsigned offset, unsigned value, unsigned nr_bytes);
  bool (*read) (struct hw *me, struct cfi *cfi, void *dest,
		unsigned offset, unsigned shifted_offset, unsigned nr_bytes);
};

/* The per-flash state.  Much of this comes from the device tree which
   people declare themselves.  See top of attach_cfi_regs() for more
   info.  */
struct cfi
{
  unsigned width, dev_size, status;
  enum cfi_state state;
  unsigned char *data, *mmap;

  struct cfi_query query;
  const struct cfi_cmdset *cmdset;

  unsigned char *erase_region_info;
  struct cfi_erase_region *erase_regions;
};

/* Helpful strings which are used with HW_TRACE.  */
static const char * const state_names[] =
{
  "READ", "READ_ID", "CFI_QUERY", "PROTECT", "STATUS", "ERASE", "WRITE",
  "WRITE_BUFFER", "WRITE_BUFFER_CONFIRM",
};

/* Erase the block specified by the offset into the given CFI flash.  */
static void
cfi_erase_block (struct hw *me, struct cfi *cfi, unsigned offset)
{
  unsigned i;
  struct cfi_erase_region *region;

  /* If no erase regions, then we can only do whole chip erase.  */
  /* XXX: Is this within spec ?  Or must there always be at least one ?  */
  if (!cfi->query.num_erase_regions)
    memset (cfi->data, 0xff, cfi->dev_size);

  for (i = 0; i < cfi->query.num_erase_regions; ++i)
    {
      region = &cfi->erase_regions[i];

      if (offset >= region->end)
	continue;

      /* XXX: Does spec require the erase addr to be erase block aligned ?
	      Maybe this is check is overly cautious ...  */
      offset &= ~(region->size - 1);
      memset (cfi->data + offset, 0xff, region->size);
      break;
    }
}

/* Depending on the bus width, addresses might be bit shifted.  This
   helps us normalize everything without cluttering up the rest of
   the code.  */
static unsigned
cfi_unshift_addr (struct cfi *cfi, unsigned addr)
{
  switch (cfi->width)
    {
    case 4: addr >>= 1; ATTRIBUTE_FALLTHROUGH;
    case 2: addr >>= 1;
    }
  return addr;
}

/* CFI requires all values to be little endian in its structure, so
   this helper writes a 16bit value into a little endian byte buffer.  */
static void
cfi_encode_16bit (unsigned char *data, unsigned num)
{
  data[0] = num;
  data[1] = num >> 8;
}

/* The functions required to implement the Intel command set.  */

static bool
cmdset_intel_write (struct hw *me, struct cfi *cfi, const void *source,
		    unsigned offset, unsigned value, unsigned nr_bytes)
{
  switch (cfi->state)
    {
    case CFI_STATE_READ:
    case CFI_STATE_READ_ID:
      switch (value)
	{
	case INTEL_CMD_ERASE_BLOCK:
	  cfi->state = CFI_STATE_ERASE;
	  break;
	case INTEL_CMD_WRITE:
	case INTEL_CMD_WRITE_ALT:
	  cfi->state = CFI_STATE_WRITE;
	  break;
	case INTEL_CMD_STATUS_CLEAR:
	  cfi->status = INTEL_SR_DWS;
	  break;
	case INTEL_CMD_LOCK_SETUP:
	  cfi->state = CFI_STATE_PROTECT;
	  break;
	default:
	  return false;
	}
      break;

    case CFI_STATE_ERASE:
      if (value == INTEL_CMD_ERASE_CONFIRM)
	{
	  cfi_erase_block (me, cfi, offset);
	  cfi->status &= ~(INTEL_SR_PS | INTEL_SR_ES);
	}
      else
	cfi->status |= INTEL_SR_PS | INTEL_SR_ES;
      cfi->state = CFI_STATE_STATUS;
      break;

    case CFI_STATE_PROTECT:
      switch (value)
	{
	case INTEL_CMD_LOCK_BLOCK:
	case INTEL_CMD_UNLOCK_BLOCK:
	case INTEL_CMD_LOCK_DOWN_BLOCK:
	  /* XXX: Handle the command.  */
	  break;
	default:
	  /* Kick out.  */
	  cfi->status |= INTEL_SR_PS | INTEL_SR_ES;
	  break;
	}
      cfi->state = CFI_STATE_STATUS;
      break;

    default:
      return false;
    }

  return true;
}

static bool
cmdset_intel_read (struct hw *me, struct cfi *cfi, void *dest,
		   unsigned offset, unsigned shifted_offset, unsigned nr_bytes)
{
  unsigned char *sdest = dest;

  switch (cfi->state)
    {
    case CFI_STATE_STATUS:
    case CFI_STATE_ERASE:
      *sdest = cfi->status;
      break;

    case CFI_STATE_READ_ID:
      switch (shifted_offset & 0x1ff)
	{
	case 0x00:	/* Manufacturer Code.  */
	  cfi_encode_16bit (dest, INTEL_ID_MANU);
	  break;
	case 0x01:	/* Device ID Code.  */
	  /* XXX: Push to device tree ?  */
	  cfi_encode_16bit (dest, 0xad);
	  break;
	case 0x02:	/* Block lock state.  */
	  /* XXX: This is per-block ...  */
	  *sdest = 0x00;
	  break;
	case 0x05:	/* Read Configuration Register.  */
	  cfi_encode_16bit (dest, (1 << 15));
	  break;
	default:
	  return false;
	}
      break;

    default:
      return false;
    }

  return true;
}

static void
cmdset_intel_setup (struct hw *me, struct cfi *cfi)
{
  cfi->status = INTEL_SR_DWS;
}

static const struct cfi_cmdset cfi_cmdset_intel =
{
  CFI_CMDSET_INTEL, cmdset_intel_setup, cmdset_intel_write, cmdset_intel_read,
};

/* All of the supported command sets get listed here.  We then walk this
   array to see if the user requested command set is implemented.  */
static const struct cfi_cmdset * const cfi_cmdsets[] =
{
  &cfi_cmdset_intel,
};

/* All writes to the flash address space come here.  Using the state
   machine, we figure out what to do with this specific write.  All
   common code sits here and if there is a request we can't process,
   we hand it off to the command set-specific write function.  */
static unsigned
cfi_io_write_buffer (struct hw *me, const void *source, int space,
		     address_word addr, unsigned nr_bytes)
{
  struct cfi *cfi = hw_data (me);
  const unsigned char *ssource = source;
  enum cfi_state old_state;
  unsigned offset, shifted_offset, value;

  offset = addr & (cfi->dev_size - 1);
  shifted_offset = cfi_unshift_addr (cfi, offset);

  if (cfi->width != nr_bytes)
    {
      HW_TRACE ((me, "write 0x%08lx length %u does not match flash width %u",
		 (unsigned long) addr, nr_bytes, cfi->width));
      return nr_bytes;
    }

  if (cfi->state == CFI_STATE_WRITE)
    {
      /* NOR flash can only go from 1 to 0.  */
      unsigned i;

      HW_TRACE ((me, "program %#x length %u", offset, nr_bytes));

      for (i = 0; i < nr_bytes; ++i)
	cfi->data[offset + i] &= ssource[i];

      cfi->state = CFI_STATE_STATUS;

      return nr_bytes;
    }

  value = ssource[0];

  old_state = cfi->state;

  if (value == CFI_CMD_READ || value == CFI_CMD_RESET)
    {
      cfi->state = CFI_STATE_READ;
      goto done;
    }

  switch (cfi->state)
    {
    case CFI_STATE_READ:
    case CFI_STATE_READ_ID:
      if (value == CFI_CMD_CFI_QUERY)
	{
	  if (shifted_offset == CFI_ADDR_CFI_QUERY_START)
	    cfi->state = CFI_STATE_CFI_QUERY;
	  goto done;
	}

      if (value == CFI_CMD_READ_ID)
	{
	  cfi->state = CFI_STATE_READ_ID;
	  goto done;
	}

      ATTRIBUTE_FALLTHROUGH;

    default:
      if (!cfi->cmdset->write (me, cfi, source, offset, value, nr_bytes))
	HW_TRACE ((me, "unhandled command %#x at %#x", value, offset));
      break;
    }

 done:
  HW_TRACE ((me, "write 0x%08lx command {%#x,%#x,%#x,%#x}; state %s -> %s",
	     (unsigned long) addr, ssource[0],
	     nr_bytes > 1 ? ssource[1] : 0,
	     nr_bytes > 2 ? ssource[2] : 0,
	     nr_bytes > 3 ? ssource[3] : 0,
	     state_names[old_state], state_names[cfi->state]));

  return nr_bytes;
}

/* All reads to the flash address space come here.  Using the state
   machine, we figure out what to return -- actual data stored in the
   flash, the CFI query structure, some status info, or something else ?
   Any requests that we can't handle are passed to the command set-
   specific read function.  */
static unsigned
cfi_io_read_buffer (struct hw *me, void *dest, int space,
		    address_word addr, unsigned nr_bytes)
{
  struct cfi *cfi = hw_data (me);
  unsigned char *sdest = dest;
  unsigned offset, shifted_offset;

  offset = addr & (cfi->dev_size - 1);
  shifted_offset = cfi_unshift_addr (cfi, offset);

  /* XXX: Is this OK to enforce ?  */
#if 0
  if (cfi->state != CFI_STATE_READ && cfi->width != nr_bytes)
    {
      HW_TRACE ((me, "read 0x%08lx length %u does not match flash width %u",
		 (unsigned long) addr, nr_bytes, cfi->width));
      return nr_bytes;
    }
#endif

  HW_TRACE ((me, "%s read 0x%08lx length %u",
	     state_names[cfi->state], (unsigned long) addr, nr_bytes));

  switch (cfi->state)
    {
    case CFI_STATE_READ:
      memcpy (dest, cfi->data + offset, nr_bytes);
      break;

    case CFI_STATE_CFI_QUERY:
      if (shifted_offset >= CFI_ADDR_CFI_QUERY_RESULT &&
	  shifted_offset < CFI_ADDR_CFI_QUERY_RESULT + sizeof (cfi->query) +
		     (cfi->query.num_erase_regions * 4))
	{
	  unsigned char *qry;

	  shifted_offset -= CFI_ADDR_CFI_QUERY_RESULT;
	  if (shifted_offset >= sizeof (cfi->query))
	    {
	      qry = cfi->erase_region_info;
	      shifted_offset -= sizeof (cfi->query);
	    }
	  else
	    qry = (void *) &cfi->query;

	  sdest[0] = qry[shifted_offset];
	  memset (sdest + 1, 0, nr_bytes - 1);

	  break;
	}

      ATTRIBUTE_FALLTHROUGH;

    default:
      if (!cfi->cmdset->read (me, cfi, dest, offset, shifted_offset, nr_bytes))
	HW_TRACE ((me, "unhandled state %s", state_names[cfi->state]));
      break;
    }

  return nr_bytes;
}

/* Clean up any state when this device is removed (e.g. when shutting
   down, or when reloading via gdb).  */
static void
cfi_delete_callback (struct hw *me)
{
#ifdef HAVE_MMAP
  struct cfi *cfi = hw_data (me);

  if (cfi->mmap)
    munmap (cfi->mmap, cfi->dev_size);
#endif
}

/* Helper function to easily add CFI erase regions to the existing set.  */
static void
cfi_add_erase_region (struct hw *me, struct cfi *cfi,
		      unsigned blocks, unsigned size)
{
  unsigned num_regions = cfi->query.num_erase_regions;
  struct cfi_erase_region *region;
  unsigned char *qry_region;

  /* Store for our own usage.  */
  region = &cfi->erase_regions[num_regions];
  region->blocks = blocks;
  region->size = size;
  if (num_regions == 0)
    region->start = 0;
  else
    region->start = region[-1].end;
  region->end = region->start + (blocks * size);

  /* Regions are 4 bytes long.  */
  qry_region = cfi->erase_region_info + 4 * num_regions;

  /* [0][1] = number erase blocks - 1 */
  if (blocks > 0xffff + 1)
    hw_abort (me, "erase blocks %u too big to fit into region info", blocks);
  cfi_encode_16bit (&qry_region[0], blocks - 1);

  /* [2][3] = block size / 256 bytes */
  if (size > 0xffff * 256)
    hw_abort (me, "erase size %u too big to fit into region info", size);
  cfi_encode_16bit (&qry_region[2], size / 256);

  /* Yet another region.  */
  cfi->query.num_erase_regions = num_regions + 1;
}

/* Device tree options:
     Required:
       .../reg <addr> <len>
       .../cmdset <primary; integer> [alt; integer]
     Optional:
       .../size <device size (must be pow of 2)>
       .../width <8|16|32>
       .../write_size <integer (must be pow of 2)>
       .../erase_regions <number blocks> <block size> \
                         [<number blocks> <block size> ...]
       .../voltage <vcc min> <vcc max> <vpp min> <vpp max>
       .../timeouts <typ unit write>  <typ buf write>  \
                    <typ block erase> <typ chip erase> \
                    <max unit write>  <max buf write>  \
                    <max block erase> <max chip erase>
       .../file <file> [ro|rw]
     Defaults:
       size: <len> from "reg"
       width: 8
       write_size: 0 (not supported)
       erase_region: 1 (can only erase whole chip)
       voltage: 0.0V (for all)
       timeouts: typ: 1µs, not supported, 1ms, not supported
                 max: 1µs, 1ms, 1ms, not supported

  TODO: Verify user args are valid (e.g. voltage is 8 bits).  */
static void
attach_cfi_regs (struct hw *me, struct cfi *cfi)
{
  address_word attach_address;
  int attach_space;
  unsigned attach_size;
  reg_property_spec reg;
  bool fd_writable;
  int i, ret, fd;
  signed_cell ival;

  if (hw_find_property (me, "reg") == NULL)
    hw_abort (me, "Missing \"reg\" property");
  if (hw_find_property (me, "cmdset") == NULL)
    hw_abort (me, "Missing \"cmdset\" property");

  if (!hw_find_reg_array_property (me, "reg", 0, &reg))
    hw_abort (me, "\"reg\" property must contain three addr/size entries");

  hw_unit_address_to_attach_address (hw_parent (me),
				     &reg.address,
				     &attach_space, &attach_address, me);
  hw_unit_size_to_attach_size (hw_parent (me), &reg.size, &attach_size, me);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  /* Extract the desired flash command set.  */
  ret = hw_find_integer_array_property (me, "cmdset", 0, &ival);
  if (ret != 1 && ret != 2)
    hw_abort (me, "\"cmdset\" property takes 1 or 2 entries");
  cfi_encode_16bit (cfi->query.p_id, ival);

  for (i = 0; i < ARRAY_SIZE (cfi_cmdsets); ++i)
    if (cfi_cmdsets[i]->id == ival)
      cfi->cmdset = cfi_cmdsets[i];
  if (cfi->cmdset == NULL)
    hw_abort (me, "cmdset %" PRIiTC " not supported", ival);

  if (ret == 2)
    {
      hw_find_integer_array_property (me, "cmdset", 1, &ival);
      cfi_encode_16bit (cfi->query.a_id, ival);
    }

  /* Extract the desired device size.  */
  if (hw_find_property (me, "size"))
    cfi->dev_size = hw_find_integer_property (me, "size");
  else
    cfi->dev_size = attach_size;
  cfi->query.dev_size = log2 (cfi->dev_size);

  /* Extract the desired flash width.  */
  if (hw_find_property (me, "width"))
    {
      cfi->width = hw_find_integer_property (me, "width");
      if (cfi->width != 8 && cfi->width != 16 && cfi->width != 32)
	hw_abort (me, "\"width\" must be 8 or 16 or 32, not %u", cfi->width);
    }
  else
    /* Default to 8 bit.  */
    cfi->width = 8;
  /* Turn 8/16/32 into 1/2/4.  */
  cfi->width /= 8;

  /* Extract optional write buffer size.  */
  if (hw_find_property (me, "write_size"))
    {
      ival = hw_find_integer_property (me, "write_size");
      cfi_encode_16bit (cfi->query.max_buf_write_len, log2 (ival));
    }

  /* Extract optional erase regions.  */
  if (hw_find_property (me, "erase_regions"))
    {
      ret = hw_find_integer_array_property (me, "erase_regions", 0, &ival);
      if (ret % 2)
	hw_abort (me, "\"erase_regions\" must be specified in sets of 2");

      cfi->erase_region_info = HW_NALLOC (me, unsigned char, ret / 2);
      cfi->erase_regions = HW_NALLOC (me, struct cfi_erase_region, ret / 2);

      for (i = 0; i < ret; i += 2)
	{
	  unsigned blocks, size;

	  hw_find_integer_array_property (me, "erase_regions", i, &ival);
	  blocks = ival;

	  hw_find_integer_array_property (me, "erase_regions", i + 1, &ival);
	  size = ival;

	  cfi_add_erase_region (me, cfi, blocks, size);
	}
    }

  /* Extract optional voltages.  */
  if (hw_find_property (me, "voltage"))
    {
      unsigned num = ARRAY_SIZE (cfi->query.voltages);

      ret = hw_find_integer_array_property (me, "voltage", 0, &ival);
      if (ret > num)
	hw_abort (me, "\"voltage\" may have only %u arguments", num);

      for (i = 0; i < ret; ++i)
	{
	  hw_find_integer_array_property (me, "voltage", i, &ival);
	  cfi->query.voltages[i] = ival;
	}
    }

  /* Extract optional timeouts.  */
  if (hw_find_property (me, "timeout"))
    {
      unsigned num = ARRAY_SIZE (cfi->query.timeouts);

      ret = hw_find_integer_array_property (me, "timeout", 0, &ival);
      if (ret > num)
	hw_abort (me, "\"timeout\" may have only %u arguments", num);

      for (i = 0; i < ret; ++i)
	{
	  hw_find_integer_array_property (me, "timeout", i, &ival);
	  cfi->query.timeouts[i] = ival;
	}
    }

  /* Extract optional file.  */
  fd = -1;
  fd_writable = false;
  if (hw_find_property (me, "file"))
    {
      const char *file;

      ret = hw_find_string_array_property (me, "file", 0, &file);
      if (ret > 2)
	hw_abort (me, "\"file\" may take only one argument");
      if (ret == 2)
	{
	  const char *writable;

	  hw_find_string_array_property (me, "file", 1, &writable);
	  fd_writable = !strcmp (writable, "rw");
	}

      fd = open (file, fd_writable ? O_RDWR : O_RDONLY);
      if (fd < 0)
	hw_abort (me, "unable to read file `%s': %s", file, strerror (errno));
    }

  /* Figure out where our initial flash data is coming from.  */
  if (fd != -1 && fd_writable)
    {
#if defined (HAVE_MMAP) && defined (HAVE_POSIX_FALLOCATE)
      posix_fallocate (fd, 0, cfi->dev_size);

      cfi->mmap = mmap (NULL, cfi->dev_size,
			PROT_READ | (fd_writable ? PROT_WRITE : 0),
			MAP_SHARED, fd, 0);

      if (cfi->mmap == MAP_FAILED)
	cfi->mmap = NULL;
      else
	cfi->data = cfi->mmap;
#else
      sim_io_eprintf (hw_system (me),
		      "cfi: sorry, file write support requires mmap()\n");
#endif
    }
  if (!cfi->data)
    {
      size_t read_len;

      cfi->data = HW_NALLOC (me, unsigned char, cfi->dev_size);

      if (fd != -1)
	{
	  /* Use stdio to avoid EINTR issues with read().  */
	  FILE *fp = fdopen (fd, "r");

	  if (fp)
	    read_len = fread (cfi->data, 1, cfi->dev_size, fp);
	  else
	    read_len = 0;

	  /* Don't need to fclose() with fdopen("r").  */
	}
      else
	read_len = 0;

      memset (cfi->data, 0xff, cfi->dev_size - read_len);
    }

  close (fd);
}

/* Once we've been declared in the device tree, this is the main
   entry point. So allocate state, attach memory addresses, and
   all that fun stuff.  */
static void
cfi_finish (struct hw *me)
{
  struct cfi *cfi;

  cfi = HW_ZALLOC (me, struct cfi);

  set_hw_data (me, cfi);
  set_hw_io_read_buffer (me, cfi_io_read_buffer);
  set_hw_io_write_buffer (me, cfi_io_write_buffer);
  set_hw_delete (me, cfi_delete_callback);

  attach_cfi_regs (me, cfi);

  /* Initialize the CFI.  */
  cfi->state = CFI_STATE_READ;
  memcpy (cfi->query.qry, "QRY", 3);
  cfi->cmdset->setup (me, cfi);
}

/* Every device is required to declare this.  */
const struct hw_descriptor dv_cfi_descriptor[] =
{
  {"cfi", cfi_finish,},
  {NULL, NULL},
};
