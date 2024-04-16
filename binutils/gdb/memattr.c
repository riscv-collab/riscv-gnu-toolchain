/* Memory attributes support, for GDB.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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

#include "defs.h"
#include "command.h"
#include "gdbcmd.h"
#include "memattr.h"
#include "target.h"
#include "target-dcache.h"
#include "value.h"
#include "language.h"
#include "breakpoint.h"
#include "cli/cli-utils.h"
#include <algorithm>
#include "gdbarch.h"
#include "inferior.h"
#include "progspace.h"

static std::vector<mem_region> user_mem_region_list, target_mem_region_list;
static std::vector<mem_region> *mem_region_list = &target_mem_region_list;
static int mem_number = 0;

/* If this flag is set, the memory region list should be automatically
   updated from the target.  If it is clear, the list is user-controlled
   and should be left alone.  */

static bool
mem_use_target ()
{
  return mem_region_list == &target_mem_region_list;
}

/* If this flag is set, we have tried to fetch the target memory regions
   since the last time it was invalidated.  If that list is still
   empty, then the target can't supply memory regions.  */
static bool target_mem_regions_valid;

/* If this flag is set, gdb will assume that memory ranges not
   specified by the memory map have type MEM_NONE, and will
   emit errors on all accesses to that memory.  */
static bool inaccessible_by_default = true;

static void
show_inaccessible_by_default (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c,
			      const char *value)
{
  if (inaccessible_by_default)
    gdb_printf (file, _("Unknown memory addresses will "
			"be treated as inaccessible.\n"));
  else
    gdb_printf (file, _("Unknown memory addresses "
			"will be treated as RAM.\n"));          
}

/* This function should be called before any command which would
   modify the memory region list.  It will handle switching from
   a target-provided list to a local list, if necessary.  */

static void
require_user_regions (int from_tty)
{
  /* If we're already using a user-provided list, nothing to do.  */
  if (!mem_use_target ())
    return;

  /* Switch to a user-provided list (possibly a copy of the current
     one).  */
  mem_region_list = &user_mem_region_list;

  /* If we don't have a target-provided region list yet, then
     no need to warn.  */
  if (target_mem_region_list.empty ())
    return;

  /* Otherwise, let the user know how to get back.  */
  if (from_tty)
    warning (_("Switching to manual control of memory regions; use "
	       "\"mem auto\" to fetch regions from the target again."));

  /* And create a new list (copy of the target-supplied regions) for the user
     to modify.  */
  user_mem_region_list = target_mem_region_list;
}

/* This function should be called before any command which would
   read the memory region list, other than those which call
   require_user_regions.  It will handle fetching the
   target-provided list, if necessary.  */

static void
require_target_regions (void)
{
  if (mem_use_target () && !target_mem_regions_valid)
    {
      target_mem_regions_valid = true;
      target_mem_region_list = target_memory_map ();
    }
}

/* Create a new user-defined memory region.  */

static void
create_user_mem_region (CORE_ADDR lo, CORE_ADDR hi,
			const mem_attrib &attrib)
{
  /* lo == hi is a useless empty region.  */
  if (lo >= hi && hi != 0)
    {
      gdb_printf (_("invalid memory region: low >= high\n"));
      return;
    }

  mem_region newobj (lo, hi, attrib);

  auto it = std::lower_bound (user_mem_region_list.begin (),
			      user_mem_region_list.end (),
			      newobj);
  int ix = std::distance (user_mem_region_list.begin (), it);

  /* Check for an overlapping memory region.  We only need to check
     in the vincinity - at most one before and one after the
     insertion point.  */
  for (int i = ix - 1; i < ix + 1; i++)
    {
      if (i < 0)
	continue;
      if (i >= user_mem_region_list.size ())
	continue;

      mem_region &n = user_mem_region_list[i];

      if ((lo >= n.lo && (lo < n.hi || n.hi == 0))
	  || (hi > n.lo && (hi <= n.hi || n.hi == 0))
	  || (lo <= n.lo && ((hi >= n.hi && n.hi != 0) || hi == 0)))
	{
	  gdb_printf (_("overlapping memory region\n"));
	  return;
	}
    }

  newobj.number = ++mem_number;
  user_mem_region_list.insert (it, newobj);
}

/* Look up the memory region corresponding to ADDR.  */

