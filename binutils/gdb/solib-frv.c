/* Handle FR-V (FDPIC) shared libraries for GDB, the GNU Debugger.
   Copyright (C) 2004-2024 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "solib.h"
#include "solist.h"
#include "frv-tdep.h"
#include "objfiles.h"
#include "symtab.h"
#include "elf/frv.h"
#include "gdb_bfd.h"
#include "inferior.h"

/* FR-V pointers are four bytes wide.  */
enum { FRV_PTR_SIZE = 4 };

/* Representation of loadmap and related structs for the FR-V FDPIC ABI.  */

/* External versions; the size and alignment of the fields should be
   the same as those on the target.  When loaded, the placement of
   the bits in each field will be the same as on the target.  */
typedef gdb_byte ext_Elf32_Half[2];
typedef gdb_byte ext_Elf32_Addr[4];
typedef gdb_byte ext_Elf32_Word[4];

struct ext_elf32_fdpic_loadseg
{
  /* Core address to which the segment is mapped.  */
  ext_Elf32_Addr addr;
  /* VMA recorded in the program header.  */
  ext_Elf32_Addr p_vaddr;
  /* Size of this segment in memory.  */
  ext_Elf32_Word p_memsz;
};

struct ext_elf32_fdpic_loadmap {
  /* Protocol version number, must be zero.  */
  ext_Elf32_Half version;
  /* Number of segments in this map.  */
  ext_Elf32_Half nsegs;
  /* The actual memory map.  */
  struct ext_elf32_fdpic_loadseg segs[1 /* nsegs, actually */];
};

/* Internal versions; the types are GDB types and the data in each
   of the fields is (or will be) decoded from the external struct
   for ease of consumption.  */
struct int_elf32_fdpic_loadseg
{
  /* Core address to which the segment is mapped.  */
  CORE_ADDR addr;
  /* VMA recorded in the program header.  */
  CORE_ADDR p_vaddr;
  /* Size of this segment in memory.  */
  long p_memsz;
};

struct int_elf32_fdpic_loadmap {
  /* Protocol version number, must be zero.  */
  int version;
  /* Number of segments in this map.  */
  int nsegs;
  /* The actual memory map.  */
  struct int_elf32_fdpic_loadseg segs[1 /* nsegs, actually */];
};

/* Given address LDMADDR, fetch and decode the loadmap at that address.
   Return NULL if there is a problem reading the target memory or if
   there doesn't appear to be a loadmap at the given address.  The
   allocated space (representing the loadmap) returned by this
   function may be freed via a single call to xfree().  */

static struct int_elf32_fdpic_loadmap *
fetch_loadmap (CORE_ADDR ldmaddr)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  struct ext_elf32_fdpic_loadmap ext_ldmbuf_partial;
  struct ext_elf32_fdpic_loadmap *ext_ldmbuf;
  struct int_elf32_fdpic_loadmap *int_ldmbuf;
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
  ext_ldmbuf_size = sizeof (struct ext_elf32_fdpic_loadmap)
	       + (nsegs - 1) * sizeof (struct ext_elf32_fdpic_loadseg);
  ext_ldmbuf = (struct ext_elf32_fdpic_loadmap *) xmalloc (ext_ldmbuf_size);

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
  int_ldmbuf_size = sizeof (struct int_elf32_fdpic_loadmap)
	       + (nsegs - 1) * sizeof (struct int_elf32_fdpic_loadseg);
  int_ldmbuf = (struct int_elf32_fdpic_loadmap *) xmalloc (int_ldmbuf_size);

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

/* External link_map and elf32_fdpic_loadaddr struct definitions.  */

typedef gdb_byte ext_ptr[4];

struct ext_elf32_fdpic_loadaddr
{
  ext_ptr map;			/* struct elf32_fdpic_loadmap *map; */
  ext_ptr got_value;		/* void *got_value; */
};

struct ext_link_map
{
  struct ext_elf32_fdpic_loadaddr l_addr;

