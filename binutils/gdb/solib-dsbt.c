/* Handle TIC6X (DSBT) shared libraries for GDB, the GNU Debugger.
   Copyright (C) 2010-2024 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "gdbcore.h"
#include "solib.h"
#include "solist.h"
#include "objfiles.h"
#include "symtab.h"
#include "command.h"
#include "gdb_bfd.h"
#include "solib-dsbt.h"
#include "elf/common.h"
#include "cli/cli-cmds.h"

#define GOT_MODULE_OFFSET 4

/* Flag which indicates whether internal debug messages should be printed.  */
static unsigned int solib_dsbt_debug = 0;

/* TIC6X pointers are four bytes wide.  */
enum { TIC6X_PTR_SIZE = 4 };

/* Representation of loadmap and related structs for the TIC6X DSBT.  */

/* External versions; the size and alignment of the fields should be
   the same as those on the target.  When loaded, the placement of
   the bits in each field will be the same as on the target.  */
typedef gdb_byte ext_Elf32_Half[2];
typedef gdb_byte ext_Elf32_Addr[4];
typedef gdb_byte ext_Elf32_Word[4];

struct ext_elf32_dsbt_loadseg
{
  /* Core address to which the segment is mapped.  */
  ext_Elf32_Addr addr;
  /* VMA recorded in the program header.  */
  ext_Elf32_Addr p_vaddr;
  /* Size of this segment in memory.  */
  ext_Elf32_Word p_memsz;
};

struct ext_elf32_dsbt_loadmap {
  /* Protocol version number, must be zero.  */
  ext_Elf32_Word version;
  /* A pointer to the DSBT table; the DSBT size and the index of this
     module.  */
  ext_Elf32_Word dsbt_table_ptr;
  ext_Elf32_Word dsbt_size;
  ext_Elf32_Word dsbt_index;
  /* Number of segments in this map.  */
  ext_Elf32_Word nsegs;
  /* The actual memory map.  */
  struct ext_elf32_dsbt_loadseg segs[1 /* nsegs, actually */];
};

/* Internal versions; the types are GDB types and the data in each
   of the fields is (or will be) decoded from the external struct
   for ease of consumption.  */
struct int_elf32_dsbt_loadseg
{
  /* Core address to which the segment is mapped.  */
  CORE_ADDR addr;
  /* VMA recorded in the program header.  */
  CORE_ADDR p_vaddr;
  /* Size of this segment in memory.  */
  long p_memsz;
};

struct int_elf32_dsbt_loadmap
{
  /* Protocol version number, must be zero.  */
  int version;
  CORE_ADDR dsbt_table_ptr;
  /* A pointer to the DSBT table; the DSBT size and the index of this
     module.  */
  int dsbt_size, dsbt_index;
  /* Number of segments in this map.  */
  int nsegs;
  /* The actual memory map.  */
  struct int_elf32_dsbt_loadseg segs[1 /* nsegs, actually */];
};

/* External link_map and elf32_dsbt_loadaddr struct definitions.  */

typedef gdb_byte ext_ptr[4];

struct ext_elf32_dsbt_loadaddr
{
  ext_ptr map;			/* struct elf32_dsbt_loadmap *map; */
};

struct dbst_ext_link_map
{
  struct ext_elf32_dsbt_loadaddr l_addr;

  /* Absolute file name object was found in.  */
  ext_ptr l_name;		/* char *l_name; */

  /* Dynamic section of the shared object.  */
  ext_ptr l_ld;			/* ElfW(Dyn) *l_ld; */

  /* Chain of loaded objects.  */
  ext_ptr l_next, l_prev;	/* struct link_map *l_next, *l_prev; */
};

/* Link map info to include in an allocated so_list entry */

struct lm_info_dsbt final : public lm_info
{
  ~lm_info_dsbt ()
  {
    xfree (this->map);
  }

  /* The loadmap, digested into an easier to use form.  */
  int_elf32_dsbt_loadmap *map = NULL;
};