struct mem_region *
lookup_mem_region (CORE_ADDR addr)
{
  static struct mem_region region (0, 0);
  CORE_ADDR lo;
  CORE_ADDR hi;

  require_target_regions ();

  /* First we initialize LO and HI so that they describe the entire
     memory space.  As we process the memory region chain, they are
     redefined to describe the minimal region containing ADDR.  LO
     and HI are used in the case where no memory region is defined
     that contains ADDR.  If a memory region is disabled, it is
     treated as if it does not exist.  The initial values for LO
     and HI represent the bottom and top of memory.  */

  lo = 0;
  hi = 0;

  /* Either find memory range containing ADDR, or set LO and HI
     to the nearest boundaries of an existing memory range.
     
     If we ever want to support a huge list of memory regions, this
     check should be replaced with a binary search (probably using
     VEC_lower_bound).  */
  for (mem_region &m : *mem_region_list)
    {
      if (m.enabled_p == 1)
	{
	  /* If the address is in the memory region, return that
	     memory range.  */
	  if (addr >= m.lo && (addr < m.hi || m.hi == 0))
	    return &m;

	  /* This (correctly) won't match if m->hi == 0, representing
	     the top of the address space, because CORE_ADDR is unsigned;
	     no value of LO is less than zero.  */
	  if (addr >= m.hi && lo < m.hi)
	    lo = m.hi;

	  /* This will never set HI to zero; if we're here and ADDR
	     is at or below M, and the region starts at zero, then ADDR
	     would have been in the region.  */
	  if (addr <= m.lo && (hi == 0 || hi > m.lo))
	    hi = m.lo;
	}
    }

  /* Because no region was found, we must cons up one based on what
     was learned above.  */
  region.lo = lo;
  region.hi = hi;

  /* When no memory map is defined at all, we always return 
     'default_mem_attrib', so that we do not make all memory 
     inaccessible for targets that don't provide a memory map.  */
  if (inaccessible_by_default && !mem_region_list->empty ())
    region.attrib = mem_attrib::unknown ();
  else
    region.attrib = mem_attrib ();

  return &region;
}

/* Invalidate any memory regions fetched from the target.  */

void
invalidate_target_mem_regions (void)
{
  if (!target_mem_regions_valid)
    return;

  target_mem_regions_valid = false;
  target_mem_region_list.clear ();
}

/* Clear user-defined memory region list.  */

static void
user_mem_clear (void)
{
  user_mem_region_list.clear ();
}


static void
mem_command (const char *args, int from_tty)
{
  CORE_ADDR lo, hi;

  if (!args)
    error_no_arg (_("No mem"));

  /* For "mem auto", switch back to using a target provided list.  */
  if (strcmp (args, "auto") == 0)
    {
      if (mem_use_target ())
	return;

      user_mem_clear ();
      mem_region_list = &target_mem_region_list;

      return;
    }

  require_user_regions (from_tty);

  std::string tok = extract_arg (&args);
  if (tok == "")
    error (_("no lo address"));
  lo = parse_and_eval_address (tok.c_str ());

  tok = extract_arg (&args);
  if (tok == "")
    error (_("no hi address"));
  hi = parse_and_eval_address (tok.c_str ());

  mem_attrib attrib;
  while ((tok = extract_arg (&args)) != "")
    {
      if (tok == "rw")
	attrib.mode = MEM_RW;
      else if (tok == "ro")
	attrib.mode = MEM_RO;
      else if (tok == "wo")
	attrib.mode = MEM_WO;

      else if (tok == "8")
	attrib.width = MEM_WIDTH_8;
      else if (tok == "16")
	{
	  if ((lo % 2 != 0) || (hi % 2 != 0))
	    error (_("region bounds not 16 bit aligned"));
	  attrib.width = MEM_WIDTH_16;
	}
      else if (tok == "32")
	{
	  if ((lo % 4 != 0) || (hi % 4 != 0))
	    error (_("region bounds not 32 bit aligned"));
	  attrib.width = MEM_WIDTH_32;
	}
      else if (tok == "64")
	{
	  if ((lo % 8 != 0) || (hi % 8 != 0))
	    error (_("region bounds not 64 bit aligned"));
	  attrib.width = MEM_WIDTH_64;
	}

#if 0
      else if (tok == "hwbreak")
	attrib.hwbreak = 1;
      else if (tok == "swbreak")
	attrib.hwbreak = 0;
#endif

      else if (tok == "cache")
	attrib.cache = 1;
      else if (tok == "nocache")
	attrib.cache = 0;

#if 0
      else if (tok == "verify")
	attrib.verify = 1;
      else if (tok == "noverify")
	attrib.verify = 0;
#endif

      else
	error (_("unknown attribute: %s"), tok.c_str ());
    }

  create_user_mem_region (lo, hi, attrib);
}


