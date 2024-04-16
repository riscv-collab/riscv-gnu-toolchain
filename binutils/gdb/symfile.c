/* Generic symbol file reading for the GNU debugger, GDB.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Support, using pieces from other GDB modules.

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
#include "arch-utils.h"
#include "bfdlink.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "frame.h"
#include "target.h"
#include "value.h"
#include "symfile.h"
#include "objfiles.h"
#include "source.h"
#include "gdbcmd.h"
#include "breakpoint.h"
#include "language.h"
#include "complaints.h"
#include "demangle.h"
#include "inferior.h"
#include "regcache.h"
#include "filenames.h"
#include "gdbsupport/gdb_obstack.h"
#include "completer.h"
#include "bcache.h"
#include "hashtab.h"
#include "readline/tilde.h"
#include "block.h"
#include "observable.h"
#include "exec.h"
#include "parser-defs.h"
#include "varobj.h"
#include "elf-bfd.h"
#include "solib.h"
#include "remote.h"
#include "stack.h"
#include "gdb_bfd.h"
#include "cli/cli-utils.h"
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/selftest.h"
#include "cli/cli-style.h"
#include "gdbsupport/forward-scope-exit.h"
#include "gdbsupport/buildargv.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <chrono>
#include <algorithm>

int (*deprecated_ui_load_progress_hook) (const char *section,
					 unsigned long num);
void (*deprecated_show_load_progress) (const char *section,
			    unsigned long section_sent,
			    unsigned long section_size,
			    unsigned long total_sent,
			    unsigned long total_size);
void (*deprecated_pre_add_symbol_hook) (const char *);
void (*deprecated_post_add_symbol_hook) (void);

using clear_symtab_users_cleanup
  = FORWARD_SCOPE_EXIT (clear_symtab_users);

/* Global variables owned by this file.  */

/* See symfile.h.  */

int readnow_symbol_files;

/* See symfile.h.  */

int readnever_symbol_files;

/* Functions this file defines.  */

static void symbol_file_add_main_1 (const char *args, symfile_add_flags add_flags,
				    objfile_flags flags, CORE_ADDR reloff);

static const struct sym_fns *find_sym_fns (bfd *);

static void overlay_invalidate_all (void);

static void simple_free_overlay_table (void);

static void read_target_long_array (CORE_ADDR, unsigned int *, int, int,
				    enum bfd_endian);

static int simple_read_overlay_table (void);

static int simple_overlay_update_1 (struct obj_section *);

static void symfile_find_segment_sections (struct objfile *objfile);

/* List of all available sym_fns.  On gdb startup, each object file reader
   calls add_symtab_fns() to register information on each format it is
   prepared to read.  */

struct registered_sym_fns
{
  registered_sym_fns (bfd_flavour sym_flavour_, const struct sym_fns *sym_fns_)
  : sym_flavour (sym_flavour_), sym_fns (sym_fns_)
  {}

  /* BFD flavour that we handle.  */
  enum bfd_flavour sym_flavour;

  /* The "vtable" of symbol functions.  */
  const struct sym_fns *sym_fns;
};

static std::vector<registered_sym_fns> symtab_fns;

/* Values for "set print symbol-loading".  */

const char print_symbol_loading_off[] = "off";
const char print_symbol_loading_brief[] = "brief";
const char print_symbol_loading_full[] = "full";
static const char *print_symbol_loading_enums[] =
{
  print_symbol_loading_off,
  print_symbol_loading_brief,
  print_symbol_loading_full,
  NULL
};
static const char *print_symbol_loading = print_symbol_loading_full;

/* See symfile.h.  */

bool auto_solib_add = true;


/* Return non-zero if symbol-loading messages should be printed.
   FROM_TTY is the standard from_tty argument to gdb commands.
   If EXEC is non-zero the messages are for the executable.
   Otherwise, messages are for shared libraries.
   If FULL is non-zero then the caller is printing a detailed message.
   E.g., the message includes the shared library name.
   Otherwise, the caller is printing a brief "summary" message.  */

int
print_symbol_loading_p (int from_tty, int exec, int full)
{
  if (!from_tty && !info_verbose)
    return 0;

  if (exec)
    {
      /* We don't check FULL for executables, there are few such
	 messages, therefore brief == full.  */
      return print_symbol_loading != print_symbol_loading_off;
    }
  if (full)
    return print_symbol_loading == print_symbol_loading_full;
  return print_symbol_loading == print_symbol_loading_brief;
}

/* True if we are reading a symbol table.  */

int currently_reading_symtab = 0;

/* Increment currently_reading_symtab and return a cleanup that can be
   used to decrement it.  */

scoped_restore_tmpl<int>
increment_reading_symtab (void)
{
  gdb_assert (currently_reading_symtab >= 0);
  return make_scoped_restore (&currently_reading_symtab,
			      currently_reading_symtab + 1);
}

/* Remember the lowest-addressed loadable section we've seen.

   In case of equal vmas, the section with the largest size becomes the
   lowest-addressed loadable section.

   If the vmas and sizes are equal, the last section is considered the
   lowest-addressed loadable section.  */

static void
find_lowest_section (asection *sect, asection **lowest)
{
  if (0 == (bfd_section_flags (sect) & (SEC_ALLOC | SEC_LOAD)))
    return;
  if (!*lowest)
    *lowest = sect;		/* First loadable section */
  else if (bfd_section_vma (*lowest) > bfd_section_vma (sect))
    *lowest = sect;		/* A lower loadable section */
  else if (bfd_section_vma (*lowest) == bfd_section_vma (sect)
	   && (bfd_section_size (*lowest) <= bfd_section_size (sect)))
    *lowest = sect;
}

/* Build (allocate and populate) a section_addr_info struct from
   an existing section table.  */

section_addr_info
build_section_addr_info_from_section_table (const std::vector<target_section> &table)
{
  section_addr_info sap;

  for (const target_section &stp : table)
    {
      struct bfd_section *asect = stp.the_bfd_section;
      bfd *abfd = asect->owner;

      if (bfd_section_flags (asect) & (SEC_ALLOC | SEC_LOAD)
	  && sap.size () < table.size ())
	sap.emplace_back (stp.addr,
			  bfd_section_name (asect),
			  gdb_bfd_section_index (abfd, asect));
    }

  return sap;
}

/* Create a section_addr_info from section offsets in ABFD.  */

static section_addr_info
build_section_addr_info_from_bfd (bfd *abfd)
{
  struct bfd_section *sec;

  section_addr_info sap;
  for (sec = abfd->sections; sec != NULL; sec = sec->next)
    if (bfd_section_flags (sec) & (SEC_ALLOC | SEC_LOAD))
      sap.emplace_back (bfd_section_vma (sec),
			bfd_section_name (sec),
			gdb_bfd_section_index (abfd, sec));

  return sap;
}

/* Create a section_addr_info from section offsets in OBJFILE.  */

section_addr_info
build_section_addr_info_from_objfile (const struct objfile *objfile)
{
  int i;

  /* Before reread_symbols gets rewritten it is not safe to call:
     gdb_assert (objfile->num_sections == bfd_count_sections (objfile->obfd));
     */
  section_addr_info sap
    = build_section_addr_info_from_bfd (objfile->obfd.get ());
  for (i = 0; i < sap.size (); i++)
    {
      int sectindex = sap[i].sectindex;

      sap[i].addr += objfile->section_offsets[sectindex];
    }
  return sap;
}

/* Initialize OBJFILE's sect_index_* members.  */

static void
init_objfile_sect_indices (struct objfile *objfile)
{
  asection *sect;
  int i;

  sect = bfd_get_section_by_name (objfile->obfd.get (), ".text");
  if (sect)
    objfile->sect_index_text = sect->index;

  sect = bfd_get_section_by_name (objfile->obfd.get (), ".data");
  if (sect)
    objfile->sect_index_data = sect->index;

  sect = bfd_get_section_by_name (objfile->obfd.get (), ".bss");
  if (sect)
    objfile->sect_index_bss = sect->index;

  sect = bfd_get_section_by_name (objfile->obfd.get (), ".rodata");
  if (sect)
    objfile->sect_index_rodata = sect->index;

  /* This is where things get really weird...  We MUST have valid
     indices for the various sect_index_* members or gdb will abort.
     So if for example, there is no ".text" section, we have to
     accommodate that.  First, check for a file with the standard
     one or two segments.  */

  symfile_find_segment_sections (objfile);

  /* Except when explicitly adding symbol files at some address,
     section_offsets contains nothing but zeros, so it doesn't matter
     which slot in section_offsets the individual sect_index_* members
     index into.  So if they are all zero, it is safe to just point
     all the currently uninitialized indices to the first slot.  But
     beware: if this is the main executable, it may be relocated
     later, e.g. by the remote qOffsets packet, and then this will
     be wrong!  That's why we try segments first.  */

  for (i = 0; i < objfile->section_offsets.size (); i++)
    {
      if (objfile->section_offsets[i] != 0)
	{
	  break;
	}
    }
  if (i == objfile->section_offsets.size ())
    {
      if (objfile->sect_index_text == -1)
	objfile->sect_index_text = 0;
      if (objfile->sect_index_data == -1)
	objfile->sect_index_data = 0;
      if (objfile->sect_index_bss == -1)
	objfile->sect_index_bss = 0;
      if (objfile->sect_index_rodata == -1)
	objfile->sect_index_rodata = 0;
    }
}

/* Find a unique offset to use for loadable section SECT if
   the user did not provide an offset.  */

static void
place_section (bfd *abfd, asection *sect, section_offsets &offsets,
	       CORE_ADDR &lowest)
{
  CORE_ADDR start_addr;
  int done;
  ULONGEST align = ((ULONGEST) 1) << bfd_section_alignment (sect);

  /* We are only interested in allocated sections.  */
  if ((bfd_section_flags (sect) & SEC_ALLOC) == 0)
    return;

  /* If the user specified an offset, honor it.  */
  if (offsets[gdb_bfd_section_index (abfd, sect)] != 0)
    return;

  /* Otherwise, let's try to find a place for the section.  */
  start_addr = (lowest + align - 1) & -align;

  do {
    asection *cur_sec;

    done = 1;

    for (cur_sec = abfd->sections; cur_sec != NULL; cur_sec = cur_sec->next)
      {
	int indx = cur_sec->index;

	/* We don't need to compare against ourself.  */
	if (cur_sec == sect)
	  continue;

	/* We can only conflict with allocated sections.  */
	if ((bfd_section_flags (cur_sec) & SEC_ALLOC) == 0)
	  continue;

	/* If the section offset is 0, either the section has not been placed
	   yet, or it was the lowest section placed (in which case LOWEST
	   will be past its end).  */
	if (offsets[indx] == 0)
	  continue;

	/* If this section would overlap us, then we must move up.  */
	if (start_addr + bfd_section_size (sect) > offsets[indx]
	    && start_addr < offsets[indx] + bfd_section_size (cur_sec))
	  {
	    start_addr = offsets[indx] + bfd_section_size (cur_sec);
	    start_addr = (start_addr + align - 1) & -align;
	    done = 0;
	    break;
	  }

	/* Otherwise, we appear to be OK.  So far.  */
      }
    }
  while (!done);

  offsets[gdb_bfd_section_index (abfd, sect)] = start_addr;
  lowest = start_addr + bfd_section_size (sect);
}

/* Store section_addr_info as prepared (made relative and with SECTINDEX
   filled-in) by addr_info_make_relative into SECTION_OFFSETS.  */

void
relative_addr_info_to_section_offsets (section_offsets &section_offsets,
				       const section_addr_info &addrs)
{
  int i;

  section_offsets.assign (section_offsets.size (), 0);

  /* Now calculate offsets for section that were specified by the caller.  */
  for (i = 0; i < addrs.size (); i++)
    {
      const struct other_sections *osp;

      osp = &addrs[i];
      if (osp->sectindex == -1)
	continue;

      /* Record all sections in offsets.  */
      /* The section_offsets in the objfile are here filled in using
	 the BFD index.  */
      section_offsets[osp->sectindex] = osp->addr;
    }
}

/* Transform section name S for a name comparison.  prelink can split section
   `.bss' into two sections `.dynbss' and `.bss' (in this order).  Similarly
   prelink can split `.sbss' into `.sdynbss' and `.sbss'.  Use virtual address
   of the new `.dynbss' (`.sdynbss') section as the adjacent new `.bss'
   (`.sbss') section has invalid (increased) virtual address.  */

static const char *
addr_section_name (const char *s)
{
  if (strcmp (s, ".dynbss") == 0)
    return ".bss";
  if (strcmp (s, ".sdynbss") == 0)
    return ".sbss";

  return s;
}

/* std::sort comparator for addrs_section_sort.  Sort entries in
   ascending order by their (name, sectindex) pair.  sectindex makes
   the sort by name stable.  */

static bool
addrs_section_compar (const struct other_sections *a,
		      const struct other_sections *b)
{
  int retval;

  retval = strcmp (addr_section_name (a->name.c_str ()),
		   addr_section_name (b->name.c_str ()));
  if (retval != 0)
    return retval < 0;

  return a->sectindex < b->sectindex;
}

/* Provide sorted array of pointers to sections of ADDRS.  */

static std::vector<const struct other_sections *>
addrs_section_sort (const section_addr_info &addrs)
{
  int i;

  std::vector<const struct other_sections *> array (addrs.size ());
  for (i = 0; i < addrs.size (); i++)
    array[i] = &addrs[i];

  std::sort (array.begin (), array.end (), addrs_section_compar);

  return array;
}

/* Relativize absolute addresses in ADDRS into offsets based on ABFD.  Fill-in
   also SECTINDEXes specific to ABFD there.  This function can be used to
   rebase ADDRS to start referencing different BFD than before.  */

