/* Read AIX xcoff symbol tables and convert to internal format, for GDB.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.
   Derived from coffread.c, dbxread.c, and a lot of hacking.
   Contributed by IBM Corporation.

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
#include "bfd.h"

#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#include <sys/stat.h>
#include <algorithm>

#include "coff/internal.h"
#include "libcoff.h"
#include "coff/xcoff.h"
#include "libxcoff.h"
#include "coff/rs6000.h"
#include "xcoffread.h"

#include "symtab.h"
#include "gdbtypes.h"
/* FIXME: ezannoni/2004-02-13 Verify if the include below is really needed.  */
#include "symfile.h"
#include "objfiles.h"
#include "buildsym-legacy.h"
#include "stabsread.h"
#include "expression.h"
#include "complaints.h"
#include "psymtab.h"
#include "dwarf2/sect-names.h"
#include "dwarf2/public.h"

#include "gdb-stabs.h"

/* For interface with stabsread.c.  */
#include "aout/stab_gnu.h"


/* We put a pointer to this structure in the read_symtab_private field
   of the psymtab.  */

struct xcoff_symloc
  {

    /* First symbol number for this file.  */

    int first_symnum;

    /* Number of symbols in the section of the symbol table devoted to
       this file's symbols (actually, the section bracketed may contain
       more than just this file's symbols).  If numsyms is 0, the only
       reason for this thing's existence is the dependency list.  Nothing
       else will happen when it is read in.  */

    int numsyms;

    /* Position of the start of the line number information for this
       psymtab.  */
    unsigned int lineno_off;
  };

/* Remember what we deduced to be the source language of this psymtab.  */

static enum language psymtab_language = language_unknown;


/* Simplified internal version of coff symbol table information.  */

struct xcoff_symbol
  {
    char *c_name;
    int c_symnum;		/* Symbol number of this entry.  */
    int c_naux;			/* 0 if syment only, 1 if syment + auxent.  */
    CORE_ADDR c_value;
    unsigned char c_sclass;
    int c_secnum;
    unsigned int c_type;
  };

/* Last function's saved coff symbol `cs'.  */

static struct xcoff_symbol fcn_cs_saved;

static bfd *symfile_bfd;

/* Core address of start and end of text of current source file.
   This is calculated from the first function seen after a C_FILE
   symbol.  */


static CORE_ADDR cur_src_end_addr;

/* Core address of the end of the first object file.  */

static CORE_ADDR first_object_file_end;

/* Initial symbol-table-debug-string vector length.  */

#define	INITIAL_STABVECTOR_LENGTH	40

/* Size of a COFF symbol.  I think it is always 18, so I'm not sure
   there is any reason not to just use a #define, but might as well
   ask BFD for the size and store it here, I guess.  */

static unsigned local_symesz;

struct xcoff_symfile_info
  {
    file_ptr min_lineno_offset {};	/* Where in file lowest line#s are.  */
    file_ptr max_lineno_offset {};	/* 1+last byte of line#s in file.  */

    /* Pointer to the string table.  */
    char *strtbl = nullptr;

    /* Pointer to debug section.  */
    char *debugsec = nullptr;

    /* Pointer to the a.out symbol table.  */
    char *symtbl = nullptr;

    /* Number of symbols in symtbl.  */
    int symtbl_num_syms = 0;

    /* Offset in data section to TOC anchor.  */
    CORE_ADDR toc_offset = 0;
  };

/* Key for XCOFF-associated data.  */

static const registry<objfile>::key<xcoff_symfile_info> xcoff_objfile_data_key;

/* Convenience macro to access the per-objfile XCOFF data.  */

#define XCOFF_DATA(objfile)						\
  xcoff_objfile_data_key.get (objfile)

/* XCOFF names for dwarf sections.  There is no compressed sections.  */

static const struct dwarf2_debug_sections dwarf2_xcoff_names = {
  { ".dwinfo", NULL },
  { ".dwabrev", NULL },
  { ".dwline", NULL },
  { ".dwloc", NULL },
  { NULL, NULL }, /* debug_loclists */
  /* AIX XCOFF defines one, named DWARF section for macro debug information.
     XLC does not generate debug_macinfo for DWARF4 and below.
     The section is assigned to debug_macro for DWARF5 and above. */
  { NULL, NULL },
  { ".dwmac", NULL },
  { ".dwstr", NULL },
  { NULL, NULL }, /* debug_str_offsets */
  { NULL, NULL }, /* debug_line_str */
  { ".dwrnges", NULL },
  { NULL, NULL }, /* debug_rnglists */
  { ".dwpbtyp", NULL },
  { NULL, NULL }, /* debug_addr */
  { ".dwframe", NULL },
  { NULL, NULL }, /* eh_frame */
  { NULL, NULL }, /* gdb_index */
  { NULL, NULL }, /* debug_names */
  { NULL, NULL }, /* debug_aranges */
  23
};

static void
bf_notfound_complaint (void)
{
  complaint (_("line numbers off, `.bf' symbol not found"));
}

static void
ef_complaint (int arg1)
{
  complaint (_("Mismatched .ef symbol ignored starting at symnum %d"), arg1);
}

static void
eb_complaint (int arg1)
{
  complaint (_("Mismatched .eb symbol ignored starting at symnum %d"), arg1);
}

static void xcoff_initial_scan (struct objfile *, symfile_add_flags);

static void scan_xcoff_symtab (minimal_symbol_reader &,
			       psymtab_storage *partial_symtabs,
			       struct objfile *);

static const char *xcoff_next_symbol_text (struct objfile *);

static void record_include_begin (struct xcoff_symbol *);

static void
enter_line_range (struct subfile *, unsigned, unsigned,
		  CORE_ADDR, CORE_ADDR, unsigned *);

static void init_stringtab (bfd *, file_ptr, struct objfile *);

static void xcoff_symfile_init (struct objfile *);

static void xcoff_new_init (struct objfile *);

static void xcoff_symfile_finish (struct objfile *);

static char *coff_getfilename (union internal_auxent *, struct objfile *);

static void read_symbol (struct internal_syment *, int);

static int read_symbol_lineno (int);

static CORE_ADDR read_symbol_nvalue (int);

static struct symbol *process_xcoff_symbol (struct xcoff_symbol *,
					    struct objfile *);

static void read_xcoff_symtab (struct objfile *, legacy_psymtab *);

#if 0
static void add_stab_to_list (char *, struct pending_stabs **);
#endif

static void record_include_end (struct xcoff_symbol *);

static void process_linenos (CORE_ADDR, CORE_ADDR);


/* Translate from a COFF section number (target_index) to a SECT_OFF_*
   code.  */
static int secnum_to_section (int, struct objfile *);
static asection *secnum_to_bfd_section (int, struct objfile *);

struct xcoff_find_targ_sec_arg
  {
    int targ_index;
    int *resultp;
    asection **bfd_sect;
    struct objfile *objfile;
  };

static void find_targ_sec (bfd *, asection *, void *);

static void
find_targ_sec (bfd *abfd, asection *sect, void *obj)
{
  struct xcoff_find_targ_sec_arg *args
    = (struct xcoff_find_targ_sec_arg *) obj;
  struct objfile *objfile = args->objfile;

  if (sect->target_index == args->targ_index)
    {
      /* This is the section.  Figure out what SECT_OFF_* code it is.  */
      if (bfd_section_flags (sect) & SEC_CODE)
	*args->resultp = SECT_OFF_TEXT (objfile);
      else if (bfd_section_flags (sect) & SEC_LOAD)
	*args->resultp = SECT_OFF_DATA (objfile);
      else
	*args->resultp = gdb_bfd_section_index (abfd, sect);
      *args->bfd_sect = sect;
    }
}

/* Search all BFD sections for the section whose target_index is
   equal to N_SCNUM.  Set *BFD_SECT to that section.  The section's
   associated index in the objfile's section_offset table is also
   stored in *SECNUM.

   If no match is found, *BFD_SECT is set to NULL, and *SECNUM
   is set to the text section's number.  */

static void
xcoff_secnum_to_sections (int n_scnum, struct objfile *objfile,
			  asection **bfd_sect, int *secnum)
{
  struct xcoff_find_targ_sec_arg args;

  args.targ_index = n_scnum;
  args.resultp = secnum;
  args.bfd_sect = bfd_sect;
  args.objfile = objfile;

  *bfd_sect = NULL;
  *secnum = SECT_OFF_TEXT (objfile);

  bfd_map_over_sections (objfile->obfd.get (), find_targ_sec, &args);
}

/* Return the section number (SECT_OFF_*) that N_SCNUM points to.  */

static int
secnum_to_section (int n_scnum, struct objfile *objfile)
{
  int secnum;
  asection *ignored;

  xcoff_secnum_to_sections (n_scnum, objfile, &ignored, &secnum);
  return secnum;
}

/* Return the BFD section that N_SCNUM points to.  */

static asection *
secnum_to_bfd_section (int n_scnum, struct objfile *objfile)
{
  int ignored;
  asection *bfd_sect;

  xcoff_secnum_to_sections (n_scnum, objfile, &bfd_sect, &ignored);
  return bfd_sect;
}

/* add a given stab string into given stab vector.  */

#if 0

static void
add_stab_to_list (char *stabname, struct pending_stabs **stabvector)
{
  if (*stabvector == NULL)
    {
      *stabvector = (struct pending_stabs *)
	xmalloc (sizeof (struct pending_stabs) +
		 INITIAL_STABVECTOR_LENGTH * sizeof (char *));
      (*stabvector)->count = 0;
      (*stabvector)->length = INITIAL_STABVECTOR_LENGTH;
    }
  else if ((*stabvector)->count >= (*stabvector)->length)
    {
      (*stabvector)->length += INITIAL_STABVECTOR_LENGTH;
      *stabvector = (struct pending_stabs *)
	xrealloc ((char *) *stabvector, sizeof (struct pending_stabs) +
		  (*stabvector)->length * sizeof (char *));
    }
  (*stabvector)->stab[(*stabvector)->count++] = stabname;
}

#endif

/* Linenos are processed on a file-by-file basis.

   Two reasons:

   1) xlc (IBM's native c compiler) postpones static function code
   emission to the end of a compilation unit.  This way it can
   determine if those functions (statics) are needed or not, and
   can do some garbage collection (I think).  This makes line
   numbers and corresponding addresses unordered, and we end up
   with a line table like:


   lineno       addr
   foo()          10    0x100
   20   0x200
   30   0x300

   foo3()         70    0x400
   80   0x500
   90   0x600

   static foo2()
   40   0x700
   50   0x800
   60   0x900           

   and that breaks gdb's binary search on line numbers, if the
   above table is not sorted on line numbers.  And that sort
   should be on function based, since gcc can emit line numbers
   like:

   10   0x100   - for the init/test part of a for stmt.
   20   0x200
   30   0x300
   10   0x400   - for the increment part of a for stmt.

   arrange_linetable() will do this sorting.

   2)   aix symbol table might look like:

   c_file               // beginning of a new file
   .bi          // beginning of include file
   .ei          // end of include file
   .bi
   .ei

   basically, .bi/.ei pairs do not necessarily encapsulate
   their scope.  They need to be recorded, and processed later
   on when we come the end of the compilation unit.
   Include table (inclTable) and process_linenos() handle
   that.  */


/* Given a line table with function entries are marked, arrange its
   functions in ascending order and strip off function entry markers
   and return it in a newly created table.  */

/* FIXME: I think all this stuff can be replaced by just passing
   sort_linevec = 1 to end_compunit_symtab.  */