/* Per pspace dsbt specific data.  */

struct dsbt_info
{
  /* The load map, got value, etc. are not available from the chain
     of loaded shared objects.  ``main_executable_lm_info'' provides
     a way to get at this information so that it doesn't need to be
     frequently recomputed.  Initialized by dsbt_relocate_main_executable.  */
  struct lm_info_dsbt *main_executable_lm_info = nullptr;

  /* Load maps for the main executable and the interpreter.  These are obtained
     from ptrace.  They are the starting point for getting into the program,
     and are required to find the solib list with the individual load maps for
     each module.  */
  struct int_elf32_dsbt_loadmap *exec_loadmap = nullptr;
  struct int_elf32_dsbt_loadmap *interp_loadmap = nullptr;

  /* Cached value for lm_base, below.  */
  CORE_ADDR lm_base_cache = 0;

  /* Link map address for main module.  */
  CORE_ADDR main_lm_addr = 0;

  CORE_ADDR interp_text_sect_low = 0;
  CORE_ADDR interp_text_sect_high = 0;
  CORE_ADDR interp_plt_sect_low = 0;
  CORE_ADDR interp_plt_sect_high = 0;
};

/* Per-program-space data key.  */
static const registry<program_space>::key<dsbt_info> solib_dsbt_pspace_data;

/* Get the dsbt solib data for PSPACE.  If none is found yet, add it now.  This
   function always returns a valid object.  */

static dsbt_info *
get_dsbt_info (program_space *pspace)
{
  dsbt_info *info = solib_dsbt_pspace_data.get (pspace);
  if (info != nullptr)
    return info;

  return solib_dsbt_pspace_data.emplace (pspace);
}


static void
dsbt_print_loadmap (struct int_elf32_dsbt_loadmap *map)
{
  int i;

  if (map == NULL)
    gdb_printf ("(null)\n");
  else if (map->version != 0)
    gdb_printf (_("Unsupported map version: %d\n"), map->version);
  else
    {
      gdb_printf ("version %d\n", map->version);

      for (i = 0; i < map->nsegs; i++)
	gdb_printf ("%s:%s -> %s:%s\n",
		    print_core_address (current_inferior ()->arch (),
					map->segs[i].p_vaddr),
		    print_core_address (current_inferior ()->arch (),
					(map->segs[i].p_vaddr
					 + map->segs[i].p_memsz)),
		    print_core_address (current_inferior ()->arch (),
					map->segs[i].addr),
		    print_core_address (current_inferior ()->arch (),
					(map->segs[i].addr
					 + map->segs[i].p_memsz)));
    }
}

/* Decode int_elf32_dsbt_loadmap from BUF.  */

static struct int_elf32_dsbt_loadmap *
decode_loadmap (const gdb_byte *buf)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  const struct ext_elf32_dsbt_loadmap *ext_ldmbuf;
  struct int_elf32_dsbt_loadmap *int_ldmbuf;

  int version, seg, nsegs;
  int int_ldmbuf_size;

  ext_ldmbuf = (struct ext_elf32_dsbt_loadmap *) buf;

  /* Extract the version.  */
  version = extract_unsigned_integer (ext_ldmbuf->version,
				      sizeof ext_ldmbuf->version,
				      byte_order);
  if (version != 0)
    {
      /* We only handle version 0.  */
      return NULL;
    }

  /* Extract the number of segments.  */
  nsegs = extract_unsigned_integer (ext_ldmbuf->nsegs,
				    sizeof ext_ldmbuf->nsegs,
				    byte_order);

  if (nsegs <= 0)
    return NULL;

  /* Allocate space into which to put information extract from the
     external loadsegs.  I.e, allocate the internal loadsegs.  */
  int_ldmbuf_size = (sizeof (struct int_elf32_dsbt_loadmap)
		     + (nsegs - 1) * sizeof (struct int_elf32_dsbt_loadseg));
  int_ldmbuf = (struct int_elf32_dsbt_loadmap *) xmalloc (int_ldmbuf_size);

  /* Place extracted information in internal structs.  */
  int_ldmbuf->version = version;
  int_ldmbuf->nsegs = nsegs;
  for (seg = 0; seg < nsegs; seg++)
    {
      int_ldmbuf->segs[seg].addr
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].addr,
				    sizeof (ext_ldmbuf->segs[seg].addr),
				    byte_order);
      int_ldmbuf->segs[seg].p_vaddr
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].p_vaddr,
				    sizeof (ext_ldmbuf->segs[seg].p_vaddr),
				    byte_order);
      int_ldmbuf->segs[seg].p_memsz
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].p_memsz,
				    sizeof (ext_ldmbuf->segs[seg].p_memsz),
				    byte_order);
    }

  return int_ldmbuf;
}