void
addr_info_make_relative (section_addr_info *addrs, bfd *abfd)
{
  asection *lower_sect;
  CORE_ADDR lower_offset;
  int i;

  /* Find lowest loadable section to be used as starting point for
     contiguous sections.  */
  lower_sect = NULL;
  for (asection *iter : gdb_bfd_sections (abfd))
    find_lowest_section (iter, &lower_sect);
  if (lower_sect == NULL)
    {
      warning (_("no loadable sections found in added symbol-file %s"),
	       bfd_get_filename (abfd));
      lower_offset = 0;
    }
  else
    lower_offset = bfd_section_vma (lower_sect);

  /* Create ADDRS_TO_ABFD_ADDRS array to map the sections in ADDRS to sections
     in ABFD.  Section names are not unique - there can be multiple sections of
     the same name.  Also the sections of the same name do not have to be
     adjacent to each other.  Some sections may be present only in one of the
     files.  Even sections present in both files do not have to be in the same
     order.

     Use stable sort by name for the sections in both files.  Then linearly
     scan both lists matching as most of the entries as possible.  */

  std::vector<const struct other_sections *> addrs_sorted
    = addrs_section_sort (*addrs);

  section_addr_info abfd_addrs = build_section_addr_info_from_bfd (abfd);
  std::vector<const struct other_sections *> abfd_addrs_sorted
    = addrs_section_sort (abfd_addrs);

  /* Now create ADDRS_TO_ABFD_ADDRS from ADDRS_SORTED and
     ABFD_ADDRS_SORTED.  */

  std::vector<const struct other_sections *>
    addrs_to_abfd_addrs (addrs->size (), nullptr);

  std::vector<const struct other_sections *>::iterator abfd_sorted_iter
    = abfd_addrs_sorted.begin ();
  for (const other_sections *sect : addrs_sorted)
    {
      const char *sect_name = addr_section_name (sect->name.c_str ());

      while (abfd_sorted_iter != abfd_addrs_sorted.end ()
	     && strcmp (addr_section_name ((*abfd_sorted_iter)->name.c_str ()),
			sect_name) < 0)
	abfd_sorted_iter++;

      if (abfd_sorted_iter != abfd_addrs_sorted.end ()
	  && strcmp (addr_section_name ((*abfd_sorted_iter)->name.c_str ()),
		     sect_name) == 0)
	{
	  int index_in_addrs;

	  /* Make the found item directly addressable from ADDRS.  */
	  index_in_addrs = sect - addrs->data ();
	  gdb_assert (addrs_to_abfd_addrs[index_in_addrs] == NULL);
	  addrs_to_abfd_addrs[index_in_addrs] = *abfd_sorted_iter;

	  /* Never use the same ABFD entry twice.  */
	  abfd_sorted_iter++;
	}
    }

  /* Calculate offsets for the loadable sections.
     FIXME! Sections must be in order of increasing loadable section
     so that contiguous sections can use the lower-offset!!!

     Adjust offsets if the segments are not contiguous.
     If the section is contiguous, its offset should be set to
     the offset of the highest loadable section lower than it
     (the loadable section directly below it in memory).
     this_offset = lower_offset = lower_addr - lower_orig_addr */

  for (i = 0; i < addrs->size (); i++)
    {
      const struct other_sections *sect = addrs_to_abfd_addrs[i];

      if (sect)
	{
	  /* This is the index used by BFD.  */
	  (*addrs)[i].sectindex = sect->sectindex;

	  if ((*addrs)[i].addr != 0)
	    {
	      (*addrs)[i].addr -= sect->addr;
	      lower_offset = (*addrs)[i].addr;
	    }
	  else
	    (*addrs)[i].addr = lower_offset;
	}
      else
	{
	  /* addr_section_name transformation is not used for SECT_NAME.  */
	  const std::string &sect_name = (*addrs)[i].name;

	  /* This section does not exist in ABFD, which is normally
	     unexpected and we want to issue a warning.

	     However, the ELF prelinker does create a few sections which are
	     marked in the main executable as loadable (they are loaded in
	     memory from the DYNAMIC segment) and yet are not present in
	     separate debug info files.  This is fine, and should not cause
	     a warning.  Shared libraries contain just the section
	     ".gnu.liblist" but it is not marked as loadable there.  There is
	     no other way to identify them than by their name as the sections
	     created by prelink have no special flags.

	     For the sections `.bss' and `.sbss' see addr_section_name.  */

	  if (!(sect_name == ".gnu.liblist"
		|| sect_name == ".gnu.conflict"
		|| (sect_name == ".bss"
		    && i > 0
		    && (*addrs)[i - 1].name == ".dynbss"
		    && addrs_to_abfd_addrs[i - 1] != NULL)
		|| (sect_name == ".sbss"
		    && i > 0
		    && (*addrs)[i - 1].name == ".sdynbss"
		    && addrs_to_abfd_addrs[i - 1] != NULL)))
	    warning (_("section %s not found in %s"), sect_name.c_str (),
		     bfd_get_filename (abfd));

	  (*addrs)[i].addr = 0;
	  (*addrs)[i].sectindex = -1;
	}
    }
}

/* Parse the user's idea of an offset for dynamic linking, into our idea
   of how to represent it for fast symbol reading.  This is the default
   version of the sym_fns.sym_offsets function for symbol readers that
   don't need to do anything special.  It allocates a section_offsets table
   for the objectfile OBJFILE and stuffs ADDR into all of the offsets.  */

void
default_symfile_offsets (struct objfile *objfile,
			 const section_addr_info &addrs)
{
  objfile->section_offsets.resize (gdb_bfd_count_sections (objfile->obfd.get ()));
  relative_addr_info_to_section_offsets (objfile->section_offsets, addrs);

  /* For relocatable files, all loadable sections will start at zero.
     The zero is meaningless, so try to pick arbitrary addresses such
     that no loadable sections overlap.  This algorithm is quadratic,
     but the number of sections in a single object file is generally
     small.  */
  if ((bfd_get_file_flags (objfile->obfd.get ()) & (EXEC_P | DYNAMIC)) == 0)
    {
      bfd *abfd = objfile->obfd.get ();
      asection *cur_sec;

      for (cur_sec = abfd->sections; cur_sec != NULL; cur_sec = cur_sec->next)
	/* We do not expect this to happen; just skip this step if the
	   relocatable file has a section with an assigned VMA.  */
	if (bfd_section_vma (cur_sec) != 0)
	  break;

      if (cur_sec == NULL)
	{
	  section_offsets &offsets = objfile->section_offsets;

	  /* Pick non-overlapping offsets for sections the user did not
	     place explicitly.  */
	  CORE_ADDR lowest = 0;
	  for (asection *sect : gdb_bfd_sections (objfile->obfd.get ()))
	    place_section (objfile->obfd.get (), sect, objfile->section_offsets,
			   lowest);

	  /* Correctly filling in the section offsets is not quite
	     enough.  Relocatable files have two properties that
	     (most) shared objects do not:

	     - Their debug information will contain relocations.  Some
	     shared libraries do also, but many do not, so this can not
	     be assumed.

	     - If there are multiple code sections they will be loaded
	     at different relative addresses in memory than they are
	     in the objfile, since all sections in the file will start
	     at address zero.

	     Because GDB has very limited ability to map from an
	     address in debug info to the correct code section,
	     it relies on adding SECT_OFF_TEXT to things which might be
	     code.  If we clear all the section offsets, and set the
	     section VMAs instead, then symfile_relocate_debug_section
	     will return meaningful debug information pointing at the
	     correct sections.

	     GDB has too many different data structures for section
	     addresses - a bfd, objfile, and so_list all have section
	     tables, as does exec_ops.  Some of these could probably
	     be eliminated.  */

	  for (cur_sec = abfd->sections; cur_sec != NULL;
	       cur_sec = cur_sec->next)
	    {
	      if ((bfd_section_flags (cur_sec) & SEC_ALLOC) == 0)
		continue;

	      bfd_set_section_vma (cur_sec, offsets[cur_sec->index]);
	      exec_set_section_address (bfd_get_filename (abfd),
					cur_sec->index,
					offsets[cur_sec->index]);
	      offsets[cur_sec->index] = 0;
	    }
	}
    }

  /* Remember the bfd indexes for the .text, .data, .bss and
     .rodata sections.  */
  init_objfile_sect_indices (objfile);
}

/* Divide the file into segments, which are individual relocatable units.
   This is the default version of the sym_fns.sym_segments function for
   symbol readers that do not have an explicit representation of segments.
   It assumes that object files do not have segments, and fully linked
   files have a single segment.  */

symfile_segment_data_up
default_symfile_segments (bfd *abfd)
{
  int num_sections, i;
  asection *sect;
  CORE_ADDR low, high;

  /* Relocatable files contain enough information to position each
     loadable section independently; they should not be relocated
     in segments.  */
  if ((bfd_get_file_flags (abfd) & (EXEC_P | DYNAMIC)) == 0)
    return NULL;

  /* Make sure there is at least one loadable section in the file.  */
  for (sect = abfd->sections; sect != NULL; sect = sect->next)
    {
      if ((bfd_section_flags (sect) & SEC_ALLOC) == 0)
	continue;

      break;
    }
  if (sect == NULL)
    return NULL;

  low = bfd_section_vma (sect);
  high = low + bfd_section_size (sect);

  symfile_segment_data_up data (new symfile_segment_data);

  num_sections = bfd_count_sections (abfd);

  /* All elements are initialized to 0 (map to no segment).  */
  data->segment_info.resize (num_sections);

  for (i = 0, sect = abfd->sections; sect != NULL; i++, sect = sect->next)
    {
      CORE_ADDR vma;

      if ((bfd_section_flags (sect) & SEC_ALLOC) == 0)
	continue;

      vma = bfd_section_vma (sect);
      if (vma < low)
	low = vma;
      if (vma + bfd_section_size (sect) > high)
	high = vma + bfd_section_size (sect);

      data->segment_info[i] = 1;
    }

  data->segments.emplace_back (low, high - low);

  return data;
}

/* This is a convenience function to call sym_read for OBJFILE and
   possibly force the partial symbols to be read.  */

static void
read_symbols (struct objfile *objfile, symfile_add_flags add_flags)
{
  (*objfile->sf->sym_read) (objfile, add_flags);
  objfile->per_bfd->minsyms_read = true;

  /* find_separate_debug_file_in_section should be called only if there is
     single binary with no existing separate debug info file.  */
  if (!objfile->has_partial_symbols ()
      && objfile->separate_debug_objfile == NULL
      && objfile->separate_debug_objfile_backlink == NULL)
    {
      gdb_bfd_ref_ptr abfd (find_separate_debug_file_in_section (objfile));

      if (abfd != NULL)
	{
	  /* find_separate_debug_file_in_section uses the same filename for the
	     virtual section-as-bfd like the bfd filename containing the
	     section.  Therefore use also non-canonical name form for the same
	     file containing the section.  */
	  symbol_file_add_separate (abfd, bfd_get_filename (abfd.get ()),
				    add_flags | SYMFILE_NOT_FILENAME, objfile);
	}
    }
}

/* Initialize entry point information for this objfile.  */

static void
init_entry_point_info (struct objfile *objfile)
{
  struct entry_info *ei = &objfile->per_bfd->ei;

  if (ei->initialized)
    return;
  ei->initialized = 1;

  /* Save startup file's range of PC addresses to help blockframe.c
     decide where the bottom of the stack is.  */

  if (bfd_get_file_flags (objfile->obfd.get ()) & EXEC_P)
    {
      /* Executable file -- record its entry point so we'll recognize
	 the startup file because it contains the entry point.  */
      ei->entry_point = bfd_get_start_address (objfile->obfd.get ());
      ei->entry_point_p = 1;
    }
  else if (bfd_get_file_flags (objfile->obfd.get ()) & DYNAMIC
	   && bfd_get_start_address (objfile->obfd.get ()) != 0)
    {
      /* Some shared libraries may have entry points set and be
	 runnable.  There's no clear way to indicate this, so just check
	 for values other than zero.  */
      ei->entry_point = bfd_get_start_address (objfile->obfd.get ());
      ei->entry_point_p = 1;
    }
  else
    {
      /* Examination of non-executable.o files.  Short-circuit this stuff.  */
      ei->entry_point_p = 0;
    }

  if (ei->entry_point_p)
    {
      CORE_ADDR entry_point =  ei->entry_point;
      int found;

      /* Make certain that the address points at real code, and not a
	 function descriptor.  */
      entry_point = gdbarch_convert_from_func_ptr_addr
	(objfile->arch (), entry_point, current_inferior ()->top_target ());

      /* Remove any ISA markers, so that this matches entries in the
	 symbol table.  */
      ei->entry_point
	= gdbarch_addr_bits_remove (objfile->arch (), entry_point);

      found = 0;
      for (obj_section *osect : objfile->sections ())
	{
	  struct bfd_section *sect = osect->the_bfd_section;

	  if (entry_point >= bfd_section_vma (sect)
	      && entry_point < (bfd_section_vma (sect)
				+ bfd_section_size (sect)))
	    {
	      ei->the_bfd_section_index
		= gdb_bfd_section_index (objfile->obfd.get (), sect);
	      found = 1;
	      break;
	    }
	}

      if (!found)
	ei->the_bfd_section_index = SECT_OFF_TEXT (objfile);
    }
}

/* Process a symbol file, as either the main file or as a dynamically
   loaded file.

   This function does not set the OBJFILE's entry-point info.

   OBJFILE is where the symbols are to be read from.

   ADDRS is the list of section load addresses.  If the user has given
   an 'add-symbol-file' command, then this is the list of offsets and
   addresses he or she provided as arguments to the command; or, if
   we're handling a shared library, these are the actual addresses the
   sections are loaded at, according to the inferior's dynamic linker
   (as gleaned by GDB's shared library code).  We convert each address
   into an offset from the section VMA's as it appears in the object
   file, and then call the file's sym_offsets function to convert this
   into a format-specific offset table --- a `section_offsets'.
   The sectindex field is used to control the ordering of sections
   with the same name.  Upon return, it is updated to contain the
   corresponding BFD section index, or -1 if the section was not found.

   ADD_FLAGS encodes verbosity level, whether this is main symbol or
   an extra symbol file such as dynamically loaded code, and whether
   breakpoint reset should be deferred.  */

static void
syms_from_objfile_1 (struct objfile *objfile,
		     section_addr_info *addrs,
		     symfile_add_flags add_flags)
{
  section_addr_info local_addr;
  const int mainline = add_flags & SYMFILE_MAINLINE;

  objfile_set_sym_fns (objfile, find_sym_fns (objfile->obfd.get ()));
  objfile->qf.clear ();

  if (objfile->sf == NULL)
    {
      /* No symbols to load, but we still need to make sure
	 that the section_offsets table is allocated.  */
      int num_sections = gdb_bfd_count_sections (objfile->obfd.get ());

      objfile->section_offsets.assign (num_sections, 0);
      return;
    }

  /* Make sure that partially constructed symbol tables will be cleaned up
     if an error occurs during symbol reading.  */
  std::optional<clear_symtab_users_cleanup> defer_clear_users;

  objfile_up objfile_holder (objfile);

  /* If ADDRS is NULL, put together a dummy address list.
     We now establish the convention that an addr of zero means
     no load address was specified.  */
  if (! addrs)
    addrs = &local_addr;

  if (mainline)
    {
      /* We will modify the main symbol table, make sure that all its users
	 will be cleaned up if an error occurs during symbol reading.  */
      defer_clear_users.emplace ((symfile_add_flag) 0);

      /* Since no error yet, throw away the old symbol table.  */

      if (current_program_space->symfile_object_file != NULL)
	{
	  current_program_space->symfile_object_file->unlink ();
	  gdb_assert (current_program_space->symfile_object_file == NULL);
	}

      /* Currently we keep symbols from the add-symbol-file command.
	 If the user wants to get rid of them, they should do "symbol-file"
	 without arguments first.  Not sure this is the best behavior
	 (PR 2207).  */

      (*objfile->sf->sym_new_init) (objfile);
    }

  /* Convert addr into an offset rather than an absolute address.
     We find the lowest address of a loaded segment in the objfile,
     and assume that <addr> is where that got loaded.

     We no longer warn if the lowest section is not a text segment (as
     happens for the PA64 port.  */
  if (addrs->size () > 0)
    addr_info_make_relative (addrs, objfile->obfd.get ());

  /* Initialize symbol reading routines for this objfile, allow complaints to
     appear for this new file, and record how verbose to be, then do the
     initial symbol reading for this file.  */

  (*objfile->sf->sym_init) (objfile);
  clear_complaints ();

  (*objfile->sf->sym_offsets) (objfile, *addrs);

  read_symbols (objfile, add_flags);

  /* Discard cleanups as symbol reading was successful.  */

  objfile_holder.release ();
  if (defer_clear_users)
    defer_clear_users->release ();
}

/* Same as syms_from_objfile_1, but also initializes the objfile
   entry-point info.  */

static void
syms_from_objfile (struct objfile *objfile,
		   section_addr_info *addrs,
		   symfile_add_flags add_flags)
{
  syms_from_objfile_1 (objfile, addrs, add_flags);
  init_entry_point_info (objfile);
}

/* Perform required actions after either reading in the initial
   symbols for a new objfile, or mapping in the symbols from a reusable
   objfile.  ADD_FLAGS is a bitmask of enum symfile_add_flags.  */

static void
finish_new_objfile (struct objfile *objfile, symfile_add_flags add_flags)
{
  /* If this is the main symbol file we have to clean up all users of the
     old main symbol file.  Otherwise it is sufficient to fixup all the
     breakpoints that may have been redefined by this symbol file.  */
  if (add_flags & SYMFILE_MAINLINE)
    {
      /* OK, make it the "real" symbol file.  */
      current_program_space->symfile_object_file = objfile;

      clear_symtab_users (add_flags);
    }
  else if ((add_flags & SYMFILE_DEFER_BP_RESET) == 0)
    {
      breakpoint_re_set ();
    }

  /* We're done reading the symbol file; finish off complaints.  */
  clear_complaints ();
}