static void
arrange_linetable (std::vector<linetable_entry> &old_linetable)
{
  std::vector<linetable_entry> fentries;

  for (int ii = 0; ii < old_linetable.size (); ++ii)
    {
      if (!old_linetable[ii].is_stmt)
	continue;

      if (old_linetable[ii].line == 0)
	{
	  /* Function entry found.  */
	  fentries.emplace_back ();
	  linetable_entry &e = fentries.back ();
	  e.line = ii;
	  e.is_stmt = true;
	  e.set_unrelocated_pc (old_linetable[ii].unrelocated_pc ());
	}
    }

  if (fentries.empty ())
    return;

  std::sort (fentries.begin (), fentries.end ());

  /* Allocate a new line table.  */
  std::vector<linetable_entry> new_linetable;
  new_linetable.reserve (old_linetable.size ());

  /* If line table does not start with a function beginning, copy up until
     a function begin.  */
  for (int i = 0; i < old_linetable.size () && old_linetable[i].line != 0; ++i)
    new_linetable.push_back (old_linetable[i]);

  /* Now copy function lines one by one.  */
  for (const linetable_entry &entry : fentries)
    {
      /* If the function was compiled with XLC, we may have to add an
	 extra line to cover the function prologue.  */
      int jj = entry.line;
      if (jj + 1 < old_linetable.size ()
	  && (old_linetable[jj].unrelocated_pc ()
	      != old_linetable[jj + 1].unrelocated_pc ()))
	{
	  new_linetable.push_back (old_linetable[jj]);
	  new_linetable.back ().line = old_linetable[jj + 1].line;
	}

      for (jj = entry.line + 1;
	   jj < old_linetable.size () && old_linetable[jj].line != 0;
	   ++jj)
	new_linetable.push_back (old_linetable[jj]);
    }

  new_linetable.shrink_to_fit ();
  old_linetable = std::move (new_linetable);
}

/* include file support: C_BINCL/C_EINCL pairs will be kept in the 
   following `IncludeChain'.  At the end of each symtab (end_compunit_symtab),
   we will determine if we should create additional symtab's to
   represent if (the include files.  */


typedef struct _inclTable
{
  char *name;			/* include filename */

  /* Offsets to the line table.  end points to the last entry which is
     part of this include file.  */
  int begin, end;

  struct subfile *subfile;
  unsigned funStartLine;	/* Start line # of its function.  */
}
InclTable;

#define	INITIAL_INCLUDE_TABLE_LENGTH	20
static InclTable *inclTable;	/* global include table */
static int inclIndx;		/* last entry to table */
static int inclLength;		/* table length */
static int inclDepth;		/* nested include depth */

/* subfile structure for the main compilation unit.  */
static subfile *main_subfile;

static void allocate_include_entry (void);

static void
record_include_begin (struct xcoff_symbol *cs)
{
  if (inclDepth)
    {
      /* In xcoff, we assume include files cannot be nested (not in .c files
	 of course, but in corresponding .s files.).  */

      /* This can happen with old versions of GCC.
	 GCC 2.3.3-930426 does not exhibit this on a test case which
	 a user said produced the message for him.  */
      complaint (_("Nested C_BINCL symbols"));
    }
  ++inclDepth;

  allocate_include_entry ();

  inclTable[inclIndx].name = cs->c_name;
  inclTable[inclIndx].begin = cs->c_value;
}

static void
record_include_end (struct xcoff_symbol *cs)
{
  InclTable *pTbl;

  if (inclDepth == 0)
    {
      complaint (_("Mismatched C_BINCL/C_EINCL pair"));
    }

  allocate_include_entry ();

  pTbl = &inclTable[inclIndx];
  pTbl->end = cs->c_value;

  --inclDepth;
  ++inclIndx;
}

static void
allocate_include_entry (void)
{
  if (inclTable == NULL)
    {
      inclTable = XCNEWVEC (InclTable, INITIAL_INCLUDE_TABLE_LENGTH);
      inclLength = INITIAL_INCLUDE_TABLE_LENGTH;
      inclIndx = 0;
      main_subfile = new subfile;
    }
  else if (inclIndx >= inclLength)
    {
      inclLength += INITIAL_INCLUDE_TABLE_LENGTH;
      inclTable = XRESIZEVEC (InclTable, inclTable, inclLength);
      memset (inclTable + inclLength - INITIAL_INCLUDE_TABLE_LENGTH,
	      '\0', sizeof (InclTable) * INITIAL_INCLUDE_TABLE_LENGTH);
    }
}

/* Global variable to pass the psymtab down to all the routines involved
   in psymtab to symtab processing.  */
static legacy_psymtab *this_symtab_psymtab;

/* Objfile related to this_symtab_psymtab; set at the same time.  */
static struct objfile *this_symtab_objfile;

/* given the start and end addresses of a compilation unit (or a csect,
   at times) process its lines and create appropriate line vectors.  */

static void
process_linenos (CORE_ADDR start, CORE_ADDR end)
{
  int offset;
  file_ptr max_offset
    = XCOFF_DATA (this_symtab_objfile)->max_lineno_offset;

  /* In the main source file, any time we see a function entry, we
     reset this variable to function's absolute starting line number.
     All the following line numbers in the function are relative to
     this, and we record absolute line numbers in record_line().  */

  unsigned int main_source_baseline = 0;

  unsigned *firstLine;

  offset =
    ((struct xcoff_symloc *) this_symtab_psymtab->read_symtab_private)->lineno_off;
  if (offset == 0)
    goto return_after_cleanup;

  if (inclIndx == 0)
    /* All source lines were in the main source file.  None in include
       files.  */

    enter_line_range (main_subfile, offset, 0, start, end,
		      &main_source_baseline);

  else
    {
      /* There was source with line numbers in include files.  */

      int linesz =
	coff_data (this_symtab_objfile->obfd)->local_linesz;
      main_source_baseline = 0;

      for (int ii = 0; ii < inclIndx; ++ii)
	{
	  /* If there is main file source before include file, enter it.  */
	  if (offset < inclTable[ii].begin)
	    {
	      enter_line_range
		(main_subfile, offset, inclTable[ii].begin - linesz,
		 start, 0, &main_source_baseline);
	    }

	  if (strcmp (inclTable[ii].name, get_last_source_file ()) == 0)
	    {
	      /* The entry in the include table refers to the main source
		 file.  Add the lines to the main subfile.  */

	      main_source_baseline = inclTable[ii].funStartLine;
	      enter_line_range
		(main_subfile, inclTable[ii].begin, inclTable[ii].end,
		 start, 0, &main_source_baseline);
	      inclTable[ii].subfile = main_subfile;
	    }
	  else
	    {
	      /* Have a new subfile for the include file.  */
	      inclTable[ii].subfile = new subfile;

	      firstLine = &(inclTable[ii].funStartLine);

	      /* Enter include file's lines now.  */
	      enter_line_range (inclTable[ii].subfile, inclTable[ii].begin,
				inclTable[ii].end, start, 0, firstLine);
	    }

	  if (offset <= inclTable[ii].end)
	    offset = inclTable[ii].end + linesz;
	}

      /* All the include files' line have been processed at this point.  Now,
	 enter remaining lines of the main file, if any left.  */
      if (offset < max_offset + 1 - linesz)
	{
	  enter_line_range (main_subfile, offset, 0, start, end,
			    &main_source_baseline);
	}
    }

  /* Process main file's line numbers.  */
  if (!main_subfile->line_vector_entries.empty ())
    {
      /* Line numbers are not necessarily ordered.  xlc compilation will
	 put static function to the end.  */
      arrange_linetable (main_subfile->line_vector_entries);
    }

  /* Now, process included files' line numbers.  */

  for (int ii = 0; ii < inclIndx; ++ii)
    {
      if (inclTable[ii].subfile != main_subfile
	  && !inclTable[ii].subfile->line_vector_entries.empty ())
	{
	  /* Line numbers are not necessarily ordered.  xlc compilation will
	     put static function to the end.  */
	  arrange_linetable (inclTable[ii].subfile->line_vector_entries);

	  push_subfile ();

	  /* For the same include file, we might want to have more than one
	     subfile.  This happens if we have something like:

	     ......
	     #include "foo.h"
	     ......
	     #include "foo.h"
	     ......

	     while foo.h including code in it.  (stupid but possible)
	     Since start_subfile() looks at the name and uses an
	     existing one if finds, we need to provide a fake name and
	     fool it.  */

#if 0
	  start_subfile (inclTable[ii].name);
#else
	  {
	    /* Pick a fake name that will produce the same results as this
	       one when passed to deduce_language_from_filename.  Kludge on
	       top of kludge.  */
	    const char *fakename = strrchr (inclTable[ii].name, '.');

	    if (fakename == NULL)
	      fakename = " ?";
	    start_subfile (fakename);
	  }
	  struct subfile *current_subfile = get_current_subfile ();
	  current_subfile->name = inclTable[ii].name;
	  current_subfile->name_for_id = inclTable[ii].name;
#endif

	  start_subfile (pop_subfile ());
	}
    }

return_after_cleanup:

  /* We don't want to keep alloc/free'ing the global include file table.  */
  inclIndx = 0;
}

static void
aix_process_linenos (struct objfile *objfile)
{
  /* There is no linenos to read if there are only dwarf info.  */
  if (this_symtab_psymtab == NULL)
    return;

  /* Process line numbers and enter them into line vector.  */
  process_linenos (get_last_source_start_addr (), cur_src_end_addr);
}


/* Enter a given range of lines into the line vector.
   can be called in the following two ways:
   enter_line_range (subfile, beginoffset, endoffset,
		     startaddr, 0, firstLine)  or
   enter_line_range (subfile, beginoffset, 0, 
		     startaddr, endaddr, firstLine)

   endoffset points to the last line table entry that we should pay
   attention to.  */

static void
enter_line_range (struct subfile *subfile, unsigned beginoffset,
		  unsigned endoffset,	/* offsets to line table */
		  CORE_ADDR startaddr,	/* offsets to line table */
		  CORE_ADDR endaddr, unsigned *firstLine)
{
  struct objfile *objfile = this_symtab_objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  unsigned int curoffset;
  CORE_ADDR addr;
  void *ext_lnno;
  struct internal_lineno int_lnno;
  unsigned int limit_offset;
  bfd *abfd;
  int linesz;

  if (endoffset == 0 && startaddr == 0 && endaddr == 0)
    return;
  curoffset = beginoffset;
  limit_offset = XCOFF_DATA (objfile)->max_lineno_offset;

  if (endoffset != 0)
    {
      if (endoffset >= limit_offset)
	{
	  complaint (_("Bad line table offset in C_EINCL directive"));
	  return;
	}
      limit_offset = endoffset;
    }
  else
    limit_offset -= 1;

  abfd = objfile->obfd.get ();
  linesz = coff_data (abfd)->local_linesz;
  ext_lnno = alloca (linesz);

  while (curoffset <= limit_offset)
    {
      if (bfd_seek (abfd, curoffset, SEEK_SET) != 0
	  || bfd_read (ext_lnno, linesz, abfd) != linesz)
	return;
      bfd_coff_swap_lineno_in (abfd, ext_lnno, &int_lnno);

      /* Find the address this line represents.  */
      addr = (int_lnno.l_lnno
	      ? int_lnno.l_addr.l_paddr
	      : read_symbol_nvalue (int_lnno.l_addr.l_symndx));
      addr += objfile->text_section_offset ();

      if (addr < startaddr || (endaddr && addr >= endaddr))
	return;

      CORE_ADDR record_addr = (gdbarch_addr_bits_remove (gdbarch, addr)
			       - objfile->text_section_offset ());
      if (int_lnno.l_lnno == 0)
	{
	  *firstLine = read_symbol_lineno (int_lnno.l_addr.l_symndx);
	  record_line (subfile, 0, unrelocated_addr (record_addr));
	  --(*firstLine);
	}
      else
	record_line (subfile, *firstLine + int_lnno.l_lnno,
		     unrelocated_addr (record_addr));
      curoffset += linesz;
    }
}