/* Interrogate the Linux kernel to find out where the program was loaded.
   There are two load maps; one for the executable and one for the
   interpreter (only in the case of a dynamically linked executable).  */

static void
dsbt_get_initial_loadmaps (void)
{
  dsbt_info *info = get_dsbt_info (current_program_space);
  std::optional<gdb::byte_vector> buf
    = target_read_alloc (current_inferior ()->top_target (),
			 TARGET_OBJECT_FDPIC, "exec");

  if (!buf || buf->empty ())
    {
      info->exec_loadmap = NULL;
      error (_("Error reading DSBT exec loadmap"));
    }
  info->exec_loadmap = decode_loadmap (buf->data ());
  if (solib_dsbt_debug)
    dsbt_print_loadmap (info->exec_loadmap);

  buf = target_read_alloc (current_inferior ()->top_target (),
			   TARGET_OBJECT_FDPIC, "exec");
  if (!buf || buf->empty ())
    {
      info->interp_loadmap = NULL;
      error (_("Error reading DSBT interp loadmap"));
    }
  info->interp_loadmap = decode_loadmap (buf->data ());
  if (solib_dsbt_debug)
    dsbt_print_loadmap (info->interp_loadmap);
}

/* Given address LDMADDR, fetch and decode the loadmap at that address.
   Return NULL if there is a problem reading the target memory or if
   there doesn't appear to be a loadmap at the given address.  The
   allocated space (representing the loadmap) returned by this
   function may be freed via a single call to xfree.  */