/* Process a symbol file, as either the main file or as a dynamically
   loaded file.

   ABFD is a BFD already open on the file, as from symfile_bfd_open.
   A new reference is acquired by this function.

   For NAME description see the objfile constructor.

   ADD_FLAGS encodes verbosity, whether this is main symbol file or
   extra, such as dynamically loaded code, and what to do with breakpoints.

   ADDRS is as described for syms_from_objfile_1, above.
   ADDRS is ignored when SYMFILE_MAINLINE bit is set in ADD_FLAGS.

   PARENT is the original objfile if ABFD is a separate debug info file.
   Otherwise PARENT is NULL.

   Upon success, returns a pointer to the objfile that was added.
   Upon failure, jumps back to command level (never returns).  */

static struct objfile *
symbol_file_add_with_addrs (const gdb_bfd_ref_ptr &abfd, const char *name,
			    symfile_add_flags add_flags,
			    section_addr_info *addrs,
			    objfile_flags flags, struct objfile *parent)
{
  struct objfile *objfile;
  const int from_tty = add_flags & SYMFILE_VERBOSE;
  const int mainline = add_flags & SYMFILE_MAINLINE;
  const int always_confirm = add_flags & SYMFILE_ALWAYS_CONFIRM;
  const int should_print = (print_symbol_loading_p (from_tty, mainline, 1)
			    && (readnow_symbol_files
				|| (add_flags & SYMFILE_NO_READ) == 0));

  if (readnow_symbol_files)
    {
      flags |= OBJF_READNOW;
      add_flags &= ~SYMFILE_NO_READ;
    }
  else if (readnever_symbol_files
	   || (parent != NULL && (parent->flags & OBJF_READNEVER)))
    {
      flags |= OBJF_READNEVER;
      add_flags |= SYMFILE_NO_READ;
    }
  if ((add_flags & SYMFILE_NOT_FILENAME) != 0)
    flags |= OBJF_NOT_FILENAME;

  /* Give user a chance to burp if ALWAYS_CONFIRM or we'd be
     interactively wiping out any existing symbols.  */

  if (from_tty
      && (always_confirm
	  || ((have_full_symbols () || have_partial_symbols ())
	      && mainline))
      && !query (_("Load new symbol table from \"%s\"? "), name))
    error (_("Not confirmed."));

  if (mainline)
    flags |= OBJF_MAINLINE;
  objfile = objfile::make (abfd, name, flags, parent);

  /* We either created a new mapped symbol table, mapped an existing
     symbol table file which has not had initial symbol reading
     performed, or need to read an unmapped symbol table.  */
  if (should_print)
    {
      if (deprecated_pre_add_symbol_hook)
	deprecated_pre_add_symbol_hook (name);
      else
	gdb_printf (_("Reading symbols from %ps...\n"),
		    styled_string (file_name_style.style (), name));
    }
  syms_from_objfile (objfile, addrs, add_flags);

  /* We now have at least a partial symbol table.  Check to see if the
     user requested that all symbols be read on initial access via either
     the gdb startup command line or on a per symbol file basis.  Expand
     all partial symbol tables for this objfile if so.  */

  if ((flags & OBJF_READNOW))
    {
      if (should_print)
	gdb_printf (_("Expanding full symbols from %ps...\n"),
		    styled_string (file_name_style.style (), name));

      objfile->expand_all_symtabs ();
    }

  /* Note that we only print a message if we have no symbols and have
     no separate debug file.  If there is a separate debug file which
     does not have symbols, we'll have emitted this message for that
     file, and so printing it twice is just redundant.  */
  if (should_print && !objfile_has_symbols (objfile)
      && objfile->separate_debug_objfile == nullptr)
    gdb_printf (_("(No debugging symbols found in %ps)\n"),
		styled_string (file_name_style.style (), name));

  if (should_print)
    {
      if (deprecated_post_add_symbol_hook)
	deprecated_post_add_symbol_hook ();
    }

  /* We print some messages regardless of whether 'from_tty ||
     info_verbose' is true, so make sure they go out at the right
     time.  */
  gdb_flush (gdb_stdout);

  if (objfile->sf != nullptr)
    finish_new_objfile (objfile, add_flags);

  gdb::observers::new_objfile.notify (objfile);

  return objfile;
}

/* Add BFD as a separate debug file for OBJFILE.  For NAME description
   see the objfile constructor.  */

void
symbol_file_add_separate (const gdb_bfd_ref_ptr &bfd, const char *name,
			  symfile_add_flags symfile_flags,
			  struct objfile *objfile)
{
  /* Create section_addr_info.  We can't directly use offsets from OBJFILE
     because sections of BFD may not match sections of OBJFILE and because
     vma may have been modified by tools such as prelink.  */
  section_addr_info sap = build_section_addr_info_from_objfile (objfile);

  symbol_file_add_with_addrs
    (bfd, name, symfile_flags, &sap,
     objfile->flags & (OBJF_SHARED | OBJF_READNOW
		       | OBJF_USERLOADED | OBJF_MAINLINE),
     objfile);
}

/* Process the symbol file ABFD, as either the main file or as a
   dynamically loaded file.
   See symbol_file_add_with_addrs's comments for details.  */

struct objfile *
symbol_file_add_from_bfd (const gdb_bfd_ref_ptr &abfd, const char *name,
			  symfile_add_flags add_flags,
			  section_addr_info *addrs,
			  objfile_flags flags, struct objfile *parent)
{
  return symbol_file_add_with_addrs (abfd, name, add_flags, addrs, flags,
				     parent);
}

/* Process a symbol file, as either the main file or as a dynamically
   loaded file.  See symbol_file_add_with_addrs's comments for details.  */

struct objfile *
symbol_file_add (const char *name, symfile_add_flags add_flags,
		 section_addr_info *addrs, objfile_flags flags)
{
  gdb_bfd_ref_ptr bfd (symfile_bfd_open (name));

  return symbol_file_add_from_bfd (bfd, name, add_flags, addrs,
				   flags, NULL);
}

/* Call symbol_file_add() with default values and update whatever is
   affected by the loading of a new main().
   Used when the file is supplied in the gdb command line
   and by some targets with special loading requirements.
   The auxiliary function, symbol_file_add_main_1(), has the flags
   argument for the switches that can only be specified in the symbol_file
   command itself.  */

void
symbol_file_add_main (const char *args, symfile_add_flags add_flags)
{
  symbol_file_add_main_1 (args, add_flags, 0, 0);
}

static void
symbol_file_add_main_1 (const char *args, symfile_add_flags add_flags,
			objfile_flags flags, CORE_ADDR reloff)
{
  add_flags |= current_inferior ()->symfile_flags | SYMFILE_MAINLINE;

  struct objfile *objfile = symbol_file_add (args, add_flags, NULL, flags);
  if (reloff != 0)
    objfile_rebase (objfile, reloff);

  /* Getting new symbols may change our opinion about
     what is frameless.  */
  reinit_frame_cache ();

  if ((add_flags & SYMFILE_NO_READ) == 0)
    set_initial_language ();
}

void
symbol_file_clear (int from_tty)
{
  if ((have_full_symbols () || have_partial_symbols ())
      && from_tty
      && (current_program_space->symfile_object_file
	  ? !query (_("Discard symbol table from `%s'? "),
		    objfile_name (current_program_space->symfile_object_file))
	  : !query (_("Discard symbol table? "))))
    error (_("Not confirmed."));

  /* solib descriptors may have handles to objfiles.  Wipe them before their
     objfiles get stale by free_all_objfiles.  */
  no_shared_libraries (NULL, from_tty);

  current_program_space->free_all_objfiles ();

  clear_symtab_users (0);

  gdb_assert (current_program_space->symfile_object_file == NULL);
  if (from_tty)
    gdb_printf (_("No symbol file now.\n"));
}

/* See symfile.h.  */

bool separate_debug_file_debug = false;

static int
separate_debug_file_exists (const std::string &name, unsigned long crc,
			    struct objfile *parent_objfile,
			    deferred_warnings *warnings)
{
  unsigned long file_crc;
  int file_crc_p;
  struct stat parent_stat, abfd_stat;
  int verified_as_different;

  /* Find a separate debug info file as if symbols would be present in
     PARENT_OBJFILE itself this function would not be called.  .gnu_debuglink
     section can contain just the basename of PARENT_OBJFILE without any
     ".debug" suffix as "/usr/lib/debug/path/to/file" is a separate tree where
     the separate debug infos with the same basename can exist.  */

  if (filename_cmp (name.c_str (), objfile_name (parent_objfile)) == 0)
    return 0;

  if (separate_debug_file_debug)
    {
      gdb_printf (gdb_stdlog, _("  Trying %s..."), name.c_str ());
      gdb_flush (gdb_stdlog);
    }

  gdb_bfd_ref_ptr abfd (gdb_bfd_open (name.c_str (), gnutarget));

  if (abfd == NULL)
    {
      if (separate_debug_file_debug)
	gdb_printf (gdb_stdlog, _(" no, unable to open.\n"));

      return 0;
    }

  /* Verify symlinks were not the cause of filename_cmp name difference above.

     Some operating systems, e.g. Windows, do not provide a meaningful
     st_ino; they always set it to zero.  (Windows does provide a
     meaningful st_dev.)  Files accessed from gdbservers that do not
     support the vFile:fstat packet will also have st_ino set to zero.
     Do not indicate a duplicate library in either case.  While there
     is no guarantee that a system that provides meaningful inode
     numbers will never set st_ino to zero, this is merely an
     optimization, so we do not need to worry about false negatives.  */

  if (bfd_stat (abfd.get (), &abfd_stat) == 0
      && abfd_stat.st_ino != 0
      && bfd_stat (parent_objfile->obfd.get (), &parent_stat) == 0)
    {
      if (abfd_stat.st_dev == parent_stat.st_dev
	  && abfd_stat.st_ino == parent_stat.st_ino)
	{
	  if (separate_debug_file_debug)
	    gdb_printf (gdb_stdlog,
			_(" no, same file as the objfile.\n"));

	  return 0;
	}
      verified_as_different = 1;
    }
  else
    verified_as_different = 0;

  file_crc_p = gdb_bfd_crc (abfd.get (), &file_crc);

  if (!file_crc_p)
    {
      if (separate_debug_file_debug)
	gdb_printf (gdb_stdlog, _(" no, error computing CRC.\n"));

      return 0;
    }

  if (crc != file_crc)
    {
      unsigned long parent_crc;

      /* If the files could not be verified as different with
	 bfd_stat then we need to calculate the parent's CRC
	 to verify whether the files are different or not.  */

      if (!verified_as_different)
	{
	  if (!gdb_bfd_crc (parent_objfile->obfd.get (), &parent_crc))
	    {
	      if (separate_debug_file_debug)
		gdb_printf (gdb_stdlog,
			    _(" no, error computing CRC.\n"));

	      return 0;
	    }
	}

      if (verified_as_different || parent_crc != file_crc)
	{
	  if (separate_debug_file_debug)
	    gdb_printf (gdb_stdlog, "the debug information found in \"%s\""
			" does not match \"%s\" (CRC mismatch).\n",
			name.c_str (), objfile_name (parent_objfile));
	  warnings->warn (_("the debug information found in \"%ps\""
			    " does not match \"%ps\" (CRC mismatch)."),
			  styled_string (file_name_style.style (),
					 name.c_str ()),
			  styled_string (file_name_style.style (),
					 objfile_name (parent_objfile)));
	}

      return 0;
    }

  if (separate_debug_file_debug)
    gdb_printf (gdb_stdlog, _(" yes!\n"));

  return 1;
}

std::string debug_file_directory;
static void
show_debug_file_directory (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("The directory where separate debug "
		"symbols are searched for is \"%s\".\n"),
	      value);
}

#if ! defined (DEBUG_SUBDIRECTORY)
#define DEBUG_SUBDIRECTORY ".debug"
#endif

/* Find a separate debuginfo file for OBJFILE, using DIR as the directory
   where the original file resides (may not be the same as
   dirname(objfile->name) due to symlinks), and DEBUGLINK as the file we are
   looking for.  CANON_DIR is the "realpath" form of DIR.
   DIR must contain a trailing '/'.
   Returns the path of the file with separate debug info, or an empty
   string.

   Any warnings generated as part of the lookup process are added to
   WARNINGS.  If some other mechanism can be used to lookup the debug
   information then the warning will not be shown, however, if GDB fails to
   find suitable debug information using any approach, then any warnings
   will be printed.  */

static std::string
find_separate_debug_file (const char *dir,
			  const char *canon_dir,
			  const char *debuglink,
			  unsigned long crc32, struct objfile *objfile,
			  deferred_warnings *warnings)
{
  if (separate_debug_file_debug)
    gdb_printf (gdb_stdlog,
		_("\nLooking for separate debug info (debug link) for "
		  "%s\n"), objfile_name (objfile));

  /* First try in the same directory as the original file.  */
  std::string debugfile = dir;
  debugfile += debuglink;

  if (separate_debug_file_exists (debugfile, crc32, objfile, warnings))
    return debugfile;

  /* Then try in the subdirectory named DEBUG_SUBDIRECTORY.  */
  debugfile = dir;
  debugfile += DEBUG_SUBDIRECTORY;
  debugfile += "/";
  debugfile += debuglink;

  if (separate_debug_file_exists (debugfile, crc32, objfile, warnings))
    return debugfile;

  /* Then try in the global debugfile directories.

     Keep backward compatibility so that DEBUG_FILE_DIRECTORY being "" will
     cause "/..." lookups.  */

  bool target_prefix = is_target_filename (dir);
  const char *dir_notarget
    = target_prefix ? dir + strlen (TARGET_SYSROOT_PREFIX) : dir;
  std::vector<gdb::unique_xmalloc_ptr<char>> debugdir_vec
    = dirnames_to_char_ptr_vec (debug_file_directory.c_str ());
  gdb::unique_xmalloc_ptr<char> canon_sysroot
    = gdb_realpath (gdb_sysroot.c_str ());

 /* MS-Windows/MS-DOS don't allow colons in file names; we must
    convert the drive letter into a one-letter directory, so that the
    file name resulting from splicing below will be valid.

    FIXME: The below only works when GDB runs on MS-Windows/MS-DOS.
    There are various remote-debugging scenarios where such a
    transformation of the drive letter might be required when GDB runs
    on a Posix host, see

    https://sourceware.org/ml/gdb-patches/2019-04/msg00605.html

    If some of those scenarios need to be supported, we will need to
    use a different condition for HAS_DRIVE_SPEC and a different macro
    instead of STRIP_DRIVE_SPEC, which work on Posix systems as well.  */
  std::string drive;
  if (HAS_DRIVE_SPEC (dir_notarget))
    {
      drive = dir_notarget[0];
      dir_notarget = STRIP_DRIVE_SPEC (dir_notarget);
    }

  for (const gdb::unique_xmalloc_ptr<char> &debugdir : debugdir_vec)
    {
      debugfile = target_prefix ? TARGET_SYSROOT_PREFIX : "";
      debugfile += debugdir;
      debugfile += "/";
      debugfile += drive;
      debugfile += dir_notarget;
      debugfile += debuglink;

      if (separate_debug_file_exists (debugfile, crc32, objfile, warnings))
	return debugfile;

      const char *base_path = NULL;
      if (canon_dir != NULL)
	{
	  if (canon_sysroot.get () != NULL)
	    base_path = child_path (canon_sysroot.get (), canon_dir);
	  else
	    base_path = child_path (gdb_sysroot.c_str (), canon_dir);
	}
      if (base_path != NULL)
	{
	  /* If the file is in the sysroot, try using its base path in
	     the global debugfile directory.  */
	  debugfile = target_prefix ? TARGET_SYSROOT_PREFIX : "";
	  debugfile += debugdir;
	  debugfile += "/";
	  debugfile += base_path;
	  debugfile += "/";
	  debugfile += debuglink;

	  if (separate_debug_file_exists (debugfile, crc32, objfile, warnings))
	    return debugfile;

	  /* If the file is in the sysroot, try using its base path in
	     the sysroot's global debugfile directory.  GDB_SYSROOT
	     might refer to a target: path; we strip the "target:"
	     prefix -- but if that would yield the empty string, we
	     don't bother at all, because that would just give the
	     same result as above.  */
	  if (gdb_sysroot != TARGET_SYSROOT_PREFIX)
	    {
	      debugfile = target_prefix ? TARGET_SYSROOT_PREFIX : "";
	      if (is_target_filename (gdb_sysroot))
		{
		  std::string root
		    = gdb_sysroot.substr (strlen (TARGET_SYSROOT_PREFIX));
		  gdb_assert (!root.empty ());
		  debugfile += root;
		}
	      else
		debugfile += gdb_sysroot;
	      debugfile += debugdir;
	      debugfile += "/";
	      debugfile += base_path;
	      debugfile += "/";
	      debugfile += debuglink;

	      if (separate_debug_file_exists (debugfile, crc32, objfile,
					      warnings))
		return debugfile;
	    }
	}
    }

  return std::string ();
}