  /* Absolute file name object was found in.  */
  ext_ptr l_name;		/* char *l_name; */

  /* Dynamic section of the shared object.  */
  ext_ptr l_ld;			/* ElfW(Dyn) *l_ld; */

  /* Chain of loaded objects.  */
  ext_ptr l_next, l_prev;	/* struct link_map *l_next, *l_prev; */
};

/* Link map info to include in an allocated so_list entry.  */

struct lm_info_frv final : public lm_info
{
  ~lm_info_frv ()
  {
    xfree (this->map);
    xfree (this->dyn_syms);
    xfree (this->dyn_relocs);
  }

  /* The loadmap, digested into an easier to use form.  */
  int_elf32_fdpic_loadmap *map = NULL;
  /* The GOT address for this link map entry.  */
  CORE_ADDR got_value = 0;
  /* The link map address, needed for frv_fetch_objfile_link_map().  */
  CORE_ADDR lm_addr = 0;

  /* Cached dynamic symbol table and dynamic relocs initialized and
     used only by find_canonical_descriptor_in_load_object().

     Note: kevinb/2004-02-26: It appears that calls to
     bfd_canonicalize_dynamic_reloc() will use the same symbols as
     those supplied to the first call to this function.  Therefore,
     it's important to NOT free the asymbol ** data structure
     supplied to the first call.  Thus the caching of the dynamic
     symbols (dyn_syms) is critical for correct operation.  The
     caching of the dynamic relocations could be dispensed with.  */
  asymbol **dyn_syms = NULL;
  arelent **dyn_relocs = NULL;
  int dyn_reloc_count = 0;	/* Number of dynamic relocs.  */
};

/* The load map, got value, etc. are not available from the chain
   of loaded shared objects.  ``main_executable_lm_info'' provides
   a way to get at this information so that it doesn't need to be
   frequently recomputed.  Initialized by frv_relocate_main_executable().  */
static lm_info_frv *main_executable_lm_info;

static void frv_relocate_main_executable (void);
static CORE_ADDR main_got (void);
static int enable_break2 (void);

/* Implement the "open_symbol_file_object" target_so_ops method.  */

static int
open_symbol_file_object (int from_tty)
{
  /* Unimplemented.  */
  return 0;
}

/* Cached value for lm_base(), below.  */
static CORE_ADDR lm_base_cache = 0;

/* Link map address for main module.  */
static CORE_ADDR main_lm_addr = 0;

/* Return the address from which the link map chain may be found.  On
   the FR-V, this may be found in a number of ways.  Assuming that the
   main executable has already been relocated, the easiest way to find
   this value is to look up the address of _GLOBAL_OFFSET_TABLE_.  A
   pointer to the start of the link map will be located at the word found
   at _GLOBAL_OFFSET_TABLE_ + 8.  (This is part of the dynamic linker
   reserve area mandated by the ABI.)  */

static CORE_ADDR
lm_base (void)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  struct bound_minimal_symbol got_sym;
  CORE_ADDR addr;
  gdb_byte buf[FRV_PTR_SIZE];

  /* One of our assumptions is that the main executable has been relocated.
     Bail out if this has not happened.  (Note that post_create_inferior()
     in infcmd.c will call solib_add prior to solib_create_inferior_hook().
     If we allow this to happen, lm_base_cache will be initialized with
     a bogus value.  */
  if (main_executable_lm_info == 0)
    return 0;

  /* If we already have a cached value, return it.  */
  if (lm_base_cache)
    return lm_base_cache;

  got_sym = lookup_minimal_symbol ("_GLOBAL_OFFSET_TABLE_", NULL,
				   current_program_space->symfile_object_file);
  if (got_sym.minsym == 0)
    {
      solib_debug_printf ("_GLOBAL_OFFSET_TABLE_ not found.");
      return 0;
    }

  addr = got_sym.value_address () + 8;

  solib_debug_printf ("_GLOBAL_OFFSET_TABLE_ + 8 = %s",
		      hex_string_custom (addr, 8));

  if (target_read_memory (addr, buf, sizeof buf) != 0)
    return 0;
  lm_base_cache = extract_unsigned_integer (buf, sizeof buf, byte_order);

  solib_debug_printf ("lm_base_cache = %s",
		      hex_string_custom (lm_base_cache, 8));

  return lm_base_cache;
}