/* Save the vital information for use when closing off the current file.
   NAME is the file name the symbols came from, START_ADDR is the first
   text address for the file, and SIZE is the number of bytes of text.  */

#define complete_symtab(name, start_addr) {	\
  set_last_source_file (name);			\
  set_last_source_start_addr (start_addr);	\
}


/* Refill the symbol table input buffer
   and set the variables that control fetching entries from it.
   Reports an error if no data available.
   This function can read past the end of the symbol table
   (into the string table) but this does no harm.  */

/* Create a new minimal symbol (using record_with_info).

   Creation of all new minimal symbols should go through this function
   rather than calling the various record functions in order
   to make sure that all symbol addresses get properly relocated.

   Arguments are:

   NAME - the symbol's name (but if NAME starts with a period, that
   leading period is discarded).
   ADDRESS - the symbol's address, prior to relocation.  This function
      relocates the address before recording the minimal symbol.
   MS_TYPE - the symbol's type.
   N_SCNUM - the symbol's XCOFF section number.
   OBJFILE - the objfile associated with the minimal symbol.  */

static void
record_minimal_symbol (minimal_symbol_reader &reader,
		       const char *name, unrelocated_addr address,
		       enum minimal_symbol_type ms_type,
		       int n_scnum,
		       struct objfile *objfile)
{
  if (name[0] == '.')
    ++name;

  reader.record_with_info (name, address, ms_type,
			   secnum_to_section (n_scnum, objfile));
}

/* xcoff has static blocks marked in `.bs', `.es' pairs.  They cannot be
   nested.  At any given time, a symbol can only be in one static block.
   This is the base address of current static block, zero if non exists.  */

static int static_block_base = 0;

/* Section number for the current static block.  */

static int static_block_section = -1;

/* true if space for symbol name has been allocated.  */

static int symname_alloced = 0;

/* Next symbol to read.  Pointer into raw seething symbol table.  */

static char *raw_symbol;

/* This is the function which stabsread.c calls to get symbol
   continuations.  */

static const char *
xcoff_next_symbol_text (struct objfile *objfile)
{
  struct internal_syment symbol;
  const char *retval;

  /* FIXME: is this the same as the passed arg?  */
  if (this_symtab_objfile)
    objfile = this_symtab_objfile;

  bfd_coff_swap_sym_in (objfile->obfd.get (), raw_symbol, &symbol);
  if (symbol.n_zeroes)
    {
      complaint (_("Unexpected symbol continuation"));

      /* Return something which points to '\0' and hope the symbol reading
	 code does something reasonable.  */
      retval = "";
    }
  else if (symbol.n_sclass & 0x80)
    {
      retval = XCOFF_DATA (objfile)->debugsec + symbol.n_offset;
      raw_symbol += coff_data (objfile->obfd)->local_symesz;
      ++symnum;
    }
  else
    {
      complaint (_("Unexpected symbol continuation"));

      /* Return something which points to '\0' and hope the symbol reading
	 code does something reasonable.  */
      retval = "";
    }
  return retval;
}

/* Read symbols for a given partial symbol table.  */