/* Modify PATH to contain only "[/]directory/" part of PATH.
   If there were no directory separators in PATH, PATH will be empty
   string on return.  */

static void
terminate_after_last_dir_separator (char *path)
{
  int i;

  /* Strip off the final filename part, leaving the directory name,
     followed by a slash.  The directory can be relative or absolute.  */
  for (i = strlen(path) - 1; i >= 0; i--)
    if (IS_DIR_SEPARATOR (path[i]))
      break;

  /* If I is -1 then no directory is present there and DIR will be "".  */
  path[i + 1] = '\0';
}

/* See symtab.h.  */

std::string
find_separate_debug_file_by_debuglink
  (struct objfile *objfile, deferred_warnings *warnings)
{
  uint32_t crc32;

  gdb::unique_xmalloc_ptr<char> debuglink
    (bfd_get_debug_link_info (objfile->obfd.get (), &crc32));

  if (debuglink == NULL)
    {
      /* There's no separate debug info, hence there's no way we could
	 load it => no warning.  */
      return std::string ();
    }

  std::string dir = objfile_name (objfile);
  terminate_after_last_dir_separator (&dir[0]);
  gdb::unique_xmalloc_ptr<char> canon_dir (lrealpath (dir.c_str ()));

  std::string debugfile
    = find_separate_debug_file (dir.c_str (), canon_dir.get (),
				debuglink.get (), crc32, objfile,
				warnings);

  if (debugfile.empty ())
    {
      /* For PR gdb/9538, try again with realpath (if different from the
	 original).  */

      struct stat st_buf;

      if (lstat (objfile_name (objfile), &st_buf) == 0
	  && S_ISLNK (st_buf.st_mode))
	{
	  gdb::unique_xmalloc_ptr<char> symlink_dir
	    (lrealpath (objfile_name (objfile)));
	  if (symlink_dir != NULL)
	    {
	      terminate_after_last_dir_separator (symlink_dir.get ());
	      if (dir != symlink_dir.get ())
		{
		  /* Different directory, so try using it.  */
		  debugfile = find_separate_debug_file (symlink_dir.get (),
							symlink_dir.get (),
							debuglink.get (),
							crc32,
							objfile,
							warnings);
		}
	    }
	}
    }

  return debugfile;
}

/* Make sure that OBJF_{READNOW,READNEVER} are not set
   simultaneously.  */

static void
validate_readnow_readnever (objfile_flags flags)
{
  if ((flags & OBJF_READNOW) && (flags & OBJF_READNEVER))
    error (_("-readnow and -readnever cannot be used simultaneously"));
}

/* This is the symbol-file command.  Read the file, analyze its
   symbols, and add a struct symtab to a symtab list.  The syntax of
   the command is rather bizarre:

   1. The function buildargv implements various quoting conventions
   which are undocumented and have little or nothing in common with
   the way things are quoted (or not quoted) elsewhere in GDB.

   2. Options are used, which are not generally used in GDB (perhaps
   "set mapped on", "set readnow on" would be better)

   3. The order of options matters, which is contrary to GNU
   conventions (because it is confusing and inconvenient).  */

void
symbol_file_command (const char *args, int from_tty)
{
  dont_repeat ();

  if (args == NULL)
    {
      symbol_file_clear (from_tty);
    }
  else
    {
      objfile_flags flags = OBJF_USERLOADED;
      symfile_add_flags add_flags = 0;
      char *name = NULL;
      bool stop_processing_options = false;
      CORE_ADDR offset = 0;
      int idx;
      char *arg;

      if (from_tty)
	add_flags |= SYMFILE_VERBOSE;

      gdb_argv built_argv (args);
      for (arg = built_argv[0], idx = 0; arg != NULL; arg = built_argv[++idx])
	{
	  if (stop_processing_options || *arg != '-')
	    {
	      if (name == NULL)
		name = arg;
	      else
		error (_("Unrecognized argument \"%s\""), arg);
	    }
	  else if (strcmp (arg, "-readnow") == 0)
	    flags |= OBJF_READNOW;
	  else if (strcmp (arg, "-readnever") == 0)
	    flags |= OBJF_READNEVER;
	  else if (strcmp (arg, "-o") == 0)
	    {
	      arg = built_argv[++idx];
	      if (arg == NULL)
		error (_("Missing argument to -o"));

	      offset = parse_and_eval_address (arg);
	    }
	  else if (strcmp (arg, "--") == 0)
	    stop_processing_options = true;
	  else
	    error (_("Unrecognized argument \"%s\""), arg);
	}

      if (name == NULL)
	error (_("no symbol file name was specified"));

      validate_readnow_readnever (flags);

      /* Set SYMFILE_DEFER_BP_RESET because the proper displacement for a PIE
	 (Position Independent Executable) main symbol file will only be
	 computed by the solib_create_inferior_hook below.  Without it,
	 breakpoint_re_set would fail to insert the breakpoints with the zero
	 displacement.  */
      add_flags |= SYMFILE_DEFER_BP_RESET;

      symbol_file_add_main_1 (name, add_flags, flags, offset);

      solib_create_inferior_hook (from_tty);

      /* Now it's safe to re-add the breakpoints.  */
      breakpoint_re_set ();

      /* Also, it's safe to re-add varobjs.  */
      varobj_re_set ();
    }
}

/* Lazily set the initial language.  */

static void
set_initial_language_callback ()
{
  enum language lang = main_language ();
  /* Make C the default language.  */
  enum language default_lang = language_c;

  if (lang == language_unknown)
    {
      const char *name = main_name ();
      struct symbol *sym
	= lookup_symbol_in_language (name, NULL, VAR_DOMAIN, default_lang,
				     NULL).symbol;

      if (sym != NULL)
	lang = sym->language ();
    }

  if (lang == language_unknown)
    {
      lang = default_lang;
    }

  set_language (lang);
  expected_language = current_language; /* Don't warn the user.  */
}

/* Set the initial language.  */

void
set_initial_language (void)
{
  if (language_mode == language_mode_manual)
    return;
  lazily_set_language (set_initial_language_callback);
}

/* Open the file specified by NAME and hand it off to BFD for
   preliminary analysis.  Return a newly initialized bfd *, which
   includes a newly malloc'd` copy of NAME (tilde-expanded and made
   absolute).  In case of trouble, error() is called.  */

gdb_bfd_ref_ptr
symfile_bfd_open (const char *name)
{
  int desc = -1;

  gdb::unique_xmalloc_ptr<char> absolute_name;
  if (!is_target_filename (name))
    {
      gdb::unique_xmalloc_ptr<char> expanded_name (tilde_expand (name));

      /* Look down path for it, allocate 2nd new malloc'd copy.  */
      desc = openp (getenv ("PATH"),
		    OPF_TRY_CWD_FIRST | OPF_RETURN_REALPATH,
		    expanded_name.get (), O_RDONLY | O_BINARY, &absolute_name);
#if defined(__GO32__) || defined(_WIN32) || defined (__CYGWIN__)
      if (desc < 0)
	{
	  char *exename = (char *) alloca (strlen (expanded_name.get ()) + 5);

	  strcat (strcpy (exename, expanded_name.get ()), ".exe");
	  desc = openp (getenv ("PATH"),
			OPF_TRY_CWD_FIRST | OPF_RETURN_REALPATH,
			exename, O_RDONLY | O_BINARY, &absolute_name);
	}
#endif
      if (desc < 0)
	perror_with_name (expanded_name.get ());

      name = absolute_name.get ();
    }

  gdb_bfd_ref_ptr sym_bfd (gdb_bfd_open (name, gnutarget, desc));
  if (sym_bfd == NULL)
    error (_("`%s': can't open to read symbols: %s."), name,
	   bfd_errmsg (bfd_get_error ()));

  if (!bfd_check_format (sym_bfd.get (), bfd_object))
    error (_("`%s': can't read symbols: %s."), name,
	   bfd_errmsg (bfd_get_error ()));

  return sym_bfd;
}

/* See symfile.h.  */

gdb_bfd_ref_ptr
symfile_bfd_open_no_error (const char *name) noexcept
{
  try
    {
      return symfile_bfd_open (name);
    }
  catch (const gdb_exception_error &err)
    {
      warning ("%s", err.what ());
    }

  return nullptr;
}

/* Return the section index for SECTION_NAME on OBJFILE.  Return -1 if
   the section was not found.  */

int
get_section_index (struct objfile *objfile, const char *section_name)
{
  asection *sect = bfd_get_section_by_name (objfile->obfd.get (), section_name);

  if (sect)
    return sect->index;
  else
    return -1;
}

/* Link SF into the global symtab_fns list.
   FLAVOUR is the file format that SF handles.
   Called on startup by the _initialize routine in each object file format
   reader, to register information about each format the reader is prepared
   to handle.  */

void
add_symtab_fns (enum bfd_flavour flavour, const struct sym_fns *sf)
{
  symtab_fns.emplace_back (flavour, sf);
}

/* Initialize OBJFILE to read symbols from its associated BFD.  It
   either returns or calls error().  The result is an initialized
   struct sym_fns in the objfile structure, that contains cached
   information about the symbol file.  */

static const struct sym_fns *
find_sym_fns (bfd *abfd)
{
  enum bfd_flavour our_flavour = bfd_get_flavour (abfd);

  if (our_flavour == bfd_target_srec_flavour
      || our_flavour == bfd_target_ihex_flavour
      || our_flavour == bfd_target_tekhex_flavour)
    return NULL;	/* No symbols.  */

  for (const registered_sym_fns &rsf : symtab_fns)
    if (our_flavour == rsf.sym_flavour)
      return rsf.sym_fns;

  error (_("I'm sorry, Dave, I can't do that.  Symbol format `%s' unknown."),
	 bfd_get_target (abfd));
}


/* This function runs the load command of our current target.  */

static void
load_command (const char *arg, int from_tty)
{
  dont_repeat ();

  /* The user might be reloading because the binary has changed.  Take
     this opportunity to check.  */
  reopen_exec_file ();
  reread_symbols (from_tty);

  std::string temp;
  if (arg == NULL)
    {
      const char *parg, *prev;

      arg = get_exec_file (1);

      /* We may need to quote this string so buildargv can pull it
	 apart.  */
      prev = parg = arg;
      while ((parg = strpbrk (parg, "\\\"'\t ")))
	{
	  temp.append (prev, parg - prev);
	  prev = parg++;
	  temp.push_back ('\\');
	}
      /* If we have not copied anything yet, then we didn't see a
	 character to quote, and we can just leave ARG unchanged.  */
      if (!temp.empty ())
	{
	  temp.append (prev);
	  arg = temp.c_str ();
	}
    }

  target_load (arg, from_tty);

  /* After re-loading the executable, we don't really know which
     overlays are mapped any more.  */
  overlay_cache_invalid = 1;
}

/* This version of "load" should be usable for any target.  Currently
   it is just used for remote targets, not inftarg.c or core files,
   on the theory that only in that case is it useful.

   Avoiding xmodem and the like seems like a win (a) because we don't have
   to worry about finding it, and (b) On VMS, fork() is very slow and so
   we don't want to run a subprocess.  On the other hand, I'm not sure how
   performance compares.  */

static int validate_download = 0;

/* Opaque data for load_progress.  */
struct load_progress_data
{
  /* Cumulative data.  */
  unsigned long write_count = 0;
  unsigned long data_count = 0;
  bfd_size_type total_size = 0;
};

/* Opaque data for load_progress for a single section.  */
struct load_progress_section_data
{
  load_progress_section_data (load_progress_data *cumulative_,
			      const char *section_name_, ULONGEST section_size_,
			      CORE_ADDR lma_, gdb_byte *buffer_)
    : cumulative (cumulative_), section_name (section_name_),
      section_size (section_size_), lma (lma_), buffer (buffer_)
  {}

  struct load_progress_data *cumulative;

  /* Per-section data.  */
  const char *section_name;
  ULONGEST section_sent = 0;
  ULONGEST section_size;
  CORE_ADDR lma;
  gdb_byte *buffer;
};

/* Opaque data for load_section_callback.  */
struct load_section_data
{
  load_section_data (load_progress_data *progress_data_)
    : progress_data (progress_data_)
  {}

  ~load_section_data ()
  {
    for (auto &&request : requests)
      {
	xfree (request.data);
	delete ((load_progress_section_data *) request.baton);
      }
  }

  CORE_ADDR load_offset = 0;
  struct load_progress_data *progress_data;
  std::vector<struct memory_write_request> requests;
};

/* Target write callback routine for progress reporting.  */

static void
load_progress (ULONGEST bytes, void *untyped_arg)
{
  struct load_progress_section_data *args
    = (struct load_progress_section_data *) untyped_arg;
  struct load_progress_data *totals;

  if (args == NULL)
    /* Writing padding data.  No easy way to get at the cumulative
       stats, so just ignore this.  */
    return;

  totals = args->cumulative;

  if (bytes == 0 && args->section_sent == 0)
    {
      /* The write is just starting.  Let the user know we've started
	 this section.  */
      current_uiout->message ("Loading section %s, size %s lma %s\n",
			      args->section_name,
			      hex_string (args->section_size),
			      paddress (current_inferior ()->arch (),
					args->lma));
      return;
    }

  if (validate_download)
    {
      /* Broken memories and broken monitors manifest themselves here
	 when bring new computers to life.  This doubles already slow
	 downloads.  */
      /* NOTE: cagney/1999-10-18: A more efficient implementation
	 might add a verify_memory() method to the target vector and
	 then use that.  remote.c could implement that method using
	 the ``qCRC'' packet.  */
      gdb::byte_vector check (bytes);

      if (target_read_memory (args->lma, check.data (), bytes) != 0)
	error (_("Download verify read failed at %s"),
	       paddress (current_inferior ()->arch (), args->lma));
      if (memcmp (args->buffer, check.data (), bytes) != 0)
	error (_("Download verify compare failed at %s"),
	       paddress (current_inferior ()->arch (), args->lma));
    }
  totals->data_count += bytes;
  args->lma += bytes;
  args->buffer += bytes;
  totals->write_count += 1;
  args->section_sent += bytes;
  if (check_quit_flag ()
      || (deprecated_ui_load_progress_hook != NULL
	  && deprecated_ui_load_progress_hook (args->section_name,
					       args->section_sent)))
    error (_("Canceled the download"));

  if (deprecated_show_load_progress != NULL)
    deprecated_show_load_progress (args->section_name,
				   args->section_sent,
				   args->section_size,
				   totals->data_count,
				   totals->total_size);
}

/* Service function for generic_load.  */

static void
load_one_section (bfd *abfd, asection *asec,
		  struct load_section_data *args)
{
  bfd_size_type size = bfd_section_size (asec);
  const char *sect_name = bfd_section_name (asec);

  if ((bfd_section_flags (asec) & SEC_LOAD) == 0)
    return;