/* Implement the "current_sos" target_so_ops method.  */

static intrusive_list<shobj>
frv_current_sos ()
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  CORE_ADDR lm_addr, mgot;
  intrusive_list<shobj> sos;

  /* Make sure that the main executable has been relocated.  This is
     required in order to find the address of the global offset table,
     which in turn is used to find the link map info.  (See lm_base()
     for details.)

     Note that the relocation of the main executable is also performed
     by solib_create_inferior_hook(), however, in the case of core
     files, this hook is called too late in order to be of benefit to
     solib_add.  solib_add eventually calls this this function,
     frv_current_sos, and also precedes the call to
     solib_create_inferior_hook().   (See post_create_inferior() in
     infcmd.c.)  */
  if (main_executable_lm_info == 0 && core_bfd != NULL)
    frv_relocate_main_executable ();

  /* Fetch the GOT corresponding to the main executable.  */
  mgot = main_got ();

  /* Locate the address of the first link map struct.  */
  lm_addr = lm_base ();

  /* We have at least one link map entry.  Fetch the lot of them,
     building the solist chain.  */
  while (lm_addr)
    {
      struct ext_link_map lm_buf;
      CORE_ADDR got_addr;

      solib_debug_printf ("reading link_map entry at %s",
			  hex_string_custom (lm_addr, 8));

      if (target_read_memory (lm_addr, (gdb_byte *) &lm_buf,
			      sizeof (lm_buf)) != 0)
	{
	  warning (_("frv_current_sos: Unable to read link map entry.  "
		     "Shared object chain may be incomplete."));
	  break;
	}

      got_addr
	= extract_unsigned_integer (lm_buf.l_addr.got_value,
				    sizeof (lm_buf.l_addr.got_value),
				    byte_order);
      /* If the got_addr is the same as mgotr, then we're looking at the
	 entry for the main executable.  By convention, we don't include
	 this in the list of shared objects.  */
      if (got_addr != mgot)
	{
	  struct int_elf32_fdpic_loadmap *loadmap;
	  CORE_ADDR addr;

	  /* Fetch the load map address.  */
	  addr = extract_unsigned_integer (lm_buf.l_addr.map,
					   sizeof lm_buf.l_addr.map,
					   byte_order);
	  loadmap = fetch_loadmap (addr);
	  if (loadmap == NULL)
	    {
	      warning (_("frv_current_sos: Unable to fetch load map.  "
			 "Shared object chain may be incomplete."));
	      break;
	    }

	  shobj *sop = new shobj;
	  auto li = std::make_unique<lm_info_frv> ();
	  li->map = loadmap;
	  li->got_value = got_addr;
	  li->lm_addr = lm_addr;
	  /* Fetch the name.  */
	  addr = extract_unsigned_integer (lm_buf.l_name,
					   sizeof (lm_buf.l_name),
					   byte_order);
	  gdb::unique_xmalloc_ptr<char> name_buf
	    = target_read_string (addr, SO_NAME_MAX_PATH_SIZE - 1);

	  solib_debug_printf ("name = %s", name_buf.get ());

	  if (name_buf == nullptr)
	    warning (_("Can't read pathname for link map entry."));
	  else
	    {
	      sop->so_name = name_buf.get ();
	      sop->so_original_name = sop->so_name;
	    }

	  sos.push_back (*sop);
	}
      else
	{
	  main_lm_addr = lm_addr;
	}

      lm_addr = extract_unsigned_integer (lm_buf.l_next,
					  sizeof (lm_buf.l_next), byte_order);
    }

  enable_break2 ();

  return sos;
}