static void
read_xcoff_symtab (struct objfile *objfile, legacy_psymtab *pst)
{
  bfd *abfd = objfile->obfd.get ();
  char *raw_auxptr;		/* Pointer to first raw aux entry for sym.  */
  struct xcoff_symfile_info *xcoff = XCOFF_DATA (objfile);
  char *strtbl = xcoff->strtbl;
  char *debugsec = xcoff->debugsec;
  const char *debugfmt = bfd_xcoff_is_xcoff64 (abfd) ? "XCOFF64" : "XCOFF";

  struct internal_syment symbol[1];
  union internal_auxent main_aux;
  struct xcoff_symbol cs[1];
  CORE_ADDR file_start_addr = 0;
  CORE_ADDR file_end_addr = 0;

  int next_file_symnum = -1;
  unsigned int max_symnum;
  int just_started = 1;
  int depth = 0;
  CORE_ADDR fcn_start_addr = 0;
  enum language pst_symtab_language;

  struct xcoff_symbol fcn_stab_saved = { 0 };

  /* fcn_cs_saved is global because process_xcoff_symbol needs it.  */
  union internal_auxent fcn_aux_saved {};
  struct context_stack *newobj;

  const char *filestring = pst->filename;	/* Name of the current file.  */

  const char *last_csect_name;	/* Last seen csect's name.  */

  this_symtab_psymtab = pst;
  this_symtab_objfile = objfile;

  /* Get the appropriate COFF "constants" related to the file we're
     handling.  */
  local_symesz = coff_data (abfd)->local_symesz;

  set_last_source_file (NULL);
  last_csect_name = 0;
  pst_symtab_language = deduce_language_from_filename (filestring);

  start_stabs ();
  start_compunit_symtab (objfile, filestring, NULL, file_start_addr,
			 pst_symtab_language);
  record_debugformat (debugfmt);
  symnum = ((struct xcoff_symloc *) pst->read_symtab_private)->first_symnum;
  max_symnum =
    symnum + ((struct xcoff_symloc *) pst->read_symtab_private)->numsyms;
  first_object_file_end = 0;

  raw_symbol = xcoff->symtbl + symnum * local_symesz;

  while (symnum < max_symnum)
    {
      QUIT;			/* make this command interruptable.  */

      /* READ_ONE_SYMBOL (symbol, cs, symname_alloced); */
      /* read one symbol into `cs' structure.  After processing the
	 whole symbol table, only string table will be kept in memory,
	 symbol table and debug section of xcoff will be freed.  Thus
	 we can mark symbols with names in string table as
	 `alloced'.  */
      {
	int ii;

	/* Swap and align the symbol into a reasonable C structure.  */
	bfd_coff_swap_sym_in (abfd, raw_symbol, symbol);

	cs->c_symnum = symnum;
	cs->c_naux = symbol->n_numaux;
	if (symbol->n_zeroes)
	  {
	    symname_alloced = 0;
	    /* We must use the original, unswapped, name here so the name field
	       pointed to by cs->c_name will persist throughout xcoffread.  If
	       we use the new field, it gets overwritten for each symbol.  */
	    cs->c_name = ((struct external_syment *) raw_symbol)->e.e_name;
	    /* If it's exactly E_SYMNMLEN characters long it isn't
	       '\0'-terminated.  */
	    if (cs->c_name[E_SYMNMLEN - 1] != '\0')
	      {
		char *p;

		p = (char *) obstack_alloc (&objfile->objfile_obstack,
					    E_SYMNMLEN + 1);
		strncpy (p, cs->c_name, E_SYMNMLEN);
		p[E_SYMNMLEN] = '\0';
		cs->c_name = p;
		symname_alloced = 1;
	      }
	  }
	else if (symbol->n_sclass & 0x80)
	  {
	    cs->c_name = debugsec + symbol->n_offset;
	    symname_alloced = 0;
	  }
	else
	  {
	    /* in string table */
	    cs->c_name = strtbl + (int) symbol->n_offset;
	    symname_alloced = 1;
	  }
	cs->c_value = symbol->n_value;
	cs->c_sclass = symbol->n_sclass;
	cs->c_secnum = symbol->n_scnum;
	cs->c_type = (unsigned) symbol->n_type;

	raw_symbol += local_symesz;
	++symnum;

	/* Save addr of first aux entry.  */
	raw_auxptr = raw_symbol;

	/* Skip all the auxents associated with this symbol.  */
	for (ii = symbol->n_numaux; ii; --ii)
	  {
	    raw_symbol += coff_data (abfd)->local_auxesz;
	    ++symnum;
	  }
      }

      /* if symbol name starts with ".$" or "$", ignore it.  */
      if (cs->c_name[0] == '$'
	  || (cs->c_name[1] == '$' && cs->c_name[0] == '.'))
	continue;

      if (cs->c_symnum == next_file_symnum && cs->c_sclass != C_FILE)
	{
	  if (get_last_source_file ())
	    {
	      pst->compunit_symtab = end_compunit_symtab (cur_src_end_addr);
	      end_stabs ();
	    }

	  start_stabs ();
	  start_compunit_symtab (objfile, "_globals_", NULL,
				 0, pst_symtab_language);
	  record_debugformat (debugfmt);
	  cur_src_end_addr = first_object_file_end;
	  /* Done with all files, everything from here on is globals.  */
	}

      if (cs->c_sclass == C_EXT || cs->c_sclass == C_HIDEXT ||
	  cs->c_sclass == C_WEAKEXT)
	{
	  /* Dealing with a symbol with a csect entry.  */

#define	CSECT(PP) ((PP)->x_csect)
#define	CSECT_LEN(PP) (CSECT(PP).x_scnlen.u64)
#define	CSECT_ALIGN(PP) (SMTYP_ALIGN(CSECT(PP).x_smtyp))
#define	CSECT_SMTYP(PP) (SMTYP_SMTYP(CSECT(PP).x_smtyp))
#define	CSECT_SCLAS(PP) (CSECT(PP).x_smclas)

	  /* Convert the auxent to something we can access.
	     XCOFF can have more than one auxiliary entries.

	     Actual functions will have two auxiliary entries, one to have the
	     function size and other to have the smtype/smclass (LD/PR).

	     c_type value of main symbol table will be set only in case of
	     C_EXT/C_HIDEEXT/C_WEAKEXT storage class symbols.
	     Bit 10 of type is set if symbol is a function, ie the value is set
	     to 32(0x20). So we need to read the first function auxiliary entry
	     which contains the size. */
	  if (cs->c_naux > 1 && ISFCN (cs->c_type))
	    {
	      /* a function entry point.  */

	      fcn_start_addr = cs->c_value;

	      /* save the function header info, which will be used
		 when `.bf' is seen.  */
	      fcn_cs_saved = *cs;

	      /* Convert the auxent to something we can access.  */
	      bfd_coff_swap_aux_in (abfd, raw_auxptr, cs->c_type, cs->c_sclass,
				    0, cs->c_naux, &fcn_aux_saved);
	      continue;
	    }
	  /* Read the csect auxiliary header, which is always the last by
	     convention. */
	  bfd_coff_swap_aux_in (abfd,
			       raw_auxptr
			       + ((coff_data (abfd)->local_symesz)
			       * (cs->c_naux - 1)),
			       cs->c_type, cs->c_sclass,
			       cs->c_naux - 1, cs->c_naux,
			       &main_aux);

	  switch (CSECT_SMTYP (&main_aux))
	    {

	    case XTY_ER:
	      /* Ignore all external references.  */
	      continue;

	    case XTY_SD:
	      /* A section description.  */
	      {
		switch (CSECT_SCLAS (&main_aux))
		  {

		  case XMC_PR:
		    {

		      /* A program csect is seen.  We have to allocate one
			 symbol table for each program csect.  Normally gdb
			 prefers one symtab for each source file.  In case
			 of AIX, one source file might include more than one
			 [PR] csect, and they don't have to be adjacent in
			 terms of the space they occupy in memory.  Thus, one
			 single source file might get fragmented in the
			 memory and gdb's file start and end address
			 approach does not work!  GCC (and I think xlc) seem
			 to put all the code in the unnamed program csect.  */

		      if (last_csect_name)
			{
			  complete_symtab (filestring, file_start_addr);
			  cur_src_end_addr = file_end_addr;
			  end_compunit_symtab (file_end_addr);
			  end_stabs ();
			  start_stabs ();
			  /* Give all csects for this source file the same
			     name.  */
			  start_compunit_symtab (objfile, filestring, NULL,
						 0, pst_symtab_language);
			  record_debugformat (debugfmt);
			}

		      /* If this is the very first csect seen,
			 basically `__start'.  */
		      if (just_started)
			{
			  first_object_file_end
			    = cs->c_value + CSECT_LEN (&main_aux);
			  just_started = 0;
			}

		      file_start_addr =
			cs->c_value + objfile->text_section_offset ();
		      file_end_addr = file_start_addr + CSECT_LEN (&main_aux);

		      if (cs->c_name && (cs->c_name[0] == '.' || cs->c_name[0] == '@'))
			last_csect_name = cs->c_name;
		    }
		    continue;

		    /* All other symbols are put into the minimal symbol
		       table only.  */

		  case XMC_RW:
		    continue;

		  case XMC_TC0:
		    continue;

		  case XMC_TC:
		    continue;

		  default:
		    /* Ignore the symbol.  */
		    continue;
		  }
	      }
	      break;

	    case XTY_LD:

	      switch (CSECT_SCLAS (&main_aux))
		{
		/* We never really come to this part as this case has been
		   handled in ISFCN check above.
		   This and other cases of XTY_LD are kept just for
		   reference. */
		case XMC_PR:
		  continue;

		case XMC_GL:
		  /* shared library function trampoline code entry point.  */
		  continue;

		case XMC_DS:
		  /* The symbols often have the same names as debug symbols for
		     functions, and confuse lookup_symbol.  */
		  continue;

		default:
		  /* xlc puts each variable in a separate csect, so we get
		     an XTY_SD for each variable.  But gcc puts several
		     variables in a csect, so that each variable only gets
		     an XTY_LD.  This will typically be XMC_RW; I suspect
		     XMC_RO and XMC_BS might be possible too.
		     These variables are put in the minimal symbol table
		     only.  */
		  continue;
		}
	      break;

	    case XTY_CM:
	      /* Common symbols are put into the minimal symbol table only.  */
	      continue;

	    default:
	      break;
	    }
	}

      switch (cs->c_sclass)
	{
	case C_FILE:

	  /* c_value field contains symnum of next .file entry in table
	     or symnum of first global after last .file.  */

	  next_file_symnum = cs->c_value;

	  /* Complete symbol table for last object file containing
	     debugging information.  */

	  /* Whether or not there was a csect in the previous file, we
	     have to call `end_stabs' and `start_stabs' to reset
	     type_vector, line_vector, etc. structures.  */

	  complete_symtab (filestring, file_start_addr);
	  cur_src_end_addr = file_end_addr;
	  end_compunit_symtab (file_end_addr);
	  end_stabs ();

	  /* XCOFF, according to the AIX 3.2 documentation, puts the
	     filename in cs->c_name.  But xlc 1.3.0.2 has decided to
	     do things the standard COFF way and put it in the auxent.
	     We use the auxent if the symbol is ".file" and an auxent
	     exists, otherwise use the symbol itself.  Simple
	     enough.  */
	  if (!strcmp (cs->c_name, ".file") && cs->c_naux > 0)
	    {
	      bfd_coff_swap_aux_in (abfd, raw_auxptr, cs->c_type, cs->c_sclass,
				    0, cs->c_naux, &main_aux);
	      filestring = coff_getfilename (&main_aux, objfile);
	    }
	  else
	    filestring = cs->c_name;

	  start_stabs ();
	  start_compunit_symtab (objfile, filestring, NULL, 0,
				 pst_symtab_language);
	  record_debugformat (debugfmt);
	  last_csect_name = 0;

	  /* reset file start and end addresses.  A compilation unit
	     with no text (only data) should have zero file
	     boundaries.  */
	  file_start_addr = file_end_addr = 0;
	  break;

	case C_FUN:
	  fcn_stab_saved = *cs;
	  break;

	case C_FCN:
	  if (strcmp (cs->c_name, ".bf") == 0)
	    {
	      CORE_ADDR off = objfile->text_section_offset ();

	      bfd_coff_swap_aux_in (abfd, raw_auxptr, cs->c_type, cs->c_sclass,
				    0, cs->c_naux, &main_aux);

	      within_function = 1;

	      newobj = push_context (0, fcn_start_addr + off);

	      newobj->name = define_symbol
		(fcn_cs_saved.c_value + off,
		 fcn_stab_saved.c_name, 0, 0, objfile);
	      if (newobj->name != NULL)
		newobj->name->set_section_index (SECT_OFF_TEXT (objfile));
	    }
	  else if (strcmp (cs->c_name, ".ef") == 0)
	    {
	      bfd_coff_swap_aux_in (abfd, raw_auxptr, cs->c_type, cs->c_sclass,
				    0, cs->c_naux, &main_aux);

	      /* The value of .ef is the address of epilogue code;
		 not useful for gdb.  */
	      /* { main_aux.x_sym.x_misc.x_lnsz.x_lnno
		 contains number of lines to '}' */

	      if (outermost_context_p ())
		{	/* We attempted to pop an empty context stack.  */
		  ef_complaint (cs->c_symnum);
		  within_function = 0;
		  break;
		}
	      struct context_stack cstk = pop_context ();
	      /* Stack must be empty now.  */
	      if (!outermost_context_p ())
		{
		  ef_complaint (cs->c_symnum);
		  within_function = 0;
		  break;
		}

	      finish_block (cstk.name, cstk.old_blocks,
			    NULL, cstk.start_addr,
			    (fcn_cs_saved.c_value
			     + fcn_aux_saved.x_sym.x_misc.x_fsize
			     + objfile->text_section_offset ()));
	      within_function = 0;
	    }
	  break;

	case C_BSTAT:
	  /* Begin static block.  */
	  {
	    struct internal_syment static_symbol;

	    read_symbol (&static_symbol, cs->c_value);
	    static_block_base = static_symbol.n_value;
	    static_block_section =
	      secnum_to_section (static_symbol.n_scnum, objfile);
	  }
	  break;

	case C_ESTAT:
	  /* End of static block.  */
	  static_block_base = 0;
	  static_block_section = -1;
	  break;

	case C_ARG:
	case C_REGPARM:
	case C_REG:
	case C_TPDEF:
	case C_STRTAG:
	case C_UNTAG:
	case C_ENTAG:
	  {
	    complaint (_("Unrecognized storage class %d."),
		       cs->c_sclass);
	  }
	  break;

	case C_LABEL:
	case C_NULL:
	  /* Ignore these.  */
	  break;

	case C_HIDEXT:
	case C_STAT:
	  break;

	case C_BINCL:
	  /* beginning of include file */
	  /* In xlc output, C_BINCL/C_EINCL pair doesn't show up in sorted
	     order.  Thus, when wee see them, we might not know enough info
	     to process them.  Thus, we'll be saving them into a table 
	     (inclTable) and postpone their processing.  */

	  record_include_begin (cs);
	  break;

	case C_EINCL:
	  /* End of include file.  */
	  /* See the comment after case C_BINCL.  */
	  record_include_end (cs);
	  break;

	case C_BLOCK:
	  if (strcmp (cs->c_name, ".bb") == 0)
	    {
	      depth++;
	      newobj = push_context (depth,
				  (cs->c_value
				   + objfile->text_section_offset ()));
	    }
	  else if (strcmp (cs->c_name, ".eb") == 0)
	    {
	      if (outermost_context_p ())
		{	/* We attempted to pop an empty context stack.  */
		  eb_complaint (cs->c_symnum);
		  break;
		}
	      struct context_stack cstk = pop_context ();
	      if (depth-- != cstk.depth)
		{
		  eb_complaint (cs->c_symnum);
		  break;
		}
	      if (*get_local_symbols () && !outermost_context_p ())
		{
		  /* Make a block for the local symbols within.  */
		  finish_block (cstk.name,
				cstk.old_blocks, NULL,
				cstk.start_addr,
				(cs->c_value
				 + objfile->text_section_offset ()));
		}
	      *get_local_symbols () = cstk.locals;
	    }
	  break;

	default:
	  process_xcoff_symbol (cs, objfile);
	  break;
	}
    }

  if (get_last_source_file ())
    {
      struct compunit_symtab *cust;

      complete_symtab (filestring, file_start_addr);
      cur_src_end_addr = file_end_addr;
      cust = end_compunit_symtab (file_end_addr);
      /* When reading symbols for the last C_FILE of the objfile, try
	 to make sure that we set pst->compunit_symtab to the symtab for the
	 file, not to the _globals_ symtab.  I'm not sure whether this
	 actually works right or when/if it comes up.  */
      if (pst->compunit_symtab == NULL)
	pst->compunit_symtab = cust;
      end_stabs ();
    }
}

#define	SYMNAME_ALLOC(NAME, ALLOCED)	\
  ((ALLOCED) ? (NAME) : obstack_strdup (&objfile->objfile_obstack, \
					(NAME)))


/* process one xcoff symbol.  */

static struct symbol *
process_xcoff_symbol (struct xcoff_symbol *cs, struct objfile *objfile)
{
  struct symbol onesymbol;
  struct symbol *sym = &onesymbol;
  struct symbol *sym2 = NULL;
  char *name, *pp;

  int sec;
  CORE_ADDR off;

  if (cs->c_secnum < 0)
    {
      /* The value is a register number, offset within a frame, etc.,
	 and does not get relocated.  */
      off = 0;
      sec = -1;
    }
  else
    {
      sec = secnum_to_section (cs->c_secnum, objfile);
      off = objfile->section_offsets[sec];
    }

  name = cs->c_name;
  if (name[0] == '.')
    ++name;

  /* default assumptions */
  sym->set_value_address (cs->c_value + off);
  sym->set_domain (VAR_DOMAIN);
  sym->set_section_index (secnum_to_section (cs->c_secnum, objfile));

  if (ISFCN (cs->c_type))
    {
      /* At this point, we don't know the type of the function.  This
	 will be patched with the type from its stab entry later on in
	 patch_block_stabs (), unless the file was compiled without -g.  */

      sym->set_linkage_name (SYMNAME_ALLOC (name, symname_alloced));
      sym->set_type (builtin_type (objfile)->nodebug_text_symbol);

      sym->set_aclass_index (LOC_BLOCK);
      sym2 = new (&objfile->objfile_obstack) symbol (*sym);

      if (cs->c_sclass == C_EXT || C_WEAKEXT)
	add_symbol_to_list (sym2, get_global_symbols ());
      else if (cs->c_sclass == C_HIDEXT || cs->c_sclass == C_STAT)
	add_symbol_to_list (sym2, get_file_symbols ());
    }
  else
    {
      /* In case we can't figure out the type, provide default.  */
      sym->set_type (builtin_type (objfile)->nodebug_data_symbol);

      switch (cs->c_sclass)
	{
#if 0
	  /* The values of functions and global symbols are now resolved
	     via the global_sym_chain in stabsread.c.  */
	case C_FUN:
	  if (fcn_cs_saved.c_sclass == C_EXT)
	    add_stab_to_list (name, &global_stabs);
	  else
	    add_stab_to_list (name, &file_stabs);
	  break;

	case C_GSYM:
	  add_stab_to_list (name, &global_stabs);
	  break;
#endif

	case C_BCOMM:
	  common_block_start (cs->c_name, objfile);
	  break;

	case C_ECOMM:
	  common_block_end (objfile);
	  break;

	default:
	  complaint (_("Unexpected storage class: %d"),
		     cs->c_sclass);
	  [[fallthrough]];

	case C_DECL:
	case C_PSYM:
	case C_RPSYM:
	case C_ECOML:
	case C_LSYM:
	case C_RSYM:
	case C_GSYM:

	  {
	    sym = define_symbol (cs->c_value + off, cs->c_name, 0, 0, objfile);
	    if (sym != NULL)
	      {
		sym->set_section_index (sec);
	      }
	    return sym;
	  }

	case C_STSYM:

	  /* For xlc (not GCC), the 'V' symbol descriptor is used for
	     all statics and we need to distinguish file-scope versus
	     function-scope using within_function.  We do this by
	     changing the string we pass to define_symbol to use 'S'
	     where we need to, which is not necessarily super-clean,
	     but seems workable enough.  */

	  if (*name == ':')
	    return NULL;

	  pp = strchr (name, ':');
	  if (pp == NULL)
	    return NULL;

	  ++pp;
	  if (*pp == 'V' && !within_function)
	    *pp = 'S';
	  sym = define_symbol ((cs->c_value
				+ objfile->section_offsets[static_block_section]),
			       cs->c_name, 0, 0, objfile);
	  if (sym != NULL)
	    {
	      sym->set_value_address
		(sym->value_address () + static_block_base);
	      sym->set_section_index (static_block_section);
	    }
	  return sym;

	}
    }
  return sym2;
}