  if (size == 0)
    return;

  ULONGEST begin = bfd_section_lma (asec) + args->load_offset;
  ULONGEST end = begin + size;
  gdb_byte *buffer = (gdb_byte *) xmalloc (size);
  bfd_get_section_contents (abfd, asec, buffer, 0, size);

  load_progress_section_data *section_data
    = new load_progress_section_data (args->progress_data, sect_name, size,
				      begin, buffer);

  args->requests.emplace_back (begin, end, buffer, section_data);
}

static void print_transfer_performance (struct ui_file *stream,
					unsigned long data_count,
					unsigned long write_count,
					std::chrono::steady_clock::duration d);

/* See symfile.h.  */

void
generic_load (const char *args, int from_tty)
{
  struct load_progress_data total_progress;
  struct load_section_data cbdata (&total_progress);
  struct ui_out *uiout = current_uiout;

  if (args == NULL)
    error_no_arg (_("file to load"));

  gdb_argv argv (args);

  gdb::unique_xmalloc_ptr<char> filename (tilde_expand (argv[0]));

  if (argv[1] != NULL)
    {
      const char *endptr;

      cbdata.load_offset = strtoulst (argv[1], &endptr, 0);

      /* If the last word was not a valid number then
	 treat it as a file name with spaces in.  */
      if (argv[1] == endptr)
	error (_("Invalid download offset:%s."), argv[1]);

      if (argv[2] != NULL)
	error (_("Too many parameters."));
    }

  /* Open the file for loading.  */
  gdb_bfd_ref_ptr loadfile_bfd (gdb_bfd_open (filename.get (), gnutarget));
  if (loadfile_bfd == NULL)
    perror_with_name (filename.get ());

  if (!bfd_check_format (loadfile_bfd.get (), bfd_object))
    {
      error (_("\"%s\" is not an object file: %s"), filename.get (),
	     bfd_errmsg (bfd_get_error ()));
    }

  for (asection *asec : gdb_bfd_sections (loadfile_bfd))
    total_progress.total_size += bfd_section_size (asec);

  for (asection *asec : gdb_bfd_sections (loadfile_bfd))
    load_one_section (loadfile_bfd.get (), asec, &cbdata);

  using namespace std::chrono;

  steady_clock::time_point start_time = steady_clock::now ();

  if (target_write_memory_blocks (cbdata.requests, flash_discard,
				  load_progress) != 0)
    error (_("Load failed"));

  steady_clock::time_point end_time = steady_clock::now ();

  CORE_ADDR entry = bfd_get_start_address (loadfile_bfd.get ());
  entry = gdbarch_addr_bits_remove (current_inferior ()->arch (), entry);
  uiout->text ("Start address ");
  uiout->field_core_addr ("address", current_inferior ()->arch (), entry);
  uiout->text (", load size ");
  uiout->field_unsigned ("load-size", total_progress.data_count);
  uiout->text ("\n");
  regcache_write_pc (get_thread_regcache (inferior_thread ()), entry);

  /* Reset breakpoints, now that we have changed the load image.  For
     instance, breakpoints may have been set (or reset, by
     post_create_inferior) while connected to the target but before we
     loaded the program.  In that case, the prologue analyzer could
     have read instructions from the target to find the right
     breakpoint locations.  Loading has changed the contents of that
     memory.  */

  breakpoint_re_set ();

  print_transfer_performance (gdb_stdout, total_progress.data_count,
			      total_progress.write_count,
			      end_time - start_time);
}

/* Report on STREAM the performance of a memory transfer operation,
   such as 'load'.  DATA_COUNT is the number of bytes transferred.
   WRITE_COUNT is the number of separate write operations, or 0, if
   that information is not available.  TIME is how long the operation
   lasted.  */

static void
print_transfer_performance (struct ui_file *stream,
			    unsigned long data_count,
			    unsigned long write_count,
			    std::chrono::steady_clock::duration time)
{
  using namespace std::chrono;
  struct ui_out *uiout = current_uiout;

  milliseconds ms = duration_cast<milliseconds> (time);

  uiout->text ("Transfer rate: ");
  if (ms.count () > 0)
    {
      unsigned long rate = ((ULONGEST) data_count * 1000) / ms.count ();

      if (uiout->is_mi_like_p ())
	{
	  uiout->field_unsigned ("transfer-rate", rate * 8);
	  uiout->text (" bits/sec");
	}
      else if (rate < 1024)
	{
	  uiout->field_unsigned ("transfer-rate", rate);
	  uiout->text (" bytes/sec");
	}
      else
	{
	  uiout->field_unsigned ("transfer-rate", rate / 1024);
	  uiout->text (" KB/sec");
	}
    }
  else
    {
      uiout->field_unsigned ("transferred-bits", (data_count * 8));
      uiout->text (" bits in <1 sec");
    }
  if (write_count > 0)
    {
      uiout->text (", ");
      uiout->field_unsigned ("write-rate", data_count / write_count);
      uiout->text (" bytes/write");
    }
  uiout->text (".\n");
}

/* Add an OFFSET to the start address of each section in OBJF, except
   sections that were specified in ADDRS.  */

static void
set_objfile_default_section_offset (struct objfile *objf,
				    const section_addr_info &addrs,
				    CORE_ADDR offset)
{
  /* Add OFFSET to all sections by default.  */
  section_offsets offsets (objf->section_offsets.size (), offset);

  /* Create sorted lists of all sections in ADDRS as well as all
     sections in OBJF.  */

  std::vector<const struct other_sections *> addrs_sorted
    = addrs_section_sort (addrs);

  section_addr_info objf_addrs
    = build_section_addr_info_from_objfile (objf);
  std::vector<const struct other_sections *> objf_addrs_sorted
    = addrs_section_sort (objf_addrs);

  /* Walk the BFD section list, and if a matching section is found in
     ADDRS_SORTED_LIST, set its offset to zero to keep its address
     unchanged.

     Note that both lists may contain multiple sections with the same
     name, and then the sections from ADDRS are matched in BFD order
     (thanks to sectindex).  */

  std::vector<const struct other_sections *>::iterator addrs_sorted_iter
    = addrs_sorted.begin ();
  for (const other_sections *objf_sect : objf_addrs_sorted)
    {
      const char *objf_name = addr_section_name (objf_sect->name.c_str ());
      int cmp = -1;

      while (cmp < 0 && addrs_sorted_iter != addrs_sorted.end ())
	{
	  const struct other_sections *sect = *addrs_sorted_iter;
	  const char *sect_name = addr_section_name (sect->name.c_str ());
	  cmp = strcmp (sect_name, objf_name);
	  if (cmp <= 0)
	    ++addrs_sorted_iter;
	}

      if (cmp == 0)
	offsets[objf_sect->sectindex] = 0;
    }

  /* Apply the new section offsets.  */
  objfile_relocate (objf, offsets);
}

/* This function allows the addition of incrementally linked object files.
   It does not modify any state in the target, only in the debugger.  */

static void
add_symbol_file_command (const char *args, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();
  gdb::unique_xmalloc_ptr<char> filename;
  char *arg;
  int argcnt = 0;
  struct objfile *objf;
  objfile_flags flags = OBJF_USERLOADED | OBJF_SHARED;
  symfile_add_flags add_flags = 0;

  if (from_tty)
    add_flags |= SYMFILE_VERBOSE;

  struct sect_opt
  {
    const char *name;
    const char *value;
  };

  std::vector<sect_opt> sect_opts = { { ".text", NULL } };
  bool stop_processing_options = false;
  CORE_ADDR offset = 0;

  dont_repeat ();

  if (args == NULL)
    error (_("add-symbol-file takes a file name and an address"));

  bool seen_addr = false;
  bool seen_offset = false;
  gdb_argv argv (args);

  for (arg = argv[0], argcnt = 0; arg != NULL; arg = argv[++argcnt])
    {
      if (stop_processing_options || *arg != '-')
	{
	  if (filename == NULL)
	    {
	      /* First non-option argument is always the filename.  */
	      filename.reset (tilde_expand (arg));
	    }
	  else if (!seen_addr)
	    {
	      /* The second non-option argument is always the text
		 address at which to load the program.  */
	      sect_opts[0].value = arg;
	      seen_addr = true;
	    }
	  else
	    error (_("Unrecognized argument \"%s\""), arg);
	}
      else if (strcmp (arg, "-readnow") == 0)
	flags |= OBJF_READNOW;
      else if (strcmp (arg, "-readnever") == 0)
	flags |= OBJF_READNEVER;
      else if (strcmp (arg, "-s") == 0)
	{
	  if (argv[argcnt + 1] == NULL)
	    error (_("Missing section name after \"-s\""));
	  else if (argv[argcnt + 2] == NULL)
	    error (_("Missing section address after \"-s\""));

	  sect_opt sect = { argv[argcnt + 1], argv[argcnt + 2] };

	  sect_opts.push_back (sect);
	  argcnt += 2;
	}
      else if (strcmp (arg, "-o") == 0)
	{
	  arg = argv[++argcnt];
	  if (arg == NULL)
	    error (_("Missing argument to -o"));

	  offset = parse_and_eval_address (arg);
	  seen_offset = true;
	}
      else if (strcmp (arg, "--") == 0)
	stop_processing_options = true;
      else
	error (_("Unrecognized argument \"%s\""), arg);
    }

  if (filename == NULL)
    error (_("You must provide a filename to be loaded."));

  validate_readnow_readnever (flags);

  /* Print the prompt for the query below.  And save the arguments into
     a sect_addr_info structure to be passed around to other
     functions.  We have to split this up into separate print
     statements because hex_string returns a local static
     string.  */

  gdb_printf (_("add symbol table from file \"%ps\""),
	      styled_string (file_name_style.style (), filename.get ()));
  section_addr_info section_addrs;
  std::vector<sect_opt>::const_iterator it = sect_opts.begin ();
  if (!seen_addr)
    ++it;
  for (; it != sect_opts.end (); ++it)
    {
      CORE_ADDR addr;
      const char *val = it->value;
      const char *sec = it->name;

      if (section_addrs.empty ())
	gdb_printf (_(" at\n"));
      addr = parse_and_eval_address (val);

      /* Here we store the section offsets in the order they were
	 entered on the command line.  Every array element is
	 assigned an ascending section index to preserve the above
	 order over an unstable sorting algorithm.  This dummy
	 index is not used for any other purpose.
      */
      section_addrs.emplace_back (addr, sec, section_addrs.size ());
      gdb_printf ("\t%s_addr = %s\n", sec,
		  paddress (gdbarch, addr));

      /* The object's sections are initialized when a
	 call is made to build_objfile_section_table (objfile).
	 This happens in reread_symbols.
	 At this point, we don't know what file type this is,
	 so we can't determine what section names are valid.  */
    }
  if (seen_offset)
    gdb_printf (_("%s offset by %s\n"),
		(section_addrs.empty ()
		 ? _(" with all sections")
		 : _("with other sections")),
		paddress (gdbarch, offset));
  else if (section_addrs.empty ())
    gdb_printf ("\n");

  if (from_tty && (!query ("%s", "")))
    error (_("Not confirmed."));

  objf = symbol_file_add (filename.get (), add_flags, &section_addrs,
			  flags);
  if (!objfile_has_symbols (objf) && objf->per_bfd->minimal_symbol_count <= 0)
    warning (_("newly-added symbol file \"%ps\" does not provide any symbols"),
	     styled_string (file_name_style.style (), filename.get ()));

  if (seen_offset)
    set_objfile_default_section_offset (objf, section_addrs, offset);

  current_program_space->add_target_sections (objf);

  /* Getting new symbols may change our opinion about what is
     frameless.  */
  reinit_frame_cache ();
}


/* This function removes a symbol file that was added via add-symbol-file.  */

static void
remove_symbol_file_command (const char *args, int from_tty)
{
  struct objfile *objf = NULL;
  struct program_space *pspace = current_program_space;

  dont_repeat ();

  if (args == NULL)
    error (_("remove-symbol-file: no symbol file provided"));

  gdb_argv argv (args);

  if (strcmp (argv[0], "-a") == 0)
    {
      /* Interpret the next argument as an address.  */
      CORE_ADDR addr;

      if (argv[1] == NULL)
	error (_("Missing address argument"));

      if (argv[2] != NULL)
	error (_("Junk after %s"), argv[1]);

      addr = parse_and_eval_address (argv[1]);

      for (objfile *objfile : current_program_space->objfiles ())
	{
	  if ((objfile->flags & OBJF_USERLOADED) != 0
	      && (objfile->flags & OBJF_SHARED) != 0
	      && objfile->pspace == pspace
	      && is_addr_in_objfile (addr, objfile))
	    {
	      objf = objfile;
	      break;
	    }
	}
    }
  else if (argv[0] != NULL)
    {
      /* Interpret the current argument as a file name.  */

      if (argv[1] != NULL)
	error (_("Junk after %s"), argv[0]);

      gdb::unique_xmalloc_ptr<char> filename (tilde_expand (argv[0]));

      for (objfile *objfile : current_program_space->objfiles ())
	{
	  if ((objfile->flags & OBJF_USERLOADED) != 0
	      && (objfile->flags & OBJF_SHARED) != 0
	      && objfile->pspace == pspace
	      && filename_cmp (filename.get (), objfile_name (objfile)) == 0)
	    {
	      objf = objfile;
	      break;
	    }
	}
    }

  if (objf == NULL)
    error (_("No symbol file found"));

  if (from_tty
      && !query (_("Remove symbol table from file \"%s\"? "),
		 objfile_name (objf)))
    error (_("Not confirmed."));

  objf->unlink ();
  clear_symtab_users (0);
}

/* Re-read symbols if a symbol-file has changed.  */