/* Return 1 if PC lies in the dynamic symbol resolution code of the
   run time loader.  */

static CORE_ADDR interp_text_sect_low;
static CORE_ADDR interp_text_sect_high;
static CORE_ADDR interp_plt_sect_low;
static CORE_ADDR interp_plt_sect_high;

static int
frv_in_dynsym_resolve_code (CORE_ADDR pc)
{
  return ((pc >= interp_text_sect_low && pc < interp_text_sect_high)
	  || (pc >= interp_plt_sect_low && pc < interp_plt_sect_high)
	  || in_plt_section (pc));
}

/* Given a loadmap and an address, return the displacement needed
   to relocate the address.  */

static CORE_ADDR
displacement_from_map (struct int_elf32_fdpic_loadmap *map,
		       CORE_ADDR addr)
{
  int seg;

  for (seg = 0; seg < map->nsegs; seg++)
    {
      if (map->segs[seg].p_vaddr <= addr
	  && addr < map->segs[seg].p_vaddr + map->segs[seg].p_memsz)
	{
	  return map->segs[seg].addr - map->segs[seg].p_vaddr;
	}
    }

  return 0;
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

/* Arrange for dynamic linker to hit breakpoint.

   The dynamic linkers has, as part of its debugger interface, support
   for arranging for the inferior to hit a breakpoint after mapping in
   the shared libraries.  This function enables that breakpoint.

   On the FR-V, using the shared library (FDPIC) ABI, the symbol
   _dl_debug_addr points to the r_debug struct which contains
   a field called r_brk.  r_brk is the address of the function
   descriptor upon which a breakpoint must be placed.  Being a
   function descriptor, we must extract the entry point in order
   to set the breakpoint.

   Our strategy will be to get the .interp section from the
   executable.  This section will provide us with the name of the
   interpreter.  We'll open the interpreter and then look up
   the address of _dl_debug_addr.  We then relocate this address
   using the interpreter's loadmap.  Once the relocated address
   is known, we fetch the value (address) corresponding to r_brk
   and then use that value to fetch the entry point of the function
   we're interested in.  */

static int enable_break2_done = 0;

static int
enable_break2 (void)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  asection *interp_sect;

  if (enable_break2_done)
    return 1;

  interp_text_sect_low = interp_text_sect_high = 0;
  interp_plt_sect_low = interp_plt_sect_high = 0;

  /* Find the .interp section; if not found, warn the user and drop
     into the old breakpoint at symbol code.  */
  interp_sect = bfd_get_section_by_name (current_program_space->exec_bfd (),
					 ".interp");
  if (interp_sect)
    {
      unsigned int interp_sect_size;
      char *buf;
      int status;
      CORE_ADDR addr, interp_loadmap_addr;
      gdb_byte addr_buf[FRV_PTR_SIZE];
      struct int_elf32_fdpic_loadmap *ldm;

      /* Read the contents of the .interp section into a local buffer;
	 the contents specify the dynamic linker this program uses.  */
      interp_sect_size = bfd_section_size (interp_sect);
      buf = (char *) alloca (interp_sect_size);
      bfd_get_section_contents (current_program_space->exec_bfd (),
				interp_sect, buf, 0, interp_sect_size);

      /* Now we need to figure out where the dynamic linker was
	 loaded so that we can load its symbols and place a breakpoint
	 in the dynamic linker itself.

	 This address is stored on the stack.  However, I've been unable
	 to find any magic formula to find it for Solaris (appears to
	 be trivial on GNU/Linux).  Therefore, we have to try an alternate
	 mechanism to find the dynamic linker's base address.  */

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

      status = frv_fdpic_loadmap_addresses (current_inferior ()->arch (),
					    &interp_loadmap_addr, 0);
      if (status < 0)
	{
	  warning (_("Unable to determine dynamic linker loadmap address."));
	  enable_break_failure_warning ();
	  return 0;
	}

      solib_debug_printf ("interp_loadmap_addr = %s",
			  hex_string_custom (interp_loadmap_addr, 8));

      ldm = fetch_loadmap (interp_loadmap_addr);
      if (ldm == NULL)
	{
	  warning (_("Unable to load dynamic linker loadmap at address %s."),
		   hex_string_custom (interp_loadmap_addr, 8));
	  enable_break_failure_warning ();
	  return 0;
	}

      /* Record the relocated start and end address of the dynamic linker
	 text and plt section for svr4_in_dynsym_resolve_code.  */
      interp_sect = bfd_get_section_by_name (tmp_bfd.get (), ".text");
      if (interp_sect)
	{
	  interp_text_sect_low = bfd_section_vma (interp_sect);
	  interp_text_sect_low
	    += displacement_from_map (ldm, interp_text_sect_low);
	  interp_text_sect_high
	    = interp_text_sect_low + bfd_section_size (interp_sect);
	}
      interp_sect = bfd_get_section_by_name (tmp_bfd.get (), ".plt");
      if (interp_sect)
	{
	  interp_plt_sect_low = bfd_section_vma (interp_sect);
	  interp_plt_sect_low
	    += displacement_from_map (ldm, interp_plt_sect_low);
	  interp_plt_sect_high =
	    interp_plt_sect_low + bfd_section_size (interp_sect);
	}

      addr = (gdb_bfd_lookup_symbol
	      (tmp_bfd.get (),
	       [] (const asymbol *sym)
	       {
		 return strcmp (sym->name, "_dl_debug_addr") == 0;
	       }));

      if (addr == 0)
	{
	  warning (_("Could not find symbol _dl_debug_addr "
		     "in dynamic linker"));
	  enable_break_failure_warning ();
	  return 0;
	}

      solib_debug_printf ("_dl_debug_addr (prior to relocation) = %s",
			  hex_string_custom (addr, 8));

      addr += displacement_from_map (ldm, addr);

      solib_debug_printf ("_dl_debug_addr (after relocation) = %s",
			  hex_string_custom (addr, 8));

      /* Fetch the address of the r_debug struct.  */
      if (target_read_memory (addr, addr_buf, sizeof addr_buf) != 0)
	{
	  warning (_("Unable to fetch contents of _dl_debug_addr "
		     "(at address %s) from dynamic linker"),
		   hex_string_custom (addr, 8));
	}
      addr = extract_unsigned_integer (addr_buf, sizeof addr_buf, byte_order);

      solib_debug_printf ("_dl_debug_addr[0..3] = %s",
			  hex_string_custom (addr, 8));

      /* If it's zero, then the ldso hasn't initialized yet, and so
	 there are no shared libs yet loaded.  */
      if (addr == 0)
	{
	  solib_debug_printf ("ldso not yet initialized");
	  /* Do not warn, but mark to run again.  */
	  return 0;
	}

      /* Fetch the r_brk field.  It's 8 bytes from the start of
	 _dl_debug_addr.  */
      if (target_read_memory (addr + 8, addr_buf, sizeof addr_buf) != 0)
	{
	  warning (_("Unable to fetch _dl_debug_addr->r_brk "
		     "(at address %s) from dynamic linker"),
		   hex_string_custom (addr + 8, 8));
	  enable_break_failure_warning ();
	  return 0;
	}
      addr = extract_unsigned_integer (addr_buf, sizeof addr_buf, byte_order);

      /* Now fetch the function entry point.  */
      if (target_read_memory (addr, addr_buf, sizeof addr_buf) != 0)
	{
	  warning (_("Unable to fetch _dl_debug_addr->.r_brk entry point "
		     "(at address %s) from dynamic linker"),
		   hex_string_custom (addr, 8));
	  enable_break_failure_warning ();
	  return 0;
	}
      addr = extract_unsigned_integer (addr_buf, sizeof addr_buf, byte_order);

      /* We're done with the loadmap.  */
      xfree (ldm);

      /* Remove all the solib event breakpoints.  Their addresses
	 may have changed since the last time we ran the program.  */
      remove_solib_event_breakpoints ();

      /* Now (finally!) create the solib breakpoint.  */
      create_solib_event_breakpoint (current_inferior ()->arch (), addr);

      enable_break2_done = 1;

      return 1;
    }

  /* Tell the user we couldn't set a dynamic linker breakpoint.  */
  enable_break_failure_warning ();

  /* Failure return.  */
  return 0;
}