/* Extract the file name from the aux entry of a C_FILE symbol.
   Result is in static storage and is only good for temporary use.  */

static char *
coff_getfilename (union internal_auxent *aux_entry, struct objfile *objfile)
{
  static char buffer[BUFSIZ];

  if (aux_entry->x_file.x_n.x_n.x_zeroes == 0)
    strcpy (buffer, (XCOFF_DATA (objfile)->strtbl
		     + aux_entry->x_file.x_n.x_n.x_offset));
  else
    {
      size_t x_fname_len = sizeof (aux_entry->x_file.x_n.x_fname);
      strncpy (buffer, aux_entry->x_file.x_n.x_fname, x_fname_len);
      buffer[x_fname_len] = '\0';
    }
  return (buffer);
}

/* Set *SYMBOL to symbol number symno in symtbl.  */
static void
read_symbol (struct internal_syment *symbol, int symno)
{
  struct xcoff_symfile_info *xcoff = XCOFF_DATA (this_symtab_objfile);
  int nsyms = xcoff->symtbl_num_syms;
  char *stbl = xcoff->symtbl;

  if (symno < 0 || symno >= nsyms)
    {
      complaint (_("Invalid symbol offset"));
      symbol->n_value = 0;
      symbol->n_scnum = -1;
      return;
    }
  bfd_coff_swap_sym_in (this_symtab_objfile->obfd.get (),
			stbl + (symno * local_symesz),
			symbol);
}

/* Get value corresponding to symbol number symno in symtbl.  */

static CORE_ADDR
read_symbol_nvalue (int symno)
{
  struct internal_syment symbol[1];

  read_symbol (symbol, symno);
  return symbol->n_value;
}


/* Find the address of the function corresponding to symno, where
   symno is the symbol pointed to by the linetable.  */

static int
read_symbol_lineno (int symno)
{
  struct objfile *objfile = this_symtab_objfile;
  int xcoff64 = bfd_xcoff_is_xcoff64 (objfile->obfd);

  struct xcoff_symfile_info *info = XCOFF_DATA (objfile);
  int nsyms = info->symtbl_num_syms;
  char *stbl = info->symtbl;
  char *strtbl = info->strtbl;

  struct internal_syment symbol[1];
  union internal_auxent main_aux[1];

  if (symno < 0)
    {
      bf_notfound_complaint ();
      return 0;
    }

  /* Note that just searching for a short distance (e.g. 50 symbols)
     is not enough, at least in the following case.

     .extern foo
     [many .stabx entries]
     [a few functions, referring to foo]
     .globl foo
     .bf

     What happens here is that the assembler moves the .stabx entries
     to right before the ".bf" for foo, but the symbol for "foo" is before
     all the stabx entries.  See PR gdb/2222.  */

  /* Maintaining a table of .bf entries might be preferable to this search.
     If I understand things correctly it would need to be done only for
     the duration of a single psymtab to symtab conversion.  */
  while (symno < nsyms)
    {
      bfd_coff_swap_sym_in (symfile_bfd,
			    stbl + (symno * local_symesz), symbol);
      if (symbol->n_sclass == C_FCN)
	{
	  char *name = xcoff64 ? strtbl + symbol->n_offset : symbol->n_name;

	  if (strcmp (name, ".bf") == 0)
	    goto gotit;
	}
      symno += symbol->n_numaux + 1;
    }

  bf_notfound_complaint ();
  return 0;

gotit:
  /* Take aux entry and return its lineno.  */
  symno++;
  bfd_coff_swap_aux_in (objfile->obfd.get (), stbl + symno * local_symesz,
			symbol->n_type, symbol->n_sclass,
			0, symbol->n_numaux, main_aux);

  return main_aux->x_sym.x_misc.x_lnsz.x_lnno;
}

/* Support for line number handling.  */

/* This function is called for every section; it finds the outer limits
 * of the line table (minimum and maximum file offset) so that the
 * mainline code can read the whole thing for efficiency.
 */
static void
find_linenos (struct bfd *abfd, struct bfd_section *asect, void *vpinfo)
{
  struct xcoff_symfile_info *info;
  int size, count;
  file_ptr offset, maxoff;

  count = asect->lineno_count;

  if (strcmp (asect->name, ".text") != 0 || count == 0)
    return;

  size = count * coff_data (abfd)->local_linesz;
  info = (struct xcoff_symfile_info *) vpinfo;
  offset = asect->line_filepos;
  maxoff = offset + size;

  if (offset < info->min_lineno_offset || info->min_lineno_offset == 0)
    info->min_lineno_offset = offset;

  if (maxoff > info->max_lineno_offset)
    info->max_lineno_offset = maxoff;
}

static void
xcoff_expand_psymtab (legacy_psymtab *pst, struct objfile *objfile)
{
  gdb_assert (!pst->readin);

  /* Read in all partial symtabs on which this one is dependent.  */
  pst->expand_dependencies (objfile);

  if (((struct xcoff_symloc *) pst->read_symtab_private)->numsyms != 0)
    {
      /* Init stuff necessary for reading in symbols.  */
      stabsread_init ();

      scoped_free_pendings free_pending;
      read_xcoff_symtab (objfile, pst);
    }

  pst->readin = true;
}

/* Read in all of the symbols for a given psymtab for real.
   Be verbose about it if the user wants that.  SELF is not NULL.  */

static void
xcoff_read_symtab (legacy_psymtab *self, struct objfile *objfile)
{
  gdb_assert (!self->readin);

  if (((struct xcoff_symloc *) self->read_symtab_private)->numsyms != 0
      || self->number_of_dependencies)
    {
      next_symbol_text_func = xcoff_next_symbol_text;

      self->expand_psymtab (objfile);

      /* Match with global symbols.  This only needs to be done once,
	 after all of the symtabs and dependencies have been read in.   */
      scan_file_globals (objfile);
    }
}

static void
xcoff_new_init (struct objfile *objfile)
{
  stabsread_new_init ();
}

/* Do initialization in preparation for reading symbols from OBJFILE.

   We will only be called if this is an XCOFF or XCOFF-like file.
   BFD handles figuring out the format of the file, and code in symfile.c
   uses BFD's determination to vector to us.  */

static void
xcoff_symfile_init (struct objfile *objfile)
{
  /* Allocate struct to keep track of the symfile.  */
  xcoff_objfile_data_key.emplace (objfile);
}

/* Perform any local cleanups required when we are done with a particular
   objfile.  I.E, we are in the process of discarding all symbol information
   for an objfile, freeing up all memory held for it, and unlinking the
   objfile struct from the global list of known objfiles.  */

static void
xcoff_symfile_finish (struct objfile *objfile)
{
  /* Start with a fresh include table for the next objfile.  */
  if (inclTable)
    {
      xfree (inclTable);
      inclTable = NULL;
      delete main_subfile;
    }
  inclIndx = inclLength = inclDepth = 0;
}


static void
init_stringtab (bfd *abfd, file_ptr offset, struct objfile *objfile)
{
  long length;
  int val;
  unsigned char lengthbuf[4];
  char *strtbl;
  struct xcoff_symfile_info *xcoff = XCOFF_DATA (objfile);

  xcoff->strtbl = NULL;

  if (bfd_seek (abfd, offset, SEEK_SET) < 0)
    error (_("cannot seek to string table in %s: %s"),
	   bfd_get_filename (abfd), bfd_errmsg (bfd_get_error ()));

  val = bfd_read ((char *) lengthbuf, sizeof lengthbuf, abfd);
  length = bfd_h_get_32 (abfd, lengthbuf);

  /* If no string table is needed, then the file may end immediately
     after the symbols.  Just return with `strtbl' set to NULL.  */

  if (val != sizeof lengthbuf || length < sizeof lengthbuf)
    return;

  /* Allocate string table from objfile_obstack.  We will need this table
     as long as we have its symbol table around.  */

  strtbl = (char *) obstack_alloc (&objfile->objfile_obstack, length);
  xcoff->strtbl = strtbl;

  /* Copy length buffer, the first byte is usually zero and is
     used for stabs with a name length of zero.  */
  memcpy (strtbl, lengthbuf, sizeof lengthbuf);
  if (length == sizeof lengthbuf)
    return;

  val = bfd_read (strtbl + sizeof lengthbuf, length - sizeof lengthbuf, abfd);

  if (val != length - sizeof lengthbuf)
    error (_("cannot read string table from %s: %s"),
	   bfd_get_filename (abfd), bfd_errmsg (bfd_get_error ()));
  if (strtbl[length - 1] != '\0')
    error (_("bad symbol file: string table "
	     "does not end with null character"));

  return;
}

/* If we have not yet seen a function for this psymtab, this is 0.  If we
   have seen one, it is the offset in the line numbers of the line numbers
   for the psymtab.  */
static unsigned int first_fun_line_offset;

/* Allocate and partially fill a partial symtab.  It will be
   completely filled at the end of the symbol list.

   SYMFILE_NAME is the name of the symbol-file we are reading from, and ADDR
   is the address relative to which its symbols are (incremental) or 0
   (normal).  */

static legacy_psymtab *
xcoff_start_psymtab (psymtab_storage *partial_symtabs,
		     struct objfile *objfile,
		     const char *filename, int first_symnum)
{
  /* We fill in textlow later.  */
  legacy_psymtab *result = new legacy_psymtab (filename, partial_symtabs,
					       objfile->per_bfd,
					       unrelocated_addr (0));

  result->read_symtab_private =
    XOBNEW (&objfile->objfile_obstack, struct xcoff_symloc);
  ((struct xcoff_symloc *) result->read_symtab_private)->first_symnum = first_symnum;
  result->legacy_read_symtab = xcoff_read_symtab;
  result->legacy_expand_psymtab = xcoff_expand_psymtab;

  /* Deduce the source language from the filename for this psymtab.  */
  psymtab_language = deduce_language_from_filename (filename);

  return result;
}

/* Close off the current usage of PST.
   Returns PST, or NULL if the partial symtab was empty and thrown away.

   CAPPING_SYMBOL_NUMBER is the end of pst (exclusive).

   INCLUDE_LIST, NUM_INCLUDES, DEPENDENCY_LIST, and NUMBER_DEPENDENCIES
   are the information for includes and dependencies.  */