void
reread_symbols (int from_tty)
{
  std::vector<struct objfile *> new_objfiles;

  /* Check to see if the executable has changed, and if so reopen it.  The
     executable might not be in the list of objfiles (if the user set
     different values for 'exec-file' and 'symbol-file'), and even if it
     is, then we use a separate timestamp (within the program_space) to
     indicate when the executable was last reloaded.  */
  reopen_exec_file ();

  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (objfile->obfd.get () == NULL)
	continue;

      /* Separate debug objfiles are handled in the main objfile.  */
      if (objfile->separate_debug_objfile_backlink)
	continue;

      /* When a in-memory BFD is initially created, it's mtime (as
	 returned by bfd_get_mtime) is the creation time of the BFD.
	 However, we call bfd_stat here as we want to see if the
	 underlying file has changed, and in this case an in-memory BFD
	 will return an st_mtime of zero, so it appears that the in-memory
	 file has changed, which isn't what we want here -- this code is
	 about reloading BFDs that changed on disk.

	 Just skip any in-memory BFD.  */
      if (objfile->obfd.get ()->flags & BFD_IN_MEMORY)
	continue;

      struct stat new_statbuf;
      int res = bfd_stat (objfile->obfd.get (), &new_statbuf);
      if (res != 0)
	{
	  /* If this object is from an archive (what you usually create
	     with `ar', often called a `static library' on most systems,
	     though a `shared library' on AIX is also an archive), then you
	     should stat on the archive name, not member name.  */
	  const char *filename;
	  if (objfile->obfd->my_archive)
	    filename = bfd_get_filename (objfile->obfd->my_archive);
	  else
	    filename = objfile_name (objfile);

	  warning (_("`%ps' has disappeared; keeping its symbols."),
		   styled_string (file_name_style.style (), filename));
	  continue;
	}
      time_t new_modtime = new_statbuf.st_mtime;
      if (new_modtime != objfile->mtime)
	{
	  gdb_printf (_("`%ps' has changed; re-reading symbols.\n"),
		      styled_string (file_name_style.style (),
				     objfile_name (objfile)));

	  /* There are various functions like symbol_file_add,
	     symfile_bfd_open, syms_from_objfile, etc., which might
	     appear to do what we want.  But they have various other
	     effects which we *don't* want.  So we just do stuff
	     ourselves.  We don't worry about mapped files (for one thing,
	     any mapped file will be out of date).  */

	  /* If we get an error, blow away this objfile (not sure if
	     that is the correct response for things like shared
	     libraries).  */
	  objfile_up objfile_holder (objfile);

	  /* We need to do this whenever any symbols go away.  */
	  clear_symtab_users_cleanup defer_clear_users (0);

	  /* Keep the calls order approx. the same as in free_objfile.  */

	  /* Free the separate debug objfiles.  It will be
	     automatically recreated by sym_read.  */
	  free_objfile_separate_debug (objfile);

	  /* Clear the stale source cache.  */
	  forget_cached_source_info ();

	  /* Remove any references to this objfile in the global
	     value lists.  */
	  preserve_values (objfile);

	  /* Nuke all the state that we will re-read.  Much of the following
	     code which sets things to NULL really is necessary to tell
	     other parts of GDB that there is nothing currently there.

	     Try to keep the freeing order compatible with free_objfile.  */

	  if (objfile->sf != NULL)
	    {
	      (*objfile->sf->sym_finish) (objfile);
	    }

	  objfile->registry_fields.clear_registry ();

	  /* Clean up any state BFD has sitting around.  */
	  {
	    gdb_bfd_ref_ptr obfd = objfile->obfd;
	    const char *obfd_filename;

	    obfd_filename = bfd_get_filename (objfile->obfd.get ());
	    /* Open the new BFD before freeing the old one, so that
	       the filename remains live.  */
	    gdb_bfd_ref_ptr temp (gdb_bfd_open (obfd_filename, gnutarget));
	    objfile->obfd = std::move (temp);
	    if (objfile->obfd == NULL)
	      error (_("Can't open %s to read symbols."), obfd_filename);
	  }

	  std::string original_name = objfile->original_name;

	  /* bfd_openr sets cacheable to true, which is what we want.  */
	  if (!bfd_check_format (objfile->obfd.get (), bfd_object))
	    error (_("Can't read symbols from %s: %s."), objfile_name (objfile),
		   bfd_errmsg (bfd_get_error ()));

	  /* NB: after this call to obstack_free, objfiles_changed
	     will need to be called (see discussion below).  */
	  obstack_free (&objfile->objfile_obstack, 0);
	  objfile->sections_start = NULL;
	  objfile->section_offsets.clear ();
	  objfile->sect_index_bss = -1;
	  objfile->sect_index_data = -1;
	  objfile->sect_index_rodata = -1;
	  objfile->sect_index_text = -1;
	  objfile->compunit_symtabs = NULL;
	  objfile->template_symbols = NULL;
	  objfile->static_links.reset (nullptr);

	  /* obstack_init also initializes the obstack so it is
	     empty.  We could use obstack_specify_allocation but
	     gdb_obstack.h specifies the alloc/dealloc functions.  */
	  obstack_init (&objfile->objfile_obstack);

	  /* set_objfile_per_bfd potentially allocates the per-bfd
	     data on the objfile's obstack (if sharing data across
	     multiple users is not possible), so it's important to
	     do it *after* the obstack has been initialized.  */
	  set_objfile_per_bfd (objfile);

	  objfile->original_name
	    = obstack_strdup (&objfile->objfile_obstack, original_name);

	  /* Reset the sym_fns pointer.  The ELF reader can change it
	     based on whether .gdb_index is present, and we need it to
	     start over.  PR symtab/15885  */
	  objfile_set_sym_fns (objfile, find_sym_fns (objfile->obfd.get ()));
	  objfile->qf.clear ();

	  build_objfile_section_table (objfile);

	  /* What the hell is sym_new_init for, anyway?  The concept of
	     distinguishing between the main file and additional files
	     in this way seems rather dubious.  */
	  if (objfile == current_program_space->symfile_object_file)
	    {
	      (*objfile->sf->sym_new_init) (objfile);
	    }

	  (*objfile->sf->sym_init) (objfile);
	  clear_complaints ();

	  /* We are about to read new symbols and potentially also
	     DWARF information.  Some targets may want to pass addresses
	     read from DWARF DIE's through an adjustment function before
	     saving them, like MIPS, which may call into
	     "find_pc_section".  When called, that function will make
	     use of per-objfile program space data.

	     Since we discarded our section information above, we have
	     dangling pointers in the per-objfile program space data
	     structure.  Force GDB to update the section mapping
	     information by letting it know the objfile has changed,
	     making the dangling pointers point to correct data
	     again.  */

	  objfiles_changed ();

	  /* Recompute section offsets and section indices.  */
	  objfile->sf->sym_offsets (objfile, {});

	  read_symbols (objfile, 0);

	  if ((objfile->flags & OBJF_READNOW))
	    {
	      const int mainline = objfile->flags & OBJF_MAINLINE;
	      const int should_print = (print_symbol_loading_p (from_tty, mainline, 1)
					&& readnow_symbol_files);
	      if (should_print)
		gdb_printf (_("Expanding full symbols from %ps...\n"),
			    styled_string (file_name_style.style (),
					   objfile_name (objfile)));

	      objfile->expand_all_symtabs ();
	    }

	  if (!objfile_has_symbols (objfile))
	    {
	      gdb_stdout->wrap_here (0);
	      gdb_printf (_("(no debugging symbols found)\n"));
	      gdb_stdout->wrap_here (0);
	    }

	  /* We're done reading the symbol file; finish off complaints.  */
	  clear_complaints ();

	  /* Getting new symbols may change our opinion about what is
	     frameless.  */

	  reinit_frame_cache ();

	  /* Discard cleanups as symbol reading was successful.  */
	  objfile_holder.release ();
	  defer_clear_users.release ();

	  /* If the mtime has changed between the time we set new_modtime
	     and now, we *want* this to be out of date, so don't call stat
	     again now.  */
	  objfile->mtime = new_modtime;
	  init_entry_point_info (objfile);

	  new_objfiles.push_back (objfile);
	}
    }

  if (!new_objfiles.empty ())
    {
      clear_symtab_users (0);

      /* The registry for each objfile was cleared and
	 gdb::observers::new_objfile.notify (NULL) has been called by
	 clear_symtab_users above.  Notify the new files now.  */
      for (auto iter : new_objfiles)
	gdb::observers::new_objfile.notify (iter);
    }
}


struct filename_language
{
  filename_language (const std::string &ext_, enum language lang_)
  : ext (ext_), lang (lang_)
  {}

  std::string ext;
  enum language lang;
};

static std::vector<filename_language> filename_language_table;

/* See symfile.h.  */

void
add_filename_language (const char *ext, enum language lang)
{
  gdb_assert (ext != nullptr);
  filename_language_table.emplace_back (ext, lang);
}

static std::string ext_args;
static void
show_ext_args (struct ui_file *file, int from_tty,
	       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Mapping between filename extension "
		"and source language is \"%s\".\n"),
	      value);
}

static void
set_ext_lang_command (const char *args,
		      int from_tty, struct cmd_list_element *e)
{
  const char *begin = ext_args.c_str ();
  const char *end = ext_args.c_str ();

  /* First arg is filename extension, starting with '.'  */
  if (*end != '.')
    error (_("'%s': Filename extension must begin with '.'"), ext_args.c_str ());

  /* Find end of first arg.  */
  while (*end != '\0' && !isspace (*end))
    end++;

  if (*end == '\0')
    error (_("'%s': two arguments required -- "
	     "filename extension and language"),
	   ext_args.c_str ());

  /* Extract first arg, the extension.  */
  std::string extension = ext_args.substr (0, end - begin);

  /* Find beginning of second arg, which should be a source language.  */
  begin = skip_spaces (end);

  if (*begin == '\0')
    error (_("'%s': two arguments required -- "
	     "filename extension and language"),
	   ext_args.c_str ());

  /* Lookup the language from among those we know.  */
  language lang = language_enum (begin);

  auto it = filename_language_table.begin ();
  /* Now lookup the filename extension: do we already know it?  */
  for (; it != filename_language_table.end (); it++)
    {
      if (it->ext == extension)
	break;
    }

  if (it == filename_language_table.end ())
    {
      /* New file extension.  */
      add_filename_language (extension.data (), lang);
    }
  else
    {
      /* Redefining a previously known filename extension.  */

      /* if (from_tty) */
      /*   query ("Really make files of type %s '%s'?", */
      /*          ext_args, language_str (lang));           */

      it->lang = lang;
    }
}

static void
info_ext_lang_command (const char *args, int from_tty)
{
  gdb_printf (_("Filename extensions and the languages they represent:"));
  gdb_printf ("\n\n");
  for (const filename_language &entry : filename_language_table)
    gdb_printf ("\t%s\t- %s\n", entry.ext.c_str (),
		language_str (entry.lang));
}

enum language
deduce_language_from_filename (const char *filename)
{
  const char *cp;

  if (filename != NULL)
    if ((cp = strrchr (filename, '.')) != NULL)
      {
	for (const filename_language &entry : filename_language_table)
	  if (entry.ext == cp)
	    return entry.lang;
      }

  return language_unknown;
}

/* Allocate and initialize a new symbol table.
   CUST is from the result of allocate_compunit_symtab.  */

struct symtab *
allocate_symtab (struct compunit_symtab *cust, const char *filename,
		 const char *filename_for_id)
{
  struct objfile *objfile = cust->objfile ();
  struct symtab *symtab
    = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct symtab);

  symtab->filename = objfile->intern (filename);
  symtab->filename_for_id = objfile->intern (filename_for_id);
  symtab->fullname = NULL;
  symtab->set_language (deduce_language_from_filename (filename));

  /* This can be very verbose with lots of headers.
     Only print at higher debug levels.  */
  if (symtab_create_debug >= 2)
    {
      /* Be a bit clever with debugging messages, and don't print objfile
	 every time, only when it changes.  */
      static std::string last_objfile_name;
      const char *this_objfile_name = objfile_name (objfile);

      if (last_objfile_name.empty () || last_objfile_name != this_objfile_name)
	{
	  last_objfile_name = this_objfile_name;

	  symtab_create_debug_printf_v
	    ("creating one or more symtabs for objfile %s", this_objfile_name);
	}

      symtab_create_debug_printf_v ("created symtab %s for module %s",
				    host_address_to_string (symtab), filename);
    }

  /* Add it to CUST's list of symtabs.  */
  cust->add_filetab (symtab);

  /* Backlink to the containing compunit symtab.  */
  symtab->set_compunit (cust);

  return symtab;
}

/* Allocate and initialize a new compunit.
   NAME is the name of the main source file, if there is one, or some
   descriptive text if there are no source files.  */

struct compunit_symtab *
allocate_compunit_symtab (struct objfile *objfile, const char *name)
{
  struct compunit_symtab *cu = OBSTACK_ZALLOC (&objfile->objfile_obstack,
					       struct compunit_symtab);
  const char *saved_name;

  cu->set_objfile (objfile);

  /* The name we record here is only for display/debugging purposes.
     Just save the basename to avoid path issues (too long for display,
     relative vs absolute, etc.).  */
  saved_name = lbasename (name);
  cu->name = obstack_strdup (&objfile->objfile_obstack, saved_name);

  cu->set_debugformat ("unknown");

  symtab_create_debug_printf_v ("created compunit symtab %s for %s",
				host_address_to_string (cu),
				cu->name);

  return cu;
}

/* Hook CU to the objfile it comes from.  */

void
add_compunit_symtab_to_objfile (struct compunit_symtab *cu)
{
  cu->next = cu->objfile ()->compunit_symtabs;
  cu->objfile ()->compunit_symtabs = cu;
}


/* Reset all data structures in gdb which may contain references to
   symbol table data.  */

void
clear_symtab_users (symfile_add_flags add_flags)
{
  /* Someday, we should do better than this, by only blowing away
     the things that really need to be blown.  */

  /* Clear the "current" symtab first, because it is no longer valid.
     breakpoint_re_set may try to access the current symtab.  */
  clear_current_source_symtab_and_line ();

  clear_displays ();
  clear_last_displayed_sal ();
  clear_pc_function_cache ();
  gdb::observers::all_objfiles_removed.notify (current_program_space);

  /* Now that the various caches have been cleared, we can re_set
     our breakpoints without risking it using stale data.  */
  if ((add_flags & SYMFILE_DEFER_BP_RESET) == 0)
    breakpoint_re_set ();
}

/* OVERLAYS:
   The following code implements an abstraction for debugging overlay sections.

   The target model is as follows:
   1) The gnu linker will permit multiple sections to be mapped into the
   same VMA, each with its own unique LMA (or load address).
   2) It is assumed that some runtime mechanism exists for mapping the
   sections, one by one, from the load address into the VMA address.
   3) This code provides a mechanism for gdb to keep track of which
   sections should be considered to be mapped from the VMA to the LMA.
   This information is used for symbol lookup, and memory read/write.
   For instance, if a section has been mapped then its contents
   should be read from the VMA, otherwise from the LMA.

   Two levels of debugger support for overlays are available.  One is
   "manual", in which the debugger relies on the user to tell it which
   overlays are currently mapped.  This level of support is
   implemented entirely in the core debugger, and the information about
   whether a section is mapped is kept in the objfile->obj_section table.

   The second level of support is "automatic", and is only available if
   the target-specific code provides functionality to read the target's
   overlay mapping table, and translate its contents for the debugger
   (by updating the mapped state information in the obj_section tables).

   The interface is as follows:
   User commands:
   overlay map <name>   -- tell gdb to consider this section mapped
   overlay unmap <name> -- tell gdb to consider this section unmapped
   overlay list         -- list the sections that GDB thinks are mapped
   overlay read-target  -- get the target's state of what's mapped
   overlay off/manual/auto -- set overlay debugging state
   Functional interface:
   find_pc_mapped_section(pc):    if the pc is in the range of a mapped
   section, return that section.
   find_pc_overlay(pc):       find any overlay section that contains
   the pc, either in its VMA or its LMA
   section_is_mapped(sect):       true if overlay is marked as mapped
   section_is_overlay(sect):      true if section's VMA != LMA
   pc_in_mapped_range(pc,sec):    true if pc belongs to section's VMA
   pc_in_unmapped_range(...):     true if pc belongs to section's LMA
   sections_overlap(sec1, sec2):  true if mapped sec1 and sec2 ranges overlap
   overlay_mapped_address(...):   map an address from section's LMA to VMA
   overlay_unmapped_address(...): map an address from section's VMA to LMA
   symbol_overlayed_address(...): Return a "current" address for symbol:
   either in VMA or LMA depending on whether
   the symbol's section is currently mapped.  */

/* Overlay debugging state: */

enum overlay_debugging_state overlay_debugging = ovly_off;
int overlay_cache_invalid = 0;	/* True if need to refresh mapped state.  */

/* Function: section_is_overlay (SECTION)
   Returns true if SECTION has VMA not equal to LMA, ie.
   SECTION is loaded at an address different from where it will "run".  */

int
section_is_overlay (struct obj_section *section)
{
  if (overlay_debugging && section)
    {
      asection *bfd_section = section->the_bfd_section;

      if (bfd_section_lma (bfd_section) != 0
	  && bfd_section_lma (bfd_section) != bfd_section_vma (bfd_section))
	return 1;
    }

  return 0;
}

/* Function: overlay_invalidate_all (void)
   Invalidate the mapped state of all overlay sections (mark it as stale).  */

static void
overlay_invalidate_all (void)
{
  for (objfile *objfile : current_program_space->objfiles ())
    for (obj_section *sect : objfile->sections ())
      if (section_is_overlay (sect))
	sect->ovly_mapped = -1;
}

/* Function: section_is_mapped (SECTION)
   Returns true if section is an overlay, and is currently mapped.

   Access to the ovly_mapped flag is restricted to this function, so
   that we can do automatic update.  If the global flag
   OVERLAY_CACHE_INVALID is set (by wait_for_inferior), then call
   overlay_invalidate_all.  If the mapped state of the particular
   section is stale, then call TARGET_OVERLAY_UPDATE to refresh it.  */