static struct int_elf32_dsbt_loadmap *
fetch_loadmap (CORE_ADDR ldmaddr)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  struct ext_elf32_dsbt_loadmap ext_ldmbuf_partial;
  struct ext_elf32_dsbt_loadmap *ext_ldmbuf;
  struct int_elf32_dsbt_loadmap *int_ldmbuf;
  int ext_ldmbuf_size, int_ldmbuf_size;
  int version, seg, nsegs;

  /* Fetch initial portion of the loadmap.  */
  if (target_read_memory (ldmaddr, (gdb_byte *) &ext_ldmbuf_partial,
			  sizeof ext_ldmbuf_partial))
    {
      /* Problem reading the target's memory.  */
      return NULL;
    }

  /* Extract the version.  */
  version = extract_unsigned_integer (ext_ldmbuf_partial.version,
				      sizeof ext_ldmbuf_partial.version,
				      byte_order);
  if (version != 0)
    {
      /* We only handle version 0.  */
      return NULL;
    }

  /* Extract the number of segments.  */
  nsegs = extract_unsigned_integer (ext_ldmbuf_partial.nsegs,
				    sizeof ext_ldmbuf_partial.nsegs,
				    byte_order);

  if (nsegs <= 0)
    return NULL;

  /* Allocate space for the complete (external) loadmap.  */
  ext_ldmbuf_size = sizeof (struct ext_elf32_dsbt_loadmap)
    + (nsegs - 1) * sizeof (struct ext_elf32_dsbt_loadseg);
  ext_ldmbuf = (struct ext_elf32_dsbt_loadmap *) xmalloc (ext_ldmbuf_size);

  /* Copy over the portion of the loadmap that's already been read.  */
  memcpy (ext_ldmbuf, &ext_ldmbuf_partial, sizeof ext_ldmbuf_partial);

  /* Read the rest of the loadmap from the target.  */
  if (target_read_memory (ldmaddr + sizeof ext_ldmbuf_partial,
			  (gdb_byte *) ext_ldmbuf + sizeof ext_ldmbuf_partial,
			  ext_ldmbuf_size - sizeof ext_ldmbuf_partial))
    {
      /* Couldn't read rest of the loadmap.  */
      xfree (ext_ldmbuf);
      return NULL;
    }

  /* Allocate space into which to put information extract from the
     external loadsegs.  I.e, allocate the internal loadsegs.  */
  int_ldmbuf_size = sizeof (struct int_elf32_dsbt_loadmap)
    + (nsegs - 1) * sizeof (struct int_elf32_dsbt_loadseg);
  int_ldmbuf = (struct int_elf32_dsbt_loadmap *) xmalloc (int_ldmbuf_size);

  /* Place extracted information in internal structs.  */
  int_ldmbuf->version = version;
  int_ldmbuf->nsegs = nsegs;
  for (seg = 0; seg < nsegs; seg++)
    {
      int_ldmbuf->segs[seg].addr
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].addr,
				    sizeof (ext_ldmbuf->segs[seg].addr),
				    byte_order);
      int_ldmbuf->segs[seg].p_vaddr
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].p_vaddr,
				    sizeof (ext_ldmbuf->segs[seg].p_vaddr),
				    byte_order);
      int_ldmbuf->segs[seg].p_memsz
	= extract_unsigned_integer (ext_ldmbuf->segs[seg].p_memsz,
				    sizeof (ext_ldmbuf->segs[seg].p_memsz),
				    byte_order);
    }

  xfree (ext_ldmbuf);
  return int_ldmbuf;
}

static void dsbt_relocate_main_executable (void);
static int enable_break (void);

/* See solist.h. */

static int
open_symbol_file_object (int from_tty)
{
  /* Unimplemented.  */
  return 0;
}

/* Given a loadmap and an address, return the displacement needed
   to relocate the address.  */

static CORE_ADDR
displacement_from_map (struct int_elf32_dsbt_loadmap *map,
		       CORE_ADDR addr)
{
  int seg;

  for (seg = 0; seg < map->nsegs; seg++)
    if (map->segs[seg].p_vaddr <= addr
	&& addr < map->segs[seg].p_vaddr + map->segs[seg].p_memsz)
      return map->segs[seg].addr - map->segs[seg].p_vaddr;

  return 0;
}

/* Return the address from which the link map chain may be found.  On
   DSBT, a pointer to the start of the link map will be located at the
   word found at base of GOT + GOT_MODULE_OFFSET.

   The base of GOT may be found in a number of ways.  Assuming that the
   main executable has already been relocated,
   1 The easiest way to find this value is to look up the address of
   _GLOBAL_OFFSET_TABLE_.
   2 The other way is to look for tag DT_PLTGOT, which contains the virtual
   address of Global Offset Table.  .*/