static legacy_psymtab *
xcoff_end_psymtab (struct objfile *objfile, psymtab_storage *partial_symtabs,
		   legacy_psymtab *pst,
		   const char **include_list, int num_includes,
		   int capping_symbol_number,
		   legacy_psymtab **dependency_list,
		   int number_dependencies, int textlow_not_set)
{
  int i;

  if (capping_symbol_number != -1)
    ((struct xcoff_symloc *) pst->read_symtab_private)->numsyms =
      capping_symbol_number
      - ((struct xcoff_symloc *) pst->read_symtab_private)->first_symnum;
  ((struct xcoff_symloc *) pst->read_symtab_private)->lineno_off =
    first_fun_line_offset;
  first_fun_line_offset = 0;

  pst->end ();

  pst->number_of_dependencies = number_dependencies;
  if (number_dependencies)
    {
      pst->dependencies
	= partial_symtabs->allocate_dependencies (number_dependencies);
      memcpy (pst->dependencies, dependency_list,
	      number_dependencies * sizeof (legacy_psymtab *));
    }
  else
    pst->dependencies = 0;

  for (i = 0; i < num_includes; i++)
    {
      legacy_psymtab *subpst =
	new legacy_psymtab (include_list[i], partial_symtabs, objfile->per_bfd);

      subpst->read_symtab_private = XOBNEW (&objfile->objfile_obstack, xcoff_symloc);
      ((struct xcoff_symloc *) subpst->read_symtab_private)->first_symnum = 0;
      ((struct xcoff_symloc *) subpst->read_symtab_private)->numsyms = 0;

      /* We could save slight bits of space by only making one of these,
	 shared by the entire set of include files.  FIXME-someday.  */
      subpst->dependencies =
	partial_symtabs->allocate_dependencies (1);
      subpst->dependencies[0] = pst;
      subpst->number_of_dependencies = 1;

      subpst->legacy_read_symtab = pst->legacy_read_symtab;
      subpst->legacy_expand_psymtab = pst->legacy_expand_psymtab;
    }

  if (num_includes == 0
      && number_dependencies == 0
      && pst->empty ())
    {
      /* Throw away this psymtab, it's empty.  */
      /* Empty psymtabs happen as a result of header files which don't have
	 any symbols in them.  There can be a lot of them.  */

      partial_symtabs->discard_psymtab (pst);

      /* Indicate that psymtab was thrown away.  */
      pst = NULL;
    }
  return pst;
}

/* Swap raw symbol at *RAW and put the name in *NAME, the symbol in
   *SYMBOL, the first auxent in *AUX.  Advance *RAW and *SYMNUMP over
   the symbol and its auxents.  */

static void
swap_sym (struct internal_syment *symbol, union internal_auxent *aux,
	  const char **name, char **raw, unsigned int *symnump,
	  struct objfile *objfile)
{
  bfd_coff_swap_sym_in (objfile->obfd.get (), *raw, symbol);
  if (symbol->n_zeroes)
    {
      /* If it's exactly E_SYMNMLEN characters long it isn't
	 '\0'-terminated.  */
      if (symbol->n_name[E_SYMNMLEN - 1] != '\0')
	{
	  /* FIXME: wastes memory for symbols which we don't end up putting
	     into the minimal symbols.  */
	  char *p;

	  p = (char *) obstack_alloc (&objfile->objfile_obstack,
				      E_SYMNMLEN + 1);
	  strncpy (p, symbol->n_name, E_SYMNMLEN);
	  p[E_SYMNMLEN] = '\0';
	  *name = p;
	}
      else
	/* Point to the unswapped name as that persists as long as the
	   objfile does.  */
	*name = ((struct external_syment *) *raw)->e.e_name;
    }
  else if (symbol->n_sclass & 0x80)
    {
      *name = XCOFF_DATA (objfile)->debugsec + symbol->n_offset;
    }
  else
    {
      *name = XCOFF_DATA (objfile)->strtbl + symbol->n_offset;
    }
  ++*symnump;
  *raw += coff_data (objfile->obfd)->local_symesz;
  if (symbol->n_numaux > 0)
    {
      bfd_coff_swap_aux_in (objfile->obfd.get (), *raw, symbol->n_type,
			    symbol->n_sclass, 0, symbol->n_numaux, aux);

      *symnump += symbol->n_numaux;
      *raw += coff_data (objfile->obfd)->local_symesz * symbol->n_numaux;
    }
}

static void
function_outside_compilation_unit_complaint (const char *arg1)
{
  complaint (_("function `%s' appears to be defined "
	       "outside of all compilation units"),
	     arg1);
}

static void
scan_xcoff_symtab (minimal_symbol_reader &reader,
		   psymtab_storage *partial_symtabs,
		   struct objfile *objfile)
{
  CORE_ADDR toc_offset = 0;	/* toc offset value in data section.  */
  const char *filestring = NULL;

  const char *namestring;
  bfd *abfd;
  asection *bfd_sect;
  unsigned int nsyms;

  /* Current partial symtab */
  legacy_psymtab *pst;

  /* List of current psymtab's include files.  */
  const char **psymtab_include_list;
  int includes_allocated;
  int includes_used;

  /* Index within current psymtab dependency list.  */
  legacy_psymtab **dependency_list;
  int dependencies_used, dependencies_allocated;

  char *sraw_symbol;
  struct internal_syment symbol;
  union internal_auxent main_aux[5];
  unsigned int ssymnum;

  const char *last_csect_name = NULL; /* Last seen csect's name and value.  */
  unrelocated_addr last_csect_val = unrelocated_addr (0);
  int last_csect_sec = 0;
  int misc_func_recorded = 0;	/* true if any misc. function.  */
  int textlow_not_set = 1;

  pst = (legacy_psymtab *) 0;

  includes_allocated = 30;
  includes_used = 0;
  psymtab_include_list = (const char **) alloca (includes_allocated *
						 sizeof (const char *));

  dependencies_allocated = 30;
  dependencies_used = 0;
  dependency_list =
    (legacy_psymtab **) alloca (dependencies_allocated *
				       sizeof (legacy_psymtab *));

  set_last_source_file (NULL);

  abfd = objfile->obfd.get ();
  next_symbol_text_func = xcoff_next_symbol_text;