int
section_is_mapped (struct obj_section *osect)
{
  struct gdbarch *gdbarch;

  if (osect == 0 || !section_is_overlay (osect))
    return 0;

  switch (overlay_debugging)
    {
    default:
    case ovly_off:
      return 0;			/* overlay debugging off */
    case ovly_auto:		/* overlay debugging automatic */
      /* Unles there is a gdbarch_overlay_update function,
	 there's really nothing useful to do here (can't really go auto).  */
      gdbarch = osect->objfile->arch ();
      if (gdbarch_overlay_update_p (gdbarch))
	{
	  if (overlay_cache_invalid)
	    {
	      overlay_invalidate_all ();
	      overlay_cache_invalid = 0;
	    }
	  if (osect->ovly_mapped == -1)
	    gdbarch_overlay_update (gdbarch, osect);
	}
      [[fallthrough]];
    case ovly_on:		/* overlay debugging manual */
      return osect->ovly_mapped == 1;
    }
}

/* Function: pc_in_unmapped_range
   If PC falls into the lma range of SECTION, return true, else false.  */

bool
pc_in_unmapped_range (CORE_ADDR pc, struct obj_section *section)
{
  if (section_is_overlay (section))
    {
      asection *bfd_section = section->the_bfd_section;

      /* We assume the LMA is relocated by the same offset as the VMA.  */
      bfd_vma size = bfd_section_size (bfd_section);
      CORE_ADDR offset = section->offset ();

      if (bfd_section_lma (bfd_section) + offset <= pc
	  && pc < bfd_section_lma (bfd_section) + offset + size)
	return true;
    }

  return false;
}

/* Function: pc_in_mapped_range
   If PC falls into the vma range of SECTION, return true, else false.  */

bool
pc_in_mapped_range (CORE_ADDR pc, struct obj_section *section)
{
  if (section_is_overlay (section))
    {
      if (section->addr () <= pc
	  && pc < section->endaddr ())
	return true;
    }

  return false;
}

/* Return true if the mapped ranges of sections A and B overlap, false
   otherwise.  */

static int
sections_overlap (struct obj_section *a, struct obj_section *b)
{
  CORE_ADDR a_start = a->addr ();
  CORE_ADDR a_end = a->endaddr ();
  CORE_ADDR b_start = b->addr ();
  CORE_ADDR b_end = b->endaddr ();

  return (a_start < b_end && b_start < a_end);
}

/* Function: overlay_unmapped_address (PC, SECTION)
   Returns the address corresponding to PC in the unmapped (load) range.
   May be the same as PC.  */

CORE_ADDR
overlay_unmapped_address (CORE_ADDR pc, struct obj_section *section)
{
  if (section_is_overlay (section) && pc_in_mapped_range (pc, section))
    {
      asection *bfd_section = section->the_bfd_section;

      return (pc + bfd_section_lma (bfd_section)
	      - bfd_section_vma (bfd_section));
    }

  return pc;
}

/* Function: overlay_mapped_address (PC, SECTION)
   Returns the address corresponding to PC in the mapped (runtime) range.
   May be the same as PC.  */

CORE_ADDR
overlay_mapped_address (CORE_ADDR pc, struct obj_section *section)
{
  if (section_is_overlay (section) && pc_in_unmapped_range (pc, section))
    {
      asection *bfd_section = section->the_bfd_section;

      return (pc + bfd_section_vma (bfd_section)
	      - bfd_section_lma (bfd_section));
    }

  return pc;
}

/* Function: symbol_overlayed_address
   Return one of two addresses (relative to the VMA or to the LMA),
   depending on whether the section is mapped or not.  */

CORE_ADDR
symbol_overlayed_address (CORE_ADDR address, struct obj_section *section)
{
  if (overlay_debugging)
    {
      /* If the symbol has no section, just return its regular address.  */
      if (section == 0)
	return address;
      /* If the symbol's section is not an overlay, just return its
	 address.  */
      if (!section_is_overlay (section))
	return address;
      /* If the symbol's section is mapped, just return its address.  */
      if (section_is_mapped (section))
	return address;
      /*
       * HOWEVER: if the symbol is in an overlay section which is NOT mapped,
       * then return its LOADED address rather than its vma address!!
       */
      return overlay_unmapped_address (address, section);
    }
  return address;
}

/* Function: find_pc_overlay (PC)
   Return the best-match overlay section for PC:
   If PC matches a mapped overlay section's VMA, return that section.
   Else if PC matches an unmapped section's VMA, return that section.
   Else if PC matches an unmapped section's LMA, return that section.  */

struct obj_section *
find_pc_overlay (CORE_ADDR pc)
{
  struct obj_section *best_match = NULL;

  if (overlay_debugging)
    {
      for (objfile *objfile : current_program_space->objfiles ())
	for (obj_section *osect : objfile->sections ())
	  if (section_is_overlay (osect))
	    {
	      if (pc_in_mapped_range (pc, osect))
		{
		  if (section_is_mapped (osect))
		    return osect;
		  else
		    best_match = osect;
		}
	      else if (pc_in_unmapped_range (pc, osect))
		best_match = osect;
	    }
    }
  return best_match;
}

/* Function: find_pc_mapped_section (PC)
   If PC falls into the VMA address range of an overlay section that is
   currently marked as MAPPED, return that section.  Else return NULL.  */

struct obj_section *
find_pc_mapped_section (CORE_ADDR pc)
{
  if (overlay_debugging)
    {
      for (objfile *objfile : current_program_space->objfiles ())
	for (obj_section *osect : objfile->sections ())
	  if (pc_in_mapped_range (pc, osect) && section_is_mapped (osect))
	    return osect;
    }

  return NULL;
}

/* Function: list_overlays_command
   Print a list of mapped sections and their PC ranges.  */

static void
list_overlays_command (const char *args, int from_tty)
{
  int nmapped = 0;

  if (overlay_debugging)
    {
      for (objfile *objfile : current_program_space->objfiles ())
	for (obj_section *osect : objfile->sections ())
	  if (section_is_mapped (osect))
	    {
	      struct gdbarch *gdbarch = objfile->arch ();
	      const char *name;
	      bfd_vma lma, vma;
	      int size;

	      vma = bfd_section_vma (osect->the_bfd_section);
	      lma = bfd_section_lma (osect->the_bfd_section);
	      size = bfd_section_size (osect->the_bfd_section);
	      name = bfd_section_name (osect->the_bfd_section);

	      gdb_printf ("Section %s, loaded at ", name);
	      gdb_puts (paddress (gdbarch, lma));
	      gdb_puts (" - ");
	      gdb_puts (paddress (gdbarch, lma + size));
	      gdb_printf (", mapped at ");
	      gdb_puts (paddress (gdbarch, vma));
	      gdb_puts (" - ");
	      gdb_puts (paddress (gdbarch, vma + size));
	      gdb_puts ("\n");

	      nmapped++;
	    }
    }
  if (nmapped == 0)
    gdb_printf (_("No sections are mapped.\n"));
}

/* Function: map_overlay_command
   Mark the named section as mapped (ie. residing at its VMA address).  */

static void
map_overlay_command (const char *args, int from_tty)
{
  if (!overlay_debugging)
    error (_("Overlay debugging not enabled.  Use "
	     "either the 'overlay auto' or\n"
	     "the 'overlay manual' command."));

  if (args == 0 || *args == 0)
    error (_("Argument required: name of an overlay section"));

  /* First, find a section matching the user supplied argument.  */
  for (objfile *obj_file : current_program_space->objfiles ())
    for (obj_section *sec : obj_file->sections ())
      if (!strcmp (bfd_section_name (sec->the_bfd_section), args))
	{
	  /* Now, check to see if the section is an overlay.  */
	  if (!section_is_overlay (sec))
	    continue;		/* not an overlay section */

	  /* Mark the overlay as "mapped".  */
	  sec->ovly_mapped = 1;

	  /* Next, make a pass and unmap any sections that are
	     overlapped by this new section: */
	  for (objfile *objfile2 : current_program_space->objfiles ())
	    for (obj_section *sec2 : objfile2->sections ())
	      if (sec2->ovly_mapped && sec != sec2 && sections_overlap (sec,
									sec2))
		{
		  if (info_verbose)
		    gdb_printf (_("Note: section %s unmapped by overlap\n"),
				bfd_section_name (sec2->the_bfd_section));
		  sec2->ovly_mapped = 0; /* sec2 overlaps sec: unmap sec2.  */
		}
	  return;
	}
  error (_("No overlay section called %s"), args);
}

/* Function: unmap_overlay_command
   Mark the overlay section as unmapped
   (ie. resident in its LMA address range, rather than the VMA range).  */

static void
unmap_overlay_command (const char *args, int from_tty)
{
  if (!overlay_debugging)
    error (_("Overlay debugging not enabled.  "
	     "Use either the 'overlay auto' or\n"
	     "the 'overlay manual' command."));

  if (args == 0 || *args == 0)
    error (_("Argument required: name of an overlay section"));

  /* First, find a section matching the user supplied argument.  */
  for (objfile *objfile : current_program_space->objfiles ())
    for (obj_section *sec : objfile->sections ())
      if (!strcmp (bfd_section_name (sec->the_bfd_section), args))
	{
	  if (!sec->ovly_mapped)
	    error (_("Section %s is not mapped"), args);
	  sec->ovly_mapped = 0;
	  return;
	}
  error (_("No overlay section called %s"), args);
}

/* Function: overlay_auto_command
   A utility command to turn on overlay debugging.
   Possibly this should be done via a set/show command.  */

static void
overlay_auto_command (const char *args, int from_tty)
{
  overlay_debugging = ovly_auto;
  enable_overlay_breakpoints ();
  if (info_verbose)
    gdb_printf (_("Automatic overlay debugging enabled."));
}

/* Function: overlay_manual_command
   A utility command to turn on overlay debugging.
   Possibly this should be done via a set/show command.  */

static void
overlay_manual_command (const char *args, int from_tty)
{
  overlay_debugging = ovly_on;
  disable_overlay_breakpoints ();
  if (info_verbose)
    gdb_printf (_("Overlay debugging enabled."));
}

/* Function: overlay_off_command
   A utility command to turn on overlay debugging.
   Possibly this should be done via a set/show command.  */

static void
overlay_off_command (const char *args, int from_tty)
{
  overlay_debugging = ovly_off;
  disable_overlay_breakpoints ();
  if (info_verbose)
    gdb_printf (_("Overlay debugging disabled."));
}

static void
overlay_load_command (const char *args, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();

  if (gdbarch_overlay_update_p (gdbarch))
    gdbarch_overlay_update (gdbarch, NULL);
  else
    error (_("This target does not know how to read its overlay state."));
}

/* Command list chain containing all defined "overlay" subcommands.  */
static struct cmd_list_element *overlaylist;

/* Target Overlays for the "Simplest" overlay manager:

   This is GDB's default target overlay layer.  It works with the
   minimal overlay manager supplied as an example by Cygnus.  The
   entry point is via a function pointer "gdbarch_overlay_update",
   so targets that use a different runtime overlay manager can
   substitute their own overlay_update function and take over the
   function pointer.

   The overlay_update function pokes around in the target's data structures
   to see what overlays are mapped, and updates GDB's overlay mapping with
   this information.

   In this simple implementation, the target data structures are as follows:
   unsigned _novlys;            /# number of overlay sections #/
   unsigned _ovly_table[_novlys][4] = {
   {VMA, OSIZE, LMA, MAPPED},    /# one entry per overlay section #/
   {..., ...,  ..., ...},
   }
   unsigned _novly_regions;     /# number of overlay regions #/
   unsigned _ovly_region_table[_novly_regions][3] = {
   {VMA, OSIZE, MAPPED_TO_LMA},  /# one entry per overlay region #/
   {..., ...,  ...},
   }
   These functions will attempt to update GDB's mappedness state in the
   symbol section table, based on the target's mappedness state.

   To do this, we keep a cached copy of the target's _ovly_table, and
   attempt to detect when the cached copy is invalidated.  The main
   entry point is "simple_overlay_update(SECT), which looks up SECT in
   the cached table and re-reads only the entry for that section from
   the target (whenever possible).  */

/* Cached, dynamically allocated copies of the target data structures: */
static unsigned (*cache_ovly_table)[4] = 0;
static unsigned cache_novlys = 0;
static CORE_ADDR cache_ovly_table_base = 0;
enum ovly_index
  {
    VMA, OSIZE, LMA, MAPPED
  };

/* Throw away the cached copy of _ovly_table.  */

static void
simple_free_overlay_table (void)
{
  xfree (cache_ovly_table);
  cache_novlys = 0;
  cache_ovly_table = NULL;
  cache_ovly_table_base = 0;
}

/* Read an array of ints of size SIZE from the target into a local buffer.
   Convert to host order.  int LEN is number of ints.  */

static void
read_target_long_array (CORE_ADDR memaddr, unsigned int *myaddr,
			int len, int size, enum bfd_endian byte_order)
{
  /* FIXME (alloca): Not safe if array is very large.  */
  gdb_byte *buf = (gdb_byte *) alloca (len * size);
  int i;

  read_memory (memaddr, buf, len * size);
  for (i = 0; i < len; i++)
    myaddr[i] = extract_unsigned_integer (size * i + buf, size, byte_order);
}

/* Find and grab a copy of the target _ovly_table
   (and _novlys, which is needed for the table's size).  */

static int
simple_read_overlay_table (void)
{
  struct bound_minimal_symbol novlys_msym;
  struct bound_minimal_symbol ovly_table_msym;
  struct gdbarch *gdbarch;
  int word_size;
  enum bfd_endian byte_order;

  simple_free_overlay_table ();
  novlys_msym = lookup_minimal_symbol ("_novlys", NULL, NULL);
  if (! novlys_msym.minsym)
    {
      error (_("Error reading inferior's overlay table: "
	     "couldn't find `_novlys' variable\n"
	     "in inferior.  Use `overlay manual' mode."));
      return 0;
    }

  ovly_table_msym = lookup_bound_minimal_symbol ("_ovly_table");
  if (! ovly_table_msym.minsym)
    {
      error (_("Error reading inferior's overlay table: couldn't find "
	     "`_ovly_table' array\n"
	     "in inferior.  Use `overlay manual' mode."));
      return 0;
    }

  gdbarch = ovly_table_msym.objfile->arch ();
  word_size = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  byte_order = gdbarch_byte_order (gdbarch);

  cache_novlys = read_memory_integer (novlys_msym.value_address (),
				      4, byte_order);
  cache_ovly_table
    = (unsigned int (*)[4]) xmalloc (cache_novlys * sizeof (*cache_ovly_table));
  cache_ovly_table_base = ovly_table_msym.value_address ();
  read_target_long_array (cache_ovly_table_base,
			  (unsigned int *) cache_ovly_table,
			  cache_novlys * 4, word_size, byte_order);

  return 1;			/* SUCCESS */
}

/* Function: simple_overlay_update_1
   A helper function for simple_overlay_update.  Assuming a cached copy
   of _ovly_table exists, look through it to find an entry whose vma,
   lma and size match those of OSECT.  Re-read the entry and make sure
   it still matches OSECT (else the table may no longer be valid).
   Set OSECT's mapped state to match the entry.  Return: 1 for
   success, 0 for failure.  */

static int
simple_overlay_update_1 (struct obj_section *osect)
{
  int i;
  asection *bsect = osect->the_bfd_section;
  struct gdbarch *gdbarch = osect->objfile->arch ();
  int word_size = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  for (i = 0; i < cache_novlys; i++)
    if (cache_ovly_table[i][VMA] == bfd_section_vma (bsect)
	&& cache_ovly_table[i][LMA] == bfd_section_lma (bsect))
      {
	read_target_long_array (cache_ovly_table_base + i * word_size,
				(unsigned int *) cache_ovly_table[i],
				4, word_size, byte_order);
	if (cache_ovly_table[i][VMA] == bfd_section_vma (bsect)
	    && cache_ovly_table[i][LMA] == bfd_section_lma (bsect))
	  {
	    osect->ovly_mapped = cache_ovly_table[i][MAPPED];
	    return 1;
	  }
	else	/* Warning!  Warning!  Target's ovly table has changed!  */
	  return 0;
      }
  return 0;
}