static CORE_ADDR
lm_base (void)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  struct bound_minimal_symbol got_sym;
  CORE_ADDR addr;
  gdb_byte buf[TIC6X_PTR_SIZE];
  dsbt_info *info = get_dsbt_info (current_program_space);

  /* One of our assumptions is that the main executable has been relocated.
     Bail out if this has not happened.  (Note that post_create_inferior
     in infcmd.c will call solib_add prior to solib_create_inferior_hook.
     If we allow this to happen, lm_base_cache will be initialized with
     a bogus value.  */
  if (info->main_executable_lm_info == 0)
    return 0;

  /* If we already have a cached value, return it.  */
  if (info->lm_base_cache)
    return info->lm_base_cache;

  got_sym = lookup_minimal_symbol ("_GLOBAL_OFFSET_TABLE_", NULL,
				   current_program_space->symfile_object_file);

  if (got_sym.minsym != 0)
    {
      addr = got_sym.value_address ();
      if (solib_dsbt_debug)
	gdb_printf (gdb_stdlog,
		    "lm_base: get addr %x by _GLOBAL_OFFSET_TABLE_.\n",
		    (unsigned int) addr);
    }
  else if (gdb_bfd_scan_elf_dyntag (DT_PLTGOT,
				    current_program_space->exec_bfd (),
				    &addr, NULL))
    {
      struct int_elf32_dsbt_loadmap *ldm;

      dsbt_get_initial_loadmaps ();
      ldm = info->exec_loadmap;
      addr += displacement_from_map (ldm, addr);
      if (solib_dsbt_debug)
	gdb_printf (gdb_stdlog,
		    "lm_base: get addr %x by DT_PLTGOT.\n",
		    (unsigned int) addr);
    }
  else
    {
      if (solib_dsbt_debug)
	gdb_printf (gdb_stdlog,
		    "lm_base: _GLOBAL_OFFSET_TABLE_ not found.\n");
      return 0;
    }
  addr += GOT_MODULE_OFFSET;

  if (solib_dsbt_debug)
    gdb_printf (gdb_stdlog,
		"lm_base: _GLOBAL_OFFSET_TABLE_ + %d = %s\n",
		GOT_MODULE_OFFSET, hex_string_custom (addr, 8));

  if (target_read_memory (addr, buf, sizeof buf) != 0)
    return 0;
  info->lm_base_cache = extract_unsigned_integer (buf, sizeof buf, byte_order);

  if (solib_dsbt_debug)
    gdb_printf (gdb_stdlog,
		"lm_base: lm_base_cache = %s\n",
		hex_string_custom (info->lm_base_cache, 8));

  return info->lm_base_cache;
}


/* Build a list of `struct shobj' objects describing the shared
   objects currently loaded in the inferior.  This list does not
   include an entry for the main executable file.

   Note that we only gather information directly available from the
   inferior --- we don't examine any of the shared library files
   themselves.  The declaration of `struct shobj' says which fields
   we provide values for.  */