  sraw_symbol = XCOFF_DATA (objfile)->symtbl;
  nsyms = XCOFF_DATA (objfile)->symtbl_num_syms;
  ssymnum = 0;
  while (ssymnum < nsyms)
    {
      int sclass;

      QUIT;

      bfd_coff_swap_sym_in (abfd, sraw_symbol, &symbol);
      sclass = symbol.n_sclass;

      switch (sclass)
	{
	case C_EXT:
	case C_HIDEXT:
	case C_WEAKEXT:
	  {
	    /* The CSECT auxent--always the last auxent.  */
	    union internal_auxent csect_aux;
	    unsigned int symnum_before = ssymnum;

	    swap_sym (&symbol, &main_aux[0], &namestring, &sraw_symbol,
		      &ssymnum, objfile);
	    if (symbol.n_numaux > 1)
	      {
		bfd_coff_swap_aux_in
		  (objfile->obfd.get (),
		   sraw_symbol - coff_data (abfd)->local_symesz,
		   symbol.n_type,
		   symbol.n_sclass,
		   symbol.n_numaux - 1,
		   symbol.n_numaux,
		   &csect_aux);
	      }
	    else
	      csect_aux = main_aux[0];

	    /* If symbol name starts with ".$" or "$", ignore it.  */
	    if (namestring[0] == '$'
		|| (namestring[0] == '.' && namestring[1] == '$'))
	      break;

	    switch (csect_aux.x_csect.x_smtyp & 0x7)
	      {
	      case XTY_SD:
		switch (csect_aux.x_csect.x_smclas)
		  {
		  case XMC_PR:
		    if (last_csect_name)
		      {
			/* If no misc. function recorded in the last
			   seen csect, enter it as a function.  This
			   will take care of functions like strcmp()
			   compiled by xlc.  */

			if (!misc_func_recorded)
			  {
			    record_minimal_symbol
			      (reader, last_csect_name, last_csect_val,
			       mst_text, last_csect_sec, objfile);
			    misc_func_recorded = 1;
			  }

			if (pst != NULL)
			  {
			    /* We have to allocate one psymtab for
			       each program csect, because their text
			       sections need not be adjacent.  */
			    xcoff_end_psymtab
			      (objfile, partial_symtabs, pst, psymtab_include_list,
			       includes_used, symnum_before, dependency_list,
			       dependencies_used, textlow_not_set);
			    includes_used = 0;
			    dependencies_used = 0;
			    /* Give all psymtabs for this source file the same
			       name.  */
			    pst = xcoff_start_psymtab
			      (partial_symtabs, objfile,
			       filestring,
			       symnum_before);
			  }
		      }
		    /* Activate the misc_func_recorded mechanism for
		       compiler- and linker-generated CSECTs like ".strcmp"
		       and "@FIX1".  */ 
		    if (namestring && (namestring[0] == '.'
				       || namestring[0] == '@'))
		      {
			last_csect_name = namestring;
			last_csect_val = unrelocated_addr (symbol.n_value);
			last_csect_sec = symbol.n_scnum;
		      }
		    if (pst != NULL)
		      {
			unrelocated_addr highval
			  = unrelocated_addr (symbol.n_value
					      + CSECT_LEN (&csect_aux));

			if (highval > pst->unrelocated_text_high ())
			  pst->set_text_high (highval);
			unrelocated_addr loval
			  = unrelocated_addr (symbol.n_value);
			if (!pst->text_low_valid
			    || loval < pst->unrelocated_text_low ())
			  pst->set_text_low (loval);
		      }
		    misc_func_recorded = 0;
		    break;

		  case XMC_RW:
		  case XMC_TD:
		    /* Data variables are recorded in the minimal symbol
		       table, except for section symbols.  */
		    if (*namestring != '.')
		      record_minimal_symbol
			(reader, namestring, unrelocated_addr (symbol.n_value),
			 sclass == C_HIDEXT ? mst_file_data : mst_data,
			 symbol.n_scnum, objfile);
		    break;

		  case XMC_TC0:
		    if (toc_offset)
		      warning (_("More than one XMC_TC0 symbol found."));
		    toc_offset = symbol.n_value;

		    /* Make TOC offset relative to start address of
		       section.  */
		    bfd_sect = secnum_to_bfd_section (symbol.n_scnum, objfile);
		    if (bfd_sect)
		      toc_offset -= bfd_section_vma (bfd_sect);
		    break;

		  case XMC_TC:
		    /* These symbols tell us where the TOC entry for a
		       variable is, not the variable itself.  */
		    break;

		  default:
		    break;
		  }
		break;

	      case XTY_LD:
		switch (csect_aux.x_csect.x_smclas)
		  {
		  case XMC_PR:
		    /* A function entry point.  */

		    if (first_fun_line_offset == 0 && symbol.n_numaux > 1)
		      first_fun_line_offset =
			main_aux[0].x_sym.x_fcnary.x_fcn.x_lnnoptr;

		    record_minimal_symbol
		      (reader, namestring, unrelocated_addr (symbol.n_value),
		       sclass == C_HIDEXT ? mst_file_text : mst_text,
		       symbol.n_scnum, objfile);
		    misc_func_recorded = 1;
		    break;

		  case XMC_GL:
		    /* shared library function trampoline code entry
		       point.  */

		    /* record trampoline code entries as
		       mst_solib_trampoline symbol.  When we lookup mst
		       symbols, we will choose mst_text over
		       mst_solib_trampoline.  */
		    record_minimal_symbol
		      (reader, namestring, unrelocated_addr (symbol.n_value),
		       mst_solib_trampoline, symbol.n_scnum, objfile);
		    misc_func_recorded = 1;
		    break;

		  case XMC_DS:
		    /* The symbols often have the same names as
		       debug symbols for functions, and confuse
		       lookup_symbol.  */
		    break;

		  default:

		    /* xlc puts each variable in a separate csect,
		       so we get an XTY_SD for each variable.  But
		       gcc puts several variables in a csect, so
		       that each variable only gets an XTY_LD.  We
		       still need to record them.  This will
		       typically be XMC_RW; I suspect XMC_RO and
		       XMC_BS might be possible too.  */
		    if (*namestring != '.')
		      record_minimal_symbol
			(reader, namestring, unrelocated_addr (symbol.n_value),
			 sclass == C_HIDEXT ? mst_file_data : mst_data,
			 symbol.n_scnum, objfile);
		    break;
		  }
		break;

	      case XTY_CM:
		switch (csect_aux.x_csect.x_smclas)
		  {
		  case XMC_RW:
		  case XMC_BS:
		    /* Common variables are recorded in the minimal symbol
		       table, except for section symbols.  */
		    if (*namestring != '.')
		      record_minimal_symbol
			(reader, namestring, unrelocated_addr (symbol.n_value),
			 sclass == C_HIDEXT ? mst_file_bss : mst_bss,
			 symbol.n_scnum, objfile);
		    break;
		  }
		break;

	      default:
		break;
	      }
	  }
	  break;
	case C_FILE:
	  {
	    unsigned int symnum_before;

	    symnum_before = ssymnum;
	    swap_sym (&symbol, &main_aux[0], &namestring, &sraw_symbol,
		      &ssymnum, objfile);

	    /* See if the last csect needs to be recorded.  */

	    if (last_csect_name && !misc_func_recorded)
	      {
		/* If no misc. function recorded in the last seen csect, enter
		   it as a function.  This will take care of functions like
		   strcmp() compiled by xlc.  */

		record_minimal_symbol (reader, last_csect_name, last_csect_val,
				       mst_text, last_csect_sec, objfile);
		misc_func_recorded = 1;
	      }

	    if (pst)
	      {
		xcoff_end_psymtab (objfile, partial_symtabs,
				   pst, psymtab_include_list,
				   includes_used, symnum_before,
				   dependency_list, dependencies_used,
				   textlow_not_set);
		includes_used = 0;
		dependencies_used = 0;
	      }
	    first_fun_line_offset = 0;

	    /* XCOFF, according to the AIX 3.2 documentation, puts the
	       filename in cs->c_name.  But xlc 1.3.0.2 has decided to
	       do things the standard COFF way and put it in the auxent.
	       We use the auxent if the symbol is ".file" and an auxent
	       exists, otherwise use the symbol itself.  */
	    if (!strcmp (namestring, ".file") && symbol.n_numaux > 0)
	      {
		filestring = coff_getfilename (&main_aux[0], objfile);
	      }
	    else
	      filestring = namestring;

	    pst = xcoff_start_psymtab (partial_symtabs, objfile,
				       filestring,
				       symnum_before);
	    last_csect_name = NULL;
	  }
	  break;

	default:
	  {
	    complaint (_("Storage class %d not recognized during scan"),
		       sclass);
	  }
	  [[fallthrough]];

	case C_FCN:
	  /* C_FCN is .bf and .ef symbols.  I think it is sufficient
	     to handle only the C_FUN and C_EXT.  */

	case C_BSTAT:
	case C_ESTAT:
	case C_ARG:
	case C_REGPARM:
	case C_REG:
	case C_TPDEF:
	case C_STRTAG:
	case C_UNTAG:
	case C_ENTAG:
	case C_LABEL:
	case C_NULL:

	  /* C_EINCL means we are switching back to the main file.  But there
	     is no reason to care; the only thing we want to know about
	     includes is the names of all the included (.h) files.  */
	case C_EINCL:

	case C_BLOCK:

	  /* I don't think C_STAT is used in xcoff; C_HIDEXT appears to be
	     used instead.  */
	case C_STAT:

	  /* I don't think the name of the common block (as opposed to the
	     variables within it) is something which is user visible
	     currently.  */
	case C_BCOMM:
	case C_ECOMM:

	case C_PSYM:
	case C_RPSYM:

	  /* I think we can ignore C_LSYM; types on xcoff seem to use C_DECL
	     so C_LSYM would appear to be only for locals.  */
	case C_LSYM:

	case C_AUTO:
	case C_RSYM:
	  {
	    /* We probably could save a few instructions by assuming that
	       C_LSYM, C_PSYM, etc., never have auxents.  */
	    int naux1 = symbol.n_numaux + 1;

	    ssymnum += naux1;
	    sraw_symbol += bfd_coff_symesz (abfd) * naux1;
	  }
	  break;

	case C_BINCL:
	  {
	    /* Mark down an include file in the current psymtab.  */
	    enum language tmp_language;

	    swap_sym (&symbol, &main_aux[0], &namestring, &sraw_symbol,
		      &ssymnum, objfile);

	    tmp_language = deduce_language_from_filename (namestring);

	    /* Only change the psymtab's language if we've learned
	       something useful (eg. tmp_language is not language_unknown).
	       In addition, to match what start_subfile does, never change
	       from C++ to C.  */
	    if (tmp_language != language_unknown
		&& (tmp_language != language_c
		    || psymtab_language != language_cplus))
	      psymtab_language = tmp_language;

	    /* In C++, one may expect the same filename to come round many
	       times, when code is coming alternately from the main file
	       and from inline functions in other files.  So I check to see
	       if this is a file we've seen before -- either the main
	       source file, or a previously included file.

	       This seems to be a lot of time to be spending on N_SOL, but
	       things like "break c-exp.y:435" need to work (I
	       suppose the psymtab_include_list could be hashed or put
	       in a binary tree, if profiling shows this is a major hog).  */
	    if (pst && strcmp (namestring, pst->filename) == 0)
	      continue;

	    {
	      int i;

	      for (i = 0; i < includes_used; i++)
		if (strcmp (namestring, psymtab_include_list[i]) == 0)
		  {
		    i = -1;
		    break;
		  }
	      if (i == -1)
		continue;
	    }
	    psymtab_include_list[includes_used++] = namestring;
	    if (includes_used >= includes_allocated)
	      {
		const char **orig = psymtab_include_list;

		psymtab_include_list = (const char **)
		  alloca ((includes_allocated *= 2) *
			  sizeof (const char *));
		memcpy (psymtab_include_list, orig,
			includes_used * sizeof (const char *));
	      }
	    continue;
	  }
	case C_FUN:
	  /* The value of the C_FUN is not the address of the function (it
	     appears to be the address before linking), but as long as it
	     is smaller than the actual address, then find_pc_partial_function
	     will use the minimal symbols instead.  I hope.  */

	case C_GSYM:
	case C_ECOML:
	case C_DECL:
	case C_STSYM:
	  {
	    const char *p;

	    swap_sym (&symbol, &main_aux[0], &namestring, &sraw_symbol,
		      &ssymnum, objfile);

	    p = strchr (namestring, ':');
	    if (!p)
	      continue;			/* Not a debugging symbol.   */

	    /* Main processing section for debugging symbols which
	       the initial read through the symbol tables needs to worry
	       about.  If we reach this point, the symbol which we are
	       considering is definitely one we are interested in.
	       p must also contain the (valid) index into the namestring
	       which indicates the debugging type symbol.  */

	    switch (p[1])
	      {
	      case 'S':
		pst->add_psymbol (std::string_view (namestring,
						    p - namestring),
				  true, VAR_DOMAIN, LOC_STATIC,
				  SECT_OFF_DATA (objfile),
				  psymbol_placement::STATIC,
				  unrelocated_addr (symbol.n_value),
				  psymtab_language,
				  partial_symtabs, objfile);
		continue;

	      case 'G':
		/* The addresses in these entries are reported to be
		   wrong.  See the code that reads 'G's for symtabs.  */
		pst->add_psymbol (std::string_view (namestring,
						    p - namestring),
				  true, VAR_DOMAIN, LOC_STATIC,
				  SECT_OFF_DATA (objfile),
				  psymbol_placement::GLOBAL,
				  unrelocated_addr (symbol.n_value),
				  psymtab_language,
				  partial_symtabs, objfile);
		continue;

	      case 'T':
		/* When a 'T' entry is defining an anonymous enum, it
		   may have a name which is the empty string, or a
		   single space.  Since they're not really defining a
		   symbol, those shouldn't go in the partial symbol
		   table.  We do pick up the elements of such enums at
		   'check_enum:', below.  */
		if (p >= namestring + 2
		    || (p == namestring + 1
			&& namestring[0] != ' '))
		  {
		    pst->add_psymbol (std::string_view (namestring,
							p - namestring),
				      true, STRUCT_DOMAIN, LOC_TYPEDEF, -1,
				      psymbol_placement::STATIC,
				      unrelocated_addr (0),
				      psymtab_language,
				      partial_symtabs, objfile);
		    if (p[2] == 't')
		      {
			/* Also a typedef with the same name.  */
			pst->add_psymbol (std::string_view (namestring,
							    p - namestring),
					  true, VAR_DOMAIN, LOC_TYPEDEF, -1,
					  psymbol_placement::STATIC,
					  unrelocated_addr (0),
					  psymtab_language,
					  partial_symtabs, objfile);
			p += 1;
		      }
		  }
		goto check_enum;

	      case 't':
		if (p != namestring)	/* a name is there, not just :T...  */
		  {
		    pst->add_psymbol (std::string_view (namestring,
							p - namestring),
				      true, VAR_DOMAIN, LOC_TYPEDEF, -1,
				      psymbol_placement::STATIC,
				      unrelocated_addr (0),
				      psymtab_language,
				      partial_symtabs, objfile);
		  }
	      check_enum:
		/* If this is an enumerated type, we need to
		   add all the enum constants to the partial symbol
		   table.  This does not cover enums without names, e.g.
		   "enum {a, b} c;" in C, but fortunately those are
		   rare.  There is no way for GDB to find those from the
		   enum type without spending too much time on it.  Thus
		   to solve this problem, the compiler needs to put out the
		   enum in a nameless type.  GCC2 does this.  */

		/* We are looking for something of the form
		   <name> ":" ("t" | "T") [<number> "="] "e"
		   {<constant> ":" <value> ","} ";".  */

		/* Skip over the colon and the 't' or 'T'.  */
		p += 2;
		/* This type may be given a number.  Also, numbers can come
		   in pairs like (0,26).  Skip over it.  */
		while ((*p >= '0' && *p <= '9')
		       || *p == '(' || *p == ',' || *p == ')'
		       || *p == '=')
		  p++;

		if (*p++ == 'e')
		  {
		    /* The aix4 compiler emits extra crud before the
		       members.  */
		    if (*p == '-')
		      {
			/* Skip over the type (?).  */
			while (*p != ':')
			  p++;

			/* Skip over the colon.  */
			p++;
		      }

		    /* We have found an enumerated type.  */
		    /* According to comments in read_enum_type
		       a comma could end it instead of a semicolon.
		       I don't know where that happens.
		       Accept either.  */
		    while (*p && *p != ';' && *p != ',')
		      {
			const char *q;

			/* Check for and handle cretinous dbx symbol name
			   continuation!  */
			if (*p == '\\' || (*p == '?' && p[1] == '\0'))
			  p = next_symbol_text (objfile);

			/* Point to the character after the name
			   of the enum constant.  */
			for (q = p; *q && *q != ':'; q++)
			  ;
			/* Note that the value doesn't matter for
			   enum constants in psymtabs, just in symtabs.  */
			pst->add_psymbol (std::string_view (p, q - p), true,
					  VAR_DOMAIN, LOC_CONST, -1,
					  psymbol_placement::STATIC,
					  unrelocated_addr (0),
					  psymtab_language,
					  partial_symtabs, objfile);
			/* Point past the name.  */
			p = q;
			/* Skip over the value.  */
			while (*p && *p != ',')
			  p++;
			/* Advance past the comma.  */
			if (*p)
			  p++;
		      }
		  }
		continue;

	      case 'c':
		/* Constant, e.g. from "const" in Pascal.  */
		pst->add_psymbol (std::string_view (namestring,
						    p - namestring),
				  true, VAR_DOMAIN, LOC_CONST, -1,
				  psymbol_placement::STATIC,
				  unrelocated_addr (0),
				  psymtab_language,
				  partial_symtabs, objfile);
		continue;

	      case 'f':
		if (! pst)
		  {
		    std::string name (namestring, (p - namestring));
		    function_outside_compilation_unit_complaint (name.c_str ());
		  }
		pst->add_psymbol (std::string_view (namestring,
						    p - namestring),
				  true, VAR_DOMAIN, LOC_BLOCK,
				  SECT_OFF_TEXT (objfile),
				  psymbol_placement::STATIC,
				  unrelocated_addr (symbol.n_value),
				  psymtab_language,
				  partial_symtabs, objfile);
		continue;

		/* Global functions were ignored here, but now they
		   are put into the global psymtab like one would expect.
		   They're also in the minimal symbol table.  */
	      case 'F':
		if (! pst)
		  {
		    std::string name (namestring, (p - namestring));
		    function_outside_compilation_unit_complaint (name.c_str ());
		  }

		/* We need only the minimal symbols for these
		   loader-generated definitions.  Keeping the global
		   symbols leads to "in psymbols but not in symbols"
		   errors.  */
		if (startswith (namestring, "@FIX"))
		  continue;

		pst->add_psymbol (std::string_view (namestring,
						    p - namestring),
				  true, VAR_DOMAIN, LOC_BLOCK,
				  SECT_OFF_TEXT (objfile),
				  psymbol_placement::GLOBAL,
				  unrelocated_addr (symbol.n_value),
				  psymtab_language,
				  partial_symtabs, objfile);
		continue;

		/* Two things show up here (hopefully); static symbols of
		   local scope (static used inside braces) or extensions
		   of structure symbols.  We can ignore both.  */
	      case 'V':
	      case '(':
	      case '0':
	      case '1':
	      case '2':
	      case '3':
	      case '4':
	      case '5':
	      case '6':
	      case '7':
	      case '8':
	      case '9':
	      case '-':
	      case '#':		/* For symbol identification (used in
				   live ranges).  */
		continue;

	      case ':':
		/* It is a C++ nested symbol.  We don't need to record it
		   (I don't think); if we try to look up foo::bar::baz,
		   then symbols for the symtab containing foo should get
		   read in, I think.  */
		/* Someone says sun cc puts out symbols like
		   /foo/baz/maclib::/usr/local/bin/maclib,
		   which would get here with a symbol type of ':'.  */
		continue;

	      default:
		/* Unexpected symbol descriptor.  The second and
		   subsequent stabs of a continued stab can show up
		   here.  The question is whether they ever can mimic
		   a normal stab--it would be nice if not, since we
		   certainly don't want to spend the time searching to
		   the end of every string looking for a
		   backslash.  */

		complaint (_("unknown symbol descriptor `%c'"), p[1]);

		/* Ignore it; perhaps it is an extension that we don't
		   know about.  */
		continue;
	      }
	  }
	}
    }