static void
info_mem_command (const char *args, int from_tty)
{
  if (mem_use_target ())
    gdb_printf (_("Using memory regions provided by the target.\n"));
  else
    gdb_printf (_("Using user-defined memory regions.\n"));

  require_target_regions ();

  if (mem_region_list->empty ())
    {
      gdb_printf (_("There are no memory regions defined.\n"));
      return;
    }

  gdb_printf ("Num ");
  gdb_printf ("Enb ");
  gdb_printf ("Low Addr   ");
  if (gdbarch_addr_bit (current_inferior ()->arch ()) > 32)
    gdb_printf ("        ");
  gdb_printf ("High Addr  ");
  if (gdbarch_addr_bit (current_inferior ()->arch ()) > 32)
    gdb_printf ("        ");
  gdb_printf ("Attrs ");
  gdb_printf ("\n");

  for (const mem_region &m : *mem_region_list)
    {
      const char *tmp;

      gdb_printf ("%-3d %-3c\t",
		  m.number,
		  m.enabled_p ? 'y' : 'n');
      if (gdbarch_addr_bit (current_inferior ()->arch ()) <= 32)
	tmp = hex_string_custom (m.lo, 8);
      else
	tmp = hex_string_custom (m.lo, 16);
      
      gdb_printf ("%s ", tmp);

      if (gdbarch_addr_bit (current_inferior ()->arch ()) <= 32)
	{
	  if (m.hi == 0)
	    tmp = "0x100000000";
	  else
	    tmp = hex_string_custom (m.hi, 8);
	}
      else
	{
	  if (m.hi == 0)
	    tmp = "0x10000000000000000";
	  else
	    tmp = hex_string_custom (m.hi, 16);
	}

      gdb_printf ("%s ", tmp);

      /* Print a token for each attribute.

       * FIXME: Should we output a comma after each token?  It may
       * make it easier for users to read, but we'd lose the ability
       * to cut-and-paste the list of attributes when defining a new
       * region.  Perhaps that is not important.
       *
       * FIXME: If more attributes are added to GDB, the output may
       * become cluttered and difficult for users to read.  At that
       * time, we may want to consider printing tokens only if they
       * are different from the default attribute.  */

      switch (m.attrib.mode)
	{
	case MEM_RW:
	  gdb_printf ("rw ");
	  break;
	case MEM_RO:
	  gdb_printf ("ro ");
	  break;
	case MEM_WO:
	  gdb_printf ("wo ");
	  break;
	case MEM_FLASH:
	  gdb_printf ("flash blocksize 0x%x ", m.attrib.blocksize);
	  break;
	}

      switch (m.attrib.width)
	{
	case MEM_WIDTH_8:
	  gdb_printf ("8 ");
	  break;
	case MEM_WIDTH_16:
	  gdb_printf ("16 ");
	  break;
	case MEM_WIDTH_32:
	  gdb_printf ("32 ");
	  break;
	case MEM_WIDTH_64:
	  gdb_printf ("64 ");
	  break;
	case MEM_WIDTH_UNSPECIFIED:
	  break;
	}

#if 0
      if (attrib->hwbreak)
	gdb_printf ("hwbreak");
      else
	gdb_printf ("swbreak");
#endif

      if (m.attrib.cache)
	gdb_printf ("cache ");
      else
	gdb_printf ("nocache ");

#if 0
      if (attrib->verify)
	gdb_printf ("verify ");
      else
	gdb_printf ("noverify ");
#endif

      gdb_printf ("\n");
    }
}


/* Enable the memory region number NUM.  */

static void
mem_enable (int num)
{
  for (mem_region &m : *mem_region_list)
    if (m.number == num)
      {
	m.enabled_p = 1;
	return;
      }
  gdb_printf (_("No memory region number %d.\n"), num);
}