static intrusive_list<shobj>
dsbt_current_sos (void)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  CORE_ADDR lm_addr;
  dsbt_info *info = get_dsbt_info (current_program_space);
  intrusive_list<shobj> sos;

  /* Make sure that the main executable has been relocated.  This is
     required in order to find the address of the global offset table,
     which in turn is used to find the link map info.  (See lm_base
     for details.)

     Note that the relocation of the main executable is also performed
     by solib_create_inferior_hook, however, in the case of core
     files, this hook is called too late in order to be of benefit to
     solib_add.  solib_add eventually calls this function,
     dsbt_current_sos, and also precedes the call to
     solib_create_inferior_hook.   (See post_create_inferior in
     infcmd.c.)  */
  if (info->main_executable_lm_info == 0 && core_bfd != NULL)
    dsbt_relocate_main_executable ();

  /* Locate the address of the first link map struct.  */
  lm_addr = lm_base ();

  /* We have at least one link map entry.  Fetch the lot of them,
     building the solist chain.  */
  while (lm_addr)
    {
      struct dbst_ext_link_map lm_buf;
      ext_Elf32_Word indexword;
      CORE_ADDR map_addr;
      int dsbt_index;
      int ret;

      if (solib_dsbt_debug)
	gdb_printf (gdb_stdlog,
		    "current_sos: reading link_map entry at %s\n",
		    hex_string_custom (lm_addr, 8));

      ret = target_read_memory (lm_addr, (gdb_byte *) &lm_buf, sizeof (lm_buf));
      if (ret)
	{
	  warning (_("dsbt_current_sos: Unable to read link map entry."
		     "  Shared object chain may be incomplete."));
	  break;
	}

      /* Fetch the load map address.  */
      map_addr = extract_unsigned_integer (lm_buf.l_addr.map,
					   sizeof lm_buf.l_addr.map,
					   byte_order);

      ret = target_read_memory (map_addr + 12, (gdb_byte *) &indexword,
				sizeof indexword);
      if (ret)
	{
	  warning (_("dsbt_current_sos: Unable to read dsbt index."
		     "  Shared object chain may be incomplete."));
	  break;
	}
      dsbt_index = extract_unsigned_integer (indexword, sizeof indexword,
					     byte_order);

      /* If the DSBT index is zero, then we're looking at the entry
	 for the main executable.  By convention, we don't include
	 this in the list of shared objects.  */
      if (dsbt_index != 0)
	{
	  struct int_elf32_dsbt_loadmap *loadmap;
	  CORE_ADDR addr;

	  loadmap = fetch_loadmap (map_addr);
	  if (loadmap == NULL)
	    {
	      warning (_("dsbt_current_sos: Unable to fetch load map."
			 "  Shared object chain may be incomplete."));
	      break;
	    }

	  shobj *sop = new shobj;
	  auto li = std::make_unique<lm_info_dsbt> ();
	  li->map = loadmap;
	  /* Fetch the name.  */
	  addr = extract_unsigned_integer (lm_buf.l_name,
					   sizeof (lm_buf.l_name),
					   byte_order);
	  gdb::unique_xmalloc_ptr<char> name_buf
	    = target_read_string (addr, SO_NAME_MAX_PATH_SIZE - 1);

	  if (name_buf == nullptr)
	    warning (_("Can't read pathname for link map entry."));
	  else
	    {
	      if (solib_dsbt_debug)
		gdb_printf (gdb_stdlog, "current_sos: name = %s\n",
			    name_buf.get ());

	      sop->so_name = name_buf.get ();
	      sop->so_original_name = sop->so_name;
	    }

	  sos.push_back (*sop);
	}
      else
	{
	  info->main_lm_addr = lm_addr;
	}

      lm_addr = extract_unsigned_integer (lm_buf.l_next,
					  sizeof (lm_buf.l_next), byte_order);
    }

  return sos;
}

/* Return 1 if PC lies in the dynamic symbol resolution code of the
   run time loader.  */

static int
dsbt_in_dynsym_resolve_code (CORE_ADDR pc)
{
  dsbt_info *info = get_dsbt_info (current_program_space);

  return ((pc >= info->interp_text_sect_low && pc < info->interp_text_sect_high)
	  || (pc >= info->interp_plt_sect_low && pc < info->interp_plt_sect_high)
	  || in_plt_section (pc));
}

/* Print a warning about being unable to set the dynamic linker
   breakpoint.  */

static void
enable_break_failure_warning (void)
{
  warning (_("Unable to find dynamic linker breakpoint function.\n"
	     "GDB will be unable to debug shared library initializers\n"
	     "and track explicitly loaded dynamic code."));
}

/* The dynamic linkers has, as part of its debugger interface, support
   for arranging for the inferior to hit a breakpoint after mapping in
   the shared libraries.  This function enables that breakpoint.

   On the TIC6X, using the shared library (DSBT), GDB can try to place
   a breakpoint on '_dl_debug_state' to monitor the shared library
   event.  */