  if (pst)
    {
      xcoff_end_psymtab (objfile, partial_symtabs,
			 pst, psymtab_include_list, includes_used,
			 ssymnum, dependency_list,
			 dependencies_used, textlow_not_set);
    }

  /* Record the toc offset value of this symbol table into objfile
     structure.  If no XMC_TC0 is found, toc_offset should be zero.
     Another place to obtain this information would be file auxiliary
     header.  */

  XCOFF_DATA (objfile)->toc_offset = toc_offset;
}

/* Return the toc offset value for a given objfile.  */

CORE_ADDR
xcoff_get_toc_offset (struct objfile *objfile)
{
  if (objfile)
    return XCOFF_DATA (objfile)->toc_offset;
  return 0;
}

/* Scan and build partial symbols for a symbol file.
   We have been initialized by a call to dbx_symfile_init, which 
   put all the relevant info into a "struct dbx_symfile_info",
   hung off the objfile structure.

   SECTION_OFFSETS contains offsets relative to which the symbols in the
   various sections are (depending where the sections were actually
   loaded).  */

static void
xcoff_initial_scan (struct objfile *objfile, symfile_add_flags symfile_flags)
{
  bfd *abfd;
  int val;
  int num_symbols;		/* # of symbols */
  file_ptr symtab_offset;	/* symbol table and */
  file_ptr stringtab_offset;	/* string table file offsets */
  struct xcoff_symfile_info *info;
  const char *name;
  unsigned int size;

  info = XCOFF_DATA (objfile);
  symfile_bfd = abfd = objfile->obfd.get ();
  name = objfile_name (objfile);

  num_symbols = bfd_get_symcount (abfd);	/* # of symbols */
  symtab_offset = obj_sym_filepos (abfd);	/* symbol table file offset */
  stringtab_offset = symtab_offset +
    num_symbols * coff_data (abfd)->local_symesz;

  info->min_lineno_offset = 0;
  info->max_lineno_offset = 0;
  bfd_map_over_sections (abfd, find_linenos, info);

  if (num_symbols > 0)
    {
      /* Read the string table.  */
      init_stringtab (abfd, stringtab_offset, objfile);

      /* Read the .debug section, if present and if we're not ignoring
	 it.  */
      if (!(objfile->flags & OBJF_READNEVER))
	{
	  struct bfd_section *secp;
	  bfd_size_type length;
	  bfd_byte *debugsec = NULL;

	  secp = bfd_get_section_by_name (abfd, ".debug");
	  if (secp)
	    {
	      length = bfd_section_size (secp);
	      if (length)
		{
		  debugsec
		    = (bfd_byte *) obstack_alloc (&objfile->objfile_obstack,
						  length);

		  if (!bfd_get_full_section_contents (abfd, secp, &debugsec))
		    {
		      error (_("Error reading .debug section of `%s': %s"),
			     name, bfd_errmsg (bfd_get_error ()));
		    }
		}
	    }
	  info->debugsec = (char *) debugsec;
	}
    }

  /* Read the symbols.  We keep them in core because we will want to
     access them randomly in read_symbol*.  */
  val = bfd_seek (abfd, symtab_offset, SEEK_SET);
  if (val < 0)
    error (_("Error reading symbols from %s: %s"),
	   name, bfd_errmsg (bfd_get_error ()));
  size = coff_data (abfd)->local_symesz * num_symbols;
  info->symtbl = (char *) obstack_alloc (&objfile->objfile_obstack, size);
  info->symtbl_num_syms = num_symbols;

  val = bfd_read (info->symtbl, size, abfd);
  if (val != size)
    perror_with_name (_("reading symbol table"));

  scoped_free_pendings free_pending;
  minimal_symbol_reader reader (objfile);

  /* Now that the symbol table data of the executable file are all in core,
     process them and define symbols accordingly.  */

  psymbol_functions *psf = new psymbol_functions ();
  psymtab_storage *partial_symtabs = psf->get_partial_symtabs ().get ();
  objfile->qf.emplace_front (psf);
  scan_xcoff_symtab (reader, partial_symtabs, objfile);

  /* Install any minimal symbols that have been collected as the current
     minimal symbols for this objfile.  */

  reader.install ();

  /* DWARF2 sections.  */

  dwarf2_initialize_objfile (objfile, &dwarf2_xcoff_names);
}

static void
xcoff_symfile_offsets (struct objfile *objfile,
		       const section_addr_info &addrs)
{
  const char *first_section_name;

  default_symfile_offsets (objfile, addrs);

  /* Oneof the weird side-effects of default_symfile_offsets is that
     it sometimes sets some section indices to zero for sections that,
     in fact do not exist. See the body of default_symfile_offsets
     for more info on when that happens. Undo that, as this then allows
     us to test whether the associated section exists or not, and then
     access it quickly (without searching it again).  */

  if (objfile->section_offsets.empty ())
    return; /* Is that even possible?  Better safe than sorry.  */

  first_section_name
    = bfd_section_name (objfile->sections_start[0].the_bfd_section);

  if (objfile->sect_index_text == 0
      && strcmp (first_section_name, ".text") != 0)
    objfile->sect_index_text = -1;

  if (objfile->sect_index_data == 0
      && strcmp (first_section_name, ".data") != 0)
    objfile->sect_index_data = -1;

  if (objfile->sect_index_bss == 0
      && strcmp (first_section_name, ".bss") != 0)
    objfile->sect_index_bss = -1;

  if (objfile->sect_index_rodata == 0
      && strcmp (first_section_name, ".rodata") != 0)
    objfile->sect_index_rodata = -1;
}

/* Register our ability to parse symbols for xcoff BFD files.  */

static const struct sym_fns xcoff_sym_fns =
{

  /* It is possible that coff and xcoff should be merged as
     they do have fundamental similarities (for example, the extra storage
     classes used for stabs could presumably be recognized in any COFF file).
     However, in addition to obvious things like all the csect hair, there are
     some subtler differences between xcoffread.c and coffread.c, notably
     the fact that coffread.c has no need to read in all the symbols, but
     xcoffread.c reads all the symbols and does in fact randomly access them
     (in C_BSTAT and line number processing).  */

  xcoff_new_init,		/* init anything gbl to entire symtab */
  xcoff_symfile_init,		/* read initial info, setup for sym_read() */
  xcoff_initial_scan,		/* read a symbol file into symtab */
  xcoff_symfile_finish,		/* finished with file, cleanup */
  xcoff_symfile_offsets,	/* xlate offsets ext->int form */
  default_symfile_segments,	/* Get segment information from a file.  */
  aix_process_linenos,
  default_symfile_relocate,	/* Relocate a debug section.  */
  NULL,				/* sym_probe_fns */
};

/* Same as xcoff_get_n_import_files, but for core files.  */

static int
xcoff_get_core_n_import_files (bfd *abfd)
{
  asection *sect = bfd_get_section_by_name (abfd, ".ldinfo");
  gdb_byte buf[4];
  file_ptr offset = 0;
  int n_entries = 0;

  if (sect == NULL)
    return -1;  /* Not a core file.  */

  for (offset = 0; offset < bfd_section_size (sect);)
    {
      int next;

      n_entries++;

      if (!bfd_get_section_contents (abfd, sect, buf, offset, 4))
	return -1;
      next = bfd_get_32 (abfd, buf);
      if (next == 0)
	break;  /* This is the last entry.  */
      offset += next;
    }

  /* Return the number of entries, excluding the first one, which is
     the path to the executable that produced this core file.  */
  return n_entries - 1;
}

/* Return the number of import files (shared libraries) that the given
   BFD depends on.  Return -1 if this number could not be computed.  */

int
xcoff_get_n_import_files (bfd *abfd)
{
  asection *sect = bfd_get_section_by_name (abfd, ".loader");
  gdb_byte buf[4];
  int l_nimpid;

  /* If the ".loader" section does not exist, the objfile is probably
     not an executable.  Might be a core file...  */
  if (sect == NULL)
    return xcoff_get_core_n_import_files (abfd);

  /* The number of entries in the Import Files Table is stored in
     field l_nimpid.  This field is always at offset 16, and is
     always 4 bytes long.  Read those 4 bytes.  */

  if (!bfd_get_section_contents (abfd, sect, buf, 16, 4))
    return -1;
  l_nimpid = bfd_get_32 (abfd, buf);

  /* By convention, the first entry is the default LIBPATH value
     to be used by the system loader, so it does not count towards
     the number of import files.  */
  return l_nimpid - 1;
}

void _initialize_xcoffread ();
void
_initialize_xcoffread ()
{
  add_symtab_fns (bfd_target_xcoff_flavour, &xcoff_sym_fns);
}