static int
enable_break (void)
{
  asection *interp_sect;
  CORE_ADDR entry_point;

  if (current_program_space->symfile_object_file == NULL)
    {
      solib_debug_printf ("No symbol file found.");
      return 0;
    }

  if (!entry_point_address_query (&entry_point))
    {
      solib_debug_printf ("Symbol file has no entry point.");
      return 0;
    }

  /* Check for the presence of a .interp section.  If there is no
     such section, the executable is statically linked.  */

  interp_sect = bfd_get_section_by_name (current_program_space->exec_bfd (),
					 ".interp");

  if (interp_sect == NULL)
    {
      solib_debug_printf ("No .interp section found.");
      return 0;
    }

  create_solib_event_breakpoint (current_inferior ()->arch (), entry_point);

  solib_debug_printf ("solib event breakpoint placed at entry point: %s",
		      hex_string_custom (entry_point, 8));
  return 1;
}

static void
frv_relocate_main_executable (void)
{
  int status;
  CORE_ADDR exec_addr, interp_addr;
  struct int_elf32_fdpic_loadmap *ldm;
  int changed;

  status = frv_fdpic_loadmap_addresses (current_inferior ()->arch (),
					&interp_addr, &exec_addr);

  if (status < 0 || (exec_addr == 0 && interp_addr == 0))
    {
      /* Not using FDPIC ABI, so do nothing.  */
      return;
    }

  /* Fetch the loadmap located at ``exec_addr''.  */
  ldm = fetch_loadmap (exec_addr);
  if (ldm == NULL)
    error (_("Unable to load the executable's loadmap."));

  delete main_executable_lm_info;
  main_executable_lm_info = new lm_info_frv;
  main_executable_lm_info->map = ldm;

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
  main_executable_lm_info->got_value = main_got ();
}