static int
enable_break (void)
{
  asection *interp_sect;

  if (current_program_space->exec_bfd () == NULL)
    return 0;

  if (!target_has_execution ())
    return 0;

  dsbt_info *info = get_dsbt_info (current_program_space);

  info->interp_text_sect_low = 0;
  info->interp_text_sect_high = 0;
  info->interp_plt_sect_low = 0;
  info->interp_plt_sect_high = 0;

  /* Find the .interp section; if not found, warn the user and drop
     into the old breakpoint at symbol code.  */
  interp_sect = bfd_get_section_by_name (current_program_space->exec_bfd (),
					 ".interp");
  if (interp_sect)
    {
      unsigned int interp_sect_size;
      char *buf;
      CORE_ADDR addr;
      struct int_elf32_dsbt_loadmap *ldm;
      int ret;

      /* Read the contents of the .interp section into a local buffer;
	 the contents specify the dynamic linker this program uses.  */
      interp_sect_size = bfd_section_size (interp_sect);
      buf = (char *) alloca (interp_sect_size);
      bfd_get_section_contents (current_program_space->exec_bfd (),
				interp_sect, buf, 0, interp_sect_size);

      /* Now we need to figure out where the dynamic linker was
	 loaded so that we can load its symbols and place a breakpoint
	 in the dynamic linker itself.  */

      gdb_bfd_ref_ptr tmp_bfd;
      try
	{
	  tmp_bfd = solib_bfd_open (buf);
	}
      catch (const gdb_exception &ex)
	{
	}

      if (tmp_bfd == NULL)
	{
	  enable_break_failure_warning ();
	  return 0;
	}

      dsbt_get_initial_loadmaps ();
      ldm = info->interp_loadmap;

      /* Record the relocated start and end address of the dynamic linker
	 text and plt section for dsbt_in_dynsym_resolve_code.  */
      interp_sect = bfd_get_section_by_name (tmp_bfd.get (), ".text");
      if (interp_sect)
	{
	  info->interp_text_sect_low = bfd_section_vma (interp_sect);
	  info->interp_text_sect_low
	    += displacement_from_map (ldm, info->interp_text_sect_low);
	  info->interp_text_sect_high
	    = info->interp_text_sect_low + bfd_section_size (interp_sect);
	}
      interp_sect = bfd_get_section_by_name (tmp_bfd.get (), ".plt");
      if (interp_sect)
	{
	  info->interp_plt_sect_low = bfd_section_vma (interp_sect);
	  info->interp_plt_sect_low
	    += displacement_from_map (ldm, info->interp_plt_sect_low);
	  info->interp_plt_sect_high
	    = info->interp_plt_sect_low + bfd_section_size (interp_sect);
	}

      addr = (gdb_bfd_lookup_symbol
	      (tmp_bfd.get (),
	       [] (const asymbol *sym)
	       {
		 return strcmp (sym->name, "_dl_debug_state") == 0;
	       }));

      if (addr != 0)
	{
	  if (solib_dsbt_debug)
	    gdb_printf (gdb_stdlog,
			"enable_break: _dl_debug_state (prior to relocation) = %s\n",
			hex_string_custom (addr, 8));
	  addr += displacement_from_map (ldm, addr);

	  if (solib_dsbt_debug)
	    gdb_printf (gdb_stdlog,
			"enable_break: _dl_debug_state (after relocation) = %s\n",
			hex_string_custom (addr, 8));

	  /* Now (finally!) create the solib breakpoint.  */
	  create_solib_event_breakpoint (current_inferior ()->arch (), addr);

	  ret = 1;
	}
      else
	{
	  if (solib_dsbt_debug)
	    gdb_printf (gdb_stdlog,
			"enable_break: _dl_debug_state is not found\n");
	  ret = 0;
	}

      /* We're done with the loadmap.  */
      xfree (ldm);

      return ret;
    }

  /* Tell the user we couldn't set a dynamic linker breakpoint.  */
  enable_break_failure_warning ();

  /* Failure return.  */
  return 0;
}