static void
enable_mem_command (const char *args, int from_tty)
{
  require_user_regions (from_tty);

  target_dcache_invalidate (current_program_space->aspace);

  if (args == NULL || *args == '\0')
    { /* Enable all mem regions.  */
      for (mem_region &m : *mem_region_list)
	m.enabled_p = 1;
    }
  else
    {
      number_or_range_parser parser (args);
      while (!parser.finished ())
	{
	  int num = parser.get_number ();
	  mem_enable (num);
	}
    }
}


/* Disable the memory region number NUM.  */

static void
mem_disable (int num)
{
  for (mem_region &m : *mem_region_list)
    if (m.number == num)
      {
	m.enabled_p = 0;
	return;
      }
  gdb_printf (_("No memory region number %d.\n"), num);
}

static void
disable_mem_command (const char *args, int from_tty)
{
  require_user_regions (from_tty);

  target_dcache_invalidate (current_program_space->aspace);

  if (args == NULL || *args == '\0')
    {
      for (mem_region &m : *mem_region_list)
	m.enabled_p = false;
    }
  else
    {
      number_or_range_parser parser (args);
      while (!parser.finished ())
	{
	  int num = parser.get_number ();
	  mem_disable (num);
	}
    }
}

/* Delete the memory region number NUM.  */

static void
mem_delete (int num)
{
  if (!mem_region_list)
    {
      gdb_printf (_("No memory region number %d.\n"), num);
      return;
    }

  auto it = std::remove_if (mem_region_list->begin (), mem_region_list->end (),
			    [num] (const mem_region &m)
    {
      return m.number == num;
    });

  if (it != mem_region_list->end ())
    mem_region_list->erase (it);
  else
    gdb_printf (_("No memory region number %d.\n"), num);
}

static void
delete_mem_command (const char *args, int from_tty)
{
  require_user_regions (from_tty);

  target_dcache_invalidate (current_program_space->aspace);

  if (args == NULL || *args == '\0')
    {
      if (query (_("Delete all memory regions? ")))
	user_mem_clear ();
      dont_repeat ();
      return;
    }

  number_or_range_parser parser (args);
  while (!parser.finished ())
    {
      int num = parser.get_number ();
      mem_delete (num);
    }

  dont_repeat ();
}

static struct cmd_list_element *mem_set_cmdlist;
static struct cmd_list_element *mem_show_cmdlist;

void _initialize_mem ();
void
_initialize_mem ()
{
  add_com ("mem", class_vars, mem_command, _("\
Define attributes for memory region or reset memory region handling to "
"target-based.\n\
Usage: mem auto\n\
       mem LOW HIGH [MODE WIDTH CACHE],\n\
where MODE  may be rw (read/write), ro (read-only) or wo (write-only),\n\
      WIDTH may be 8, 16, 32, or 64, and\n\
      CACHE may be cache or nocache"));

  add_cmd ("mem", class_vars, enable_mem_command, _("\
Enable memory region.\n\
Arguments are the IDs of the memory regions to enable.\n\
Usage: enable mem [ID]...\n\
Do \"info mem\" to see current list of IDs."), &enablelist);

  add_cmd ("mem", class_vars, disable_mem_command, _("\
Disable memory region.\n\
Arguments are the IDs of the memory regions to disable.\n\
Usage: disable mem [ID]...\n\
Do \"info mem\" to see current list of IDs."), &disablelist);

  add_cmd ("mem", class_vars, delete_mem_command, _("\
Delete memory region.\n\
Arguments are the IDs of the memory regions to delete.\n\
Usage: delete mem [ID]...\n\
Do \"info mem\" to see current list of IDs."), &deletelist);

  add_info ("mem", info_mem_command,
	    _("Memory region attributes."));

  add_setshow_prefix_cmd ("mem", class_vars,
			  _("Memory regions settings."),
			  _("Memory regions settings."),
			  &mem_set_cmdlist, &mem_show_cmdlist,
			  &setlist, &showlist);

  add_setshow_boolean_cmd ("inaccessible-by-default", no_class,
				  &inaccessible_by_default, _("\
Set handling of unknown memory regions."), _("\
Show handling of unknown memory regions."), _("\
If on, and some memory map is defined, debugger will emit errors on\n\
accesses to memory not defined in the memory map. If off, accesses to all\n\
memory addresses will be allowed."),
				NULL,
				show_inaccessible_by_default,
				&mem_set_cmdlist,
				&mem_show_cmdlist);
}