/* Function: simple_overlay_update
   If OSECT is NULL, then update all sections' mapped state
   (after re-reading the entire target _ovly_table).
   If OSECT is non-NULL, then try to find a matching entry in the
   cached ovly_table and update only OSECT's mapped state.
   If a cached entry can't be found or the cache isn't valid, then
   re-read the entire cache, and go ahead and update all sections.  */

void
simple_overlay_update (struct obj_section *osect)
{
  /* Were we given an osect to look up?  NULL means do all of them.  */
  if (osect)
    /* Have we got a cached copy of the target's overlay table?  */
    if (cache_ovly_table != NULL)
      {
	/* Does its cached location match what's currently in the
	   symtab?  */
	struct bound_minimal_symbol minsym
	  = lookup_minimal_symbol ("_ovly_table", NULL, NULL);

	if (minsym.minsym == NULL)
	  error (_("Error reading inferior's overlay table: couldn't "
		   "find `_ovly_table' array\n"
		   "in inferior.  Use `overlay manual' mode."));
	
	if (cache_ovly_table_base == minsym.value_address ())
	  /* Then go ahead and try to look up this single section in
	     the cache.  */
	  if (simple_overlay_update_1 (osect))
	    /* Found it!  We're done.  */
	    return;
      }

  /* Cached table no good: need to read the entire table anew.
     Or else we want all the sections, in which case it's actually
     more efficient to read the whole table in one block anyway.  */

  if (! simple_read_overlay_table ())
    return;

  /* Now may as well update all sections, even if only one was requested.  */
  for (objfile *objfile : current_program_space->objfiles ())
    for (obj_section *sect : objfile->sections ())
      if (section_is_overlay (sect))
	{
	  int i;
	  asection *bsect = sect->the_bfd_section;

	  for (i = 0; i < cache_novlys; i++)
	    if (cache_ovly_table[i][VMA] == bfd_section_vma (bsect)
		&& cache_ovly_table[i][LMA] == bfd_section_lma (bsect))
	      { /* obj_section matches i'th entry in ovly_table.  */
		sect->ovly_mapped = cache_ovly_table[i][MAPPED];
		break;		/* finished with inner for loop: break out.  */
	      }
	}
}

/* Default implementation for sym_relocate.  */

bfd_byte *
default_symfile_relocate (struct objfile *objfile, asection *sectp,
			  bfd_byte *buf)
{
  /* Use sectp->owner instead of objfile->obfd.  sectp may point to a
     DWO file.  */
  bfd *abfd = sectp->owner;

  /* We're only interested in sections with relocation
     information.  */
  if ((sectp->flags & SEC_RELOC) == 0)
    return NULL;

  /* We will handle section offsets properly elsewhere, so relocate as if
     all sections begin at 0.  */
  for (asection *sect : gdb_bfd_sections (abfd))
    {
      sect->output_section = sect;
      sect->output_offset = 0;
    }

  return bfd_simple_get_relocated_section_contents (abfd, sectp, buf, NULL);
}

/* Relocate the contents of a debug section SECTP in ABFD.  The
   contents are stored in BUF if it is non-NULL, or returned in a
   malloc'd buffer otherwise.

   For some platforms and debug info formats, shared libraries contain
   relocations against the debug sections (particularly for DWARF-2;
   one affected platform is PowerPC GNU/Linux, although it depends on
   the version of the linker in use).  Also, ELF object files naturally
   have unresolved relocations for their debug sections.  We need to apply
   the relocations in order to get the locations of symbols correct.
   Another example that may require relocation processing, is the
   DWARF-2 .eh_frame section in .o files, although it isn't strictly a
   debug section.  */

bfd_byte *
symfile_relocate_debug_section (struct objfile *objfile,
				asection *sectp, bfd_byte *buf)
{
  gdb_assert (objfile->sf->sym_relocate);

  return (*objfile->sf->sym_relocate) (objfile, sectp, buf);
}

symfile_segment_data_up
get_symfile_segment_data (bfd *abfd)
{
  const struct sym_fns *sf = find_sym_fns (abfd);

  if (sf == NULL)
    return NULL;

  return sf->sym_segments (abfd);
}

/* Given:
   - DATA, containing segment addresses from the object file ABFD, and
     the mapping from ABFD's sections onto the segments that own them,
     and
   - SEGMENT_BASES[0 .. NUM_SEGMENT_BASES - 1], holding the actual
     segment addresses reported by the target,
   store the appropriate offsets for each section in OFFSETS.

   If there are fewer entries in SEGMENT_BASES than there are segments
   in DATA, then apply SEGMENT_BASES' last entry to all the segments.

   If there are more entries, then ignore the extra.  The target may
   not be able to distinguish between an empty data segment and a
   missing data segment; a missing text segment is less plausible.  */

int
symfile_map_offsets_to_segments (bfd *abfd,
				 const struct symfile_segment_data *data,
				 section_offsets &offsets,
				 int num_segment_bases,
				 const CORE_ADDR *segment_bases)
{
  int i;
  asection *sect;

  /* It doesn't make sense to call this function unless you have some
     segment base addresses.  */
  gdb_assert (num_segment_bases > 0);

  /* If we do not have segment mappings for the object file, we
     can not relocate it by segments.  */
  gdb_assert (data != NULL);
  gdb_assert (data->segments.size () > 0);

  for (i = 0, sect = abfd->sections; sect != NULL; i++, sect = sect->next)
    {
      int which = data->segment_info[i];

      gdb_assert (0 <= which && which <= data->segments.size ());

      /* Don't bother computing offsets for sections that aren't
	 loaded as part of any segment.  */
      if (! which)
	continue;

      /* Use the last SEGMENT_BASES entry as the address of any extra
	 segments mentioned in DATA->segment_info.  */
      if (which > num_segment_bases)
	which = num_segment_bases;

      offsets[i] = segment_bases[which - 1] - data->segments[which - 1].base;
    }

  return 1;
}

static void
symfile_find_segment_sections (struct objfile *objfile)
{
  bfd *abfd = objfile->obfd.get ();
  int i;
  asection *sect;

  symfile_segment_data_up data = get_symfile_segment_data (abfd);
  if (data == NULL)
    return;

  if (data->segments.size () != 1 && data->segments.size () != 2)
    return;

  for (i = 0, sect = abfd->sections; sect != NULL; i++, sect = sect->next)
    {
      int which = data->segment_info[i];

      if (which == 1)
	{
	  if (objfile->sect_index_text == -1)
	    objfile->sect_index_text = sect->index;

	  if (objfile->sect_index_rodata == -1)
	    objfile->sect_index_rodata = sect->index;
	}
      else if (which == 2)
	{
	  if (objfile->sect_index_data == -1)
	    objfile->sect_index_data = sect->index;

	  if (objfile->sect_index_bss == -1)
	    objfile->sect_index_bss = sect->index;
	}
    }
}

/* Listen for free_objfile events.  */

static void
symfile_free_objfile (struct objfile *objfile)
{
  /* Remove the target sections owned by this objfile.  */
  objfile->pspace->remove_target_sections (objfile);
}

/* Wrapper around the quick_symbol_functions expand_symtabs_matching "method".
   Expand all symtabs that match the specified criteria.
   See quick_symbol_functions.expand_symtabs_matching for details.  */

bool
expand_symtabs_matching
  (gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
   const lookup_name_info &lookup_name,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
   gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
   block_search_flags search_flags,
   enum search_domain kind)
{
  for (objfile *objfile : current_program_space->objfiles ())
    if (!objfile->expand_symtabs_matching (file_matcher,
					   &lookup_name,
					   symbol_matcher,
					   expansion_notify,
					   search_flags,
					   UNDEF_DOMAIN,
					   kind))
      return false;
  return true;
}

/* Wrapper around the quick_symbol_functions map_symbol_filenames "method".
   Map function FUN over every file.
   See quick_symbol_functions.map_symbol_filenames for details.  */

void
map_symbol_filenames (gdb::function_view<symbol_filename_ftype> fun,
		      bool need_fullname)
{
  for (objfile *objfile : current_program_space->objfiles ())
    objfile->map_symbol_filenames (fun, need_fullname);
}

#if GDB_SELF_TEST

namespace selftests {
namespace filename_language {

static void test_filename_language ()
{
  /* This test messes up the filename_language_table global.  */
  scoped_restore restore_flt = make_scoped_restore (&filename_language_table);

  /* Test deducing an unknown extension.  */
  language lang = deduce_language_from_filename ("myfile.blah");
  SELF_CHECK (lang == language_unknown);

  /* Test deducing a known extension.  */
  lang = deduce_language_from_filename ("myfile.c");
  SELF_CHECK (lang == language_c);

  /* Test adding a new extension using the internal API.  */
  add_filename_language (".blah", language_pascal);
  lang = deduce_language_from_filename ("myfile.blah");
  SELF_CHECK (lang == language_pascal);
}

static void
test_set_ext_lang_command ()
{
  /* This test messes up the filename_language_table global.  */
  scoped_restore restore_flt = make_scoped_restore (&filename_language_table);

  /* Confirm that the .hello extension is not known.  */
  language lang = deduce_language_from_filename ("cake.hello");
  SELF_CHECK (lang == language_unknown);

  /* Test adding a new extension using the CLI command.  */
  ext_args = ".hello rust";
  set_ext_lang_command (NULL, 1, NULL);

  lang = deduce_language_from_filename ("cake.hello");
  SELF_CHECK (lang == language_rust);

  /* Test overriding an existing extension using the CLI command.  */
  int size_before = filename_language_table.size ();
  ext_args = ".hello pascal";
  set_ext_lang_command (NULL, 1, NULL);
  int size_after = filename_language_table.size ();

  lang = deduce_language_from_filename ("cake.hello");
  SELF_CHECK (lang == language_pascal);
  SELF_CHECK (size_before == size_after);
}

} /* namespace filename_language */
} /* namespace selftests */

#endif /* GDB_SELF_TEST */

void _initialize_symfile ();
void
_initialize_symfile ()
{
  struct cmd_list_element *c;

  gdb::observers::free_objfile.attach (symfile_free_objfile, "symfile");

#define READNOW_READNEVER_HELP \
  "The '-readnow' option will cause GDB to read the entire symbol file\n\
immediately.  This makes the command slower, but may make future operations\n\
faster.\n\
The '-readnever' option will prevent GDB from reading the symbol file's\n\
symbolic debug information."

  c = add_cmd ("symbol-file", class_files, symbol_file_command, _("\
Load symbol table from executable file FILE.\n\
Usage: symbol-file [-readnow | -readnever] [-o OFF] FILE\n\
OFF is an optional offset which is added to each section address.\n\
The `file' command can also load symbol tables, as well as setting the file\n\
to execute.\n" READNOW_READNEVER_HELP), &cmdlist);
  set_cmd_completer (c, filename_completer);

  c = add_cmd ("add-symbol-file", class_files, add_symbol_file_command, _("\
Load symbols from FILE, assuming FILE has been dynamically loaded.\n\
Usage: add-symbol-file FILE [-readnow | -readnever] [-o OFF] [ADDR] \
[-s SECT-NAME SECT-ADDR]...\n\
ADDR is the starting address of the file's text.\n\
Each '-s' argument provides a section name and address, and\n\
should be specified if the data and bss segments are not contiguous\n\
with the text.  SECT-NAME is a section name to be loaded at SECT-ADDR.\n\
OFF is an optional offset which is added to the default load addresses\n\
of all sections for which no other address was specified.\n"
READNOW_READNEVER_HELP),
	       &cmdlist);
  set_cmd_completer (c, filename_completer);

  c = add_cmd ("remove-symbol-file", class_files,
	       remove_symbol_file_command, _("\
Remove a symbol file added via the add-symbol-file command.\n\
Usage: remove-symbol-file FILENAME\n\
       remove-symbol-file -a ADDRESS\n\
The file to remove can be identified by its filename or by an address\n\
that lies within the boundaries of this symbol file in memory."),
	       &cmdlist);

  c = add_cmd ("load", class_files, load_command, _("\
Dynamically load FILE into the running program.\n\
FILE symbols are recorded for access from GDB.\n\
Usage: load [FILE] [OFFSET]\n\
An optional load OFFSET may also be given as a literal address.\n\
When OFFSET is provided, FILE must also be provided.  FILE can be provided\n\
on its own."), &cmdlist);
  set_cmd_completer (c, filename_completer);

  cmd_list_element *overlay_cmd
    = add_basic_prefix_cmd ("overlay", class_support,
			    _("Commands for debugging overlays."), &overlaylist,
			    0, &cmdlist);

  add_com_alias ("ovly", overlay_cmd, class_support, 1);
  add_com_alias ("ov", overlay_cmd, class_support, 1);

  add_cmd ("map-overlay", class_support, map_overlay_command,
	   _("Assert that an overlay section is mapped."), &overlaylist);

  add_cmd ("unmap-overlay", class_support, unmap_overlay_command,
	   _("Assert that an overlay section is unmapped."), &overlaylist);

  add_cmd ("list-overlays", class_support, list_overlays_command,
	   _("List mappings of overlay sections."), &overlaylist);

  add_cmd ("manual", class_support, overlay_manual_command,
	   _("Enable overlay debugging."), &overlaylist);
  add_cmd ("off", class_support, overlay_off_command,
	   _("Disable overlay debugging."), &overlaylist);
  add_cmd ("auto", class_support, overlay_auto_command,
	   _("Enable automatic overlay debugging."), &overlaylist);
  add_cmd ("load-target", class_support, overlay_load_command,
	   _("Read the overlay mapping state from the target."), &overlaylist);

  /* Filename extension to source language lookup table: */
  add_setshow_string_noescape_cmd ("extension-language", class_files,
				   &ext_args, _("\
Set mapping between filename extension and source language."), _("\
Show mapping between filename extension and source language."), _("\
Usage: set extension-language .foo bar"),
				   set_ext_lang_command,
				   show_ext_args,
				   &setlist, &showlist);

  add_info ("extensions", info_ext_lang_command,
	    _("All filename extensions associated with a source language."));

  add_setshow_optional_filename_cmd ("debug-file-directory", class_support,
				     &debug_file_directory, _("\
Set the directories where separate debug symbols are searched for."), _("\
Show the directories where separate debug symbols are searched for."), _("\
Separate debug symbols are first searched for in the same\n\
directory as the binary, then in the `" DEBUG_SUBDIRECTORY "' subdirectory,\n\
and lastly at the path of the directory of the binary with\n\
each global debug-file-directory component prepended."),
				     NULL,
				     show_debug_file_directory,
				     &setlist, &showlist);

  add_setshow_enum_cmd ("symbol-loading", no_class,
			print_symbol_loading_enums, &print_symbol_loading,
			_("\
Set printing of symbol loading messages."), _("\
Show printing of symbol loading messages."), _("\
off   == turn all messages off\n\
brief == print messages for the executable,\n\
	 and brief messages for shared libraries\n\
full  == print messages for the executable,\n\
	 and messages for each shared library."),
			NULL,
			NULL,
			&setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("separate-debug-file", no_class,
			   &separate_debug_file_debug, _("\
Set printing of separate debug info file search debug."), _("\
Show printing of separate debug info file search debug."), _("\
When on, GDB prints the searched locations while looking for separate debug \
info files."), NULL, NULL, &setdebuglist, &showdebuglist);

#if GDB_SELF_TEST
  selftests::register_test
    ("filename_language", selftests::filename_language::test_filename_language);
  selftests::register_test
    ("set_ext_lang_command",
     selftests::filename_language::test_set_ext_lang_command);
#endif
}