static void
dsbt_relocate_main_executable (void)
{
  struct int_elf32_dsbt_loadmap *ldm;
  int changed;
  dsbt_info *info = get_dsbt_info (current_program_space);

  dsbt_get_initial_loadmaps ();
  ldm = info->exec_loadmap;

  delete info->main_executable_lm_info;
  info->main_executable_lm_info = new lm_info_dsbt;
  info->main_executable_lm_info->map = ldm;

  objfile *objf = current_program_space->symfile_object_file;
  section_offsets new_offsets (objf->section_offsets.size ());
  changed = 0;

  for (obj_section *osect : objf->sections ())
    {
      CORE_ADDR orig_addr, addr, offset;
      int osect_idx;
      int seg;

      osect_idx = osect - objf->sections_start;

      /* Current address of section.  */
      addr = osect->addr ();
      /* Offset from where this section started.  */
      offset = objf->section_offsets[osect_idx];
      /* Original address prior to any past relocations.  */
      orig_addr = addr - offset;

      for (seg = 0; seg < ldm->nsegs; seg++)
	{
	  if (ldm->segs[seg].p_vaddr <= orig_addr
	      && orig_addr < ldm->segs[seg].p_vaddr + ldm->segs[seg].p_memsz)
	    {
	      new_offsets[osect_idx]
		= ldm->segs[seg].addr - ldm->segs[seg].p_vaddr;

	      if (new_offsets[osect_idx] != offset)
		changed = 1;
	      break;
	    }
	}
    }

  if (changed)
    objfile_relocate (objf, new_offsets);

  /* Now that OBJF has been relocated, we can compute the GOT value
     and stash it away.  */
}

/* When gdb starts up the inferior, it nurses it along (through the
   shell) until it is ready to execute its first instruction.  At this
   point, this function gets called via solib_create_inferior_hook.

   For the DSBT shared library, the main executable needs to be relocated.
   The shared library breakpoints also need to be enabled.  */

static void
dsbt_solib_create_inferior_hook (int from_tty)
{
  /* Relocate main executable.  */
  dsbt_relocate_main_executable ();

  /* Enable shared library breakpoints.  */
  if (!enable_break ())
    {
      warning (_("shared library handler failed to enable breakpoint"));
      return;
    }
}

static void
dsbt_clear_solib (program_space *pspace)
{
  dsbt_info *info = get_dsbt_info (pspace);

  info->lm_base_cache = 0;
  info->main_lm_addr = 0;

  delete info->main_executable_lm_info;
  info->main_executable_lm_info = NULL;
}

static void
dsbt_relocate_section_addresses (shobj &so, target_section *sec)
{
  int seg;
  auto *li = gdb::checked_static_cast<lm_info_dsbt *> (so.lm_info.get ());
  int_elf32_dsbt_loadmap *map = li->map;

  for (seg = 0; seg < map->nsegs; seg++)
    {
      if (map->segs[seg].p_vaddr <= sec->addr
	  && sec->addr < map->segs[seg].p_vaddr + map->segs[seg].p_memsz)
	{
	  CORE_ADDR displ = map->segs[seg].addr - map->segs[seg].p_vaddr;

	  sec->addr += displ;
	  sec->endaddr += displ;
	  break;
	}
    }
}
static void
show_dsbt_debug (struct ui_file *file, int from_tty,
		 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("solib-dsbt debugging is %s.\n"), value);
}

const struct target_so_ops dsbt_so_ops =
{
  dsbt_relocate_section_addresses,
  nullptr,
  dsbt_clear_solib,
  dsbt_solib_create_inferior_hook,
  dsbt_current_sos,
  open_symbol_file_object,
  dsbt_in_dynsym_resolve_code,
  solib_bfd_open,
};

void _initialize_dsbt_solib ();
void
_initialize_dsbt_solib ()
{
  /* Debug this file's internals.  */
  add_setshow_zuinteger_cmd ("solib-dsbt", class_maintenance,
			     &solib_dsbt_debug, _("\
Set internal debugging of shared library code for DSBT ELF."), _("\
Show internal debugging of shared library code for DSBT ELF."), _("\
When non-zero, DSBT solib specific internal debugging is enabled."),
			     NULL,
			     show_dsbt_debug,
			     &setdebuglist, &showdebuglist);
}