/* Implement the "create_inferior_hook" target_solib_ops method.

   For the FR-V shared library ABI (FDPIC), the main executable needs
   to be relocated.  The shared library breakpoints also need to be
   enabled.  */

static void
frv_solib_create_inferior_hook (int from_tty)
{
  /* Relocate main executable.  */
  frv_relocate_main_executable ();

  /* Enable shared library breakpoints.  */
  if (!enable_break ())
    {
      warning (_("shared library handler failed to enable breakpoint"));
      return;
    }
}

static void
frv_clear_solib (program_space *pspace)
{
  lm_base_cache = 0;
  enable_break2_done = 0;
  main_lm_addr = 0;

  delete main_executable_lm_info;
  main_executable_lm_info = NULL;
}

static void
frv_relocate_section_addresses (shobj &so, target_section *sec)
{
  int seg;
  auto *li = gdb::checked_static_cast<lm_info_frv *> (so.lm_info.get ());
  int_elf32_fdpic_loadmap *map = li->map;

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

/* Return the GOT address associated with the main executable.  Return
   0 if it can't be found.  */

static CORE_ADDR
main_got (void)
{
  struct bound_minimal_symbol got_sym;

  objfile *objf = current_program_space->symfile_object_file;
  got_sym = lookup_minimal_symbol ("_GLOBAL_OFFSET_TABLE_", NULL, objf);
  if (got_sym.minsym == 0)
    return 0;

  return got_sym.value_address ();
}

/* Find the global pointer for the given function address ADDR.  */

CORE_ADDR
frv_fdpic_find_global_pointer (CORE_ADDR addr)
{
  for (const shobj &so : current_program_space->solibs ())
    {
      int seg;
      auto *li = gdb::checked_static_cast<lm_info_frv *> (so.lm_info.get ());
      int_elf32_fdpic_loadmap *map = li->map;

      for (seg = 0; seg < map->nsegs; seg++)
	{
	  if (map->segs[seg].addr <= addr
	      && addr < map->segs[seg].addr + map->segs[seg].p_memsz)
	    return li->got_value;
	}
    }

  /* Didn't find it in any of the shared objects.  So assume it's in the
     main executable.  */
  return main_got ();
}

/* Forward declarations for frv_fdpic_find_canonical_descriptor().  */
static CORE_ADDR find_canonical_descriptor_in_load_object
  (CORE_ADDR, CORE_ADDR, const char *, bfd *, lm_info_frv *);

/* Given a function entry point, attempt to find the canonical descriptor
   associated with that entry point.  Return 0 if no canonical descriptor
   could be found.  */

CORE_ADDR
frv_fdpic_find_canonical_descriptor (CORE_ADDR entry_point)
{
  const char *name;
  CORE_ADDR addr;
  CORE_ADDR got_value;
  struct symbol *sym;

  /* Fetch the corresponding global pointer for the entry point.  */
  got_value = frv_fdpic_find_global_pointer (entry_point);

  /* Attempt to find the name of the function.  If the name is available,
     it'll be used as an aid in finding matching functions in the dynamic
     symbol table.  */
  sym = find_pc_function (entry_point);
  if (sym == 0)
    name = 0;
  else
    name = sym->linkage_name ();

  /* Check the main executable.  */
  objfile *objf = current_program_space->symfile_object_file;
  addr = find_canonical_descriptor_in_load_object
	   (entry_point, got_value, name, objf->obfd.get (),
	    main_executable_lm_info);

  /* If descriptor not found via main executable, check each load object
     in list of shared objects.  */
  if (addr == 0)
    {
      for (const shobj &so : current_program_space->solibs ())
	{
	  auto *li = gdb::checked_static_cast<lm_info_frv *> (so.lm_info.get ());

	  addr = find_canonical_descriptor_in_load_object
		   (entry_point, got_value, name, so.abfd.get(), li);

	  if (addr != 0)
	    break;
	}
    }

  return addr;
}

static CORE_ADDR
find_canonical_descriptor_in_load_object
  (CORE_ADDR entry_point, CORE_ADDR got_value, const char *name, bfd *abfd,
   lm_info_frv *lm)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  arelent *rel;
  unsigned int i;
  CORE_ADDR addr = 0;

  /* Nothing to do if no bfd.  */
  if (abfd == 0)
    return 0;

  /* Nothing to do if no link map.  */
  if (lm == 0)
    return 0;

  /* We want to scan the dynamic relocs for R_FRV_FUNCDESC relocations.
     (More about this later.)  But in order to fetch the relocs, we
     need to first fetch the dynamic symbols.  These symbols need to
     be cached due to the way that bfd_canonicalize_dynamic_reloc()
     works.  (See the comments in the declaration of struct lm_info
     for more information.)  */
  if (lm->dyn_syms == NULL)
    {
      long storage_needed;
      unsigned int number_of_symbols;

      /* Determine amount of space needed to hold the dynamic symbol table.  */
      storage_needed = bfd_get_dynamic_symtab_upper_bound (abfd);

      /* If there are no dynamic symbols, there's nothing to do.  */
      if (storage_needed <= 0)
	return 0;

      /* Allocate space for the dynamic symbol table.  */
      lm->dyn_syms = (asymbol **) xmalloc (storage_needed);

      /* Fetch the dynamic symbol table.  */
      number_of_symbols = bfd_canonicalize_dynamic_symtab (abfd, lm->dyn_syms);

      if (number_of_symbols == 0)
	return 0;
    }

  /* Fetch the dynamic relocations if not already cached.  */
  if (lm->dyn_relocs == NULL)
    {
      long storage_needed;

      /* Determine amount of space needed to hold the dynamic relocs.  */
      storage_needed = bfd_get_dynamic_reloc_upper_bound (abfd);

      /* Bail out if there are no dynamic relocs.  */
      if (storage_needed <= 0)
	return 0;

      /* Allocate space for the relocs.  */
      lm->dyn_relocs = (arelent **) xmalloc (storage_needed);

      /* Fetch the dynamic relocs.  */
      lm->dyn_reloc_count 
	= bfd_canonicalize_dynamic_reloc (abfd, lm->dyn_relocs, lm->dyn_syms);
    }

  /* Search the dynamic relocs.  */
  for (i = 0; i < lm->dyn_reloc_count; i++)
    {
      rel = lm->dyn_relocs[i];

      /* Relocs of interest are those which meet the following
	 criteria:

	   - the names match (assuming the caller could provide
	     a name which matches ``entry_point'').
	   - the relocation type must be R_FRV_FUNCDESC.  Relocs
	     of this type are used (by the dynamic linker) to
	     look up the address of a canonical descriptor (allocating
	     it if need be) and initializing the GOT entry referred
	     to by the offset to the address of the descriptor.

	 These relocs of interest may be used to obtain a
	 candidate descriptor by first adjusting the reloc's
	 address according to the link map and then dereferencing
	 this address (which is a GOT entry) to obtain a descriptor
	 address.  */
      if ((name == 0 || strcmp (name, (*rel->sym_ptr_ptr)->name) == 0)
	  && rel->howto->type == R_FRV_FUNCDESC)
	{
	  gdb_byte buf [FRV_PTR_SIZE];

	  /* Compute address of address of candidate descriptor.  */
	  addr = rel->address + displacement_from_map (lm->map, rel->address);

	  /* Fetch address of candidate descriptor.  */
	  if (target_read_memory (addr, buf, sizeof buf) != 0)
	    continue;
	  addr = extract_unsigned_integer (buf, sizeof buf, byte_order);

	  /* Check for matching entry point.  */
	  if (target_read_memory (addr, buf, sizeof buf) != 0)
	    continue;
	  if (extract_unsigned_integer (buf, sizeof buf, byte_order)
	      != entry_point)
	    continue;

	  /* Check for matching got value.  */
	  if (target_read_memory (addr + 4, buf, sizeof buf) != 0)
	    continue;
	  if (extract_unsigned_integer (buf, sizeof buf, byte_order)
	      != got_value)
	    continue;

	  /* Match was successful!  Exit loop.  */
	  break;
	}
    }

  return addr;
}

/* Given an objfile, return the address of its link map.  This value is
   needed for TLS support.  */
CORE_ADDR
frv_fetch_objfile_link_map (struct objfile *objfile)
{
  /* Cause frv_current_sos() to be run if it hasn't been already.  */
  if (main_lm_addr == 0)
    solib_add (0, 0, 1);

  /* frv_current_sos() will set main_lm_addr for the main executable.  */
  if (objfile == current_program_space->symfile_object_file)
    return main_lm_addr;

  /* The other link map addresses may be found by examining the list
     of shared libraries.  */
  for (const shobj &so : current_program_space->solibs ())
    {
      auto *li = gdb::checked_static_cast<lm_info_frv *> (so.lm_info.get ());

      if (so.objfile == objfile)
	return li->lm_addr;
    }

  /* Not found!  */
  return 0;
}

const struct target_so_ops frv_so_ops =
{
  frv_relocate_section_addresses,
  nullptr,
  frv_clear_solib,
  frv_solib_create_inferior_hook,
  frv_current_sos,
  open_symbol_file_object,
  frv_in_dynsym_resolve_code,
  solib_bfd_open,
};
