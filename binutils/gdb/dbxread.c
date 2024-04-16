/* Read dbx symbol tables and convert to internal format, for GDB.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

/* This module provides three functions: dbx_symfile_init,
   which initializes to read a symbol file; dbx_new_init, which 
   discards existing cached information when all symbols are being
   discarded; and dbx_symfile_read, which reads a symbol table
   from a file.

   dbx_symfile_read only does the minimum work necessary for letting the
   user "name" things symbolically; it does not read the entire symtab.
   Instead, it reads the external and static symbols and puts them in partial
   symbol tables.  When more extensive information is requested of a
   file, the corresponding partial symbol table is mutated into a full
   fledged symbol table by going back and reading the symbols
   for real.  dbx_psymtab_to_symtab() is the function that does this */

#include "defs.h"

#include "gdbsupport/gdb_obstack.h"
#include <sys/stat.h>
#include "symtab.h"
#include "breakpoint.h"
#include "target.h"
#include "gdbcore.h"
#include "libaout.h"
#include "filenames.h"
#include "objfiles.h"
#include "buildsym-legacy.h"
#include "stabsread.h"
#include "gdb-stabs.h"
#include "demangle.h"
#include "complaints.h"
#include "cp-abi.h"
#include "cp-support.h"
#include "c-lang.h"
#include "psymtab.h"
#include "block.h"
#include "aout/aout64.h"
#include "aout/stab_gnu.h"


/* Key for dbx-associated data.  */

const registry<objfile>::key<dbx_symfile_info> dbx_objfile_data_key;

/* We put a pointer to this structure in the read_symtab_private field
   of the psymtab.  */

struct symloc
  {
    /* Offset within the file symbol table of first local symbol for this
       file.  */

    int ldsymoff;

    /* Length (in bytes) of the section of the symbol table devoted to
       this file's symbols (actually, the section bracketed may contain
       more than just this file's symbols).  If ldsymlen is 0, the only
       reason for this thing's existence is the dependency list.  Nothing
       else will happen when it is read in.  */

    int ldsymlen;

    /* The size of each symbol in the symbol file (in external form).  */

    int symbol_size;

    /* Further information needed to locate the symbols if they are in
       an ELF file.  */

    int symbol_offset;
    int string_offset;
    int file_string_offset;
    enum language pst_language;
  };

#define LDSYMOFF(p) (((struct symloc *)((p)->read_symtab_private))->ldsymoff)
#define LDSYMLEN(p) (((struct symloc *)((p)->read_symtab_private))->ldsymlen)
#define SYMLOC(p) ((struct symloc *)((p)->read_symtab_private))
#define SYMBOL_SIZE(p) (SYMLOC(p)->symbol_size)
#define SYMBOL_OFFSET(p) (SYMLOC(p)->symbol_offset)
#define STRING_OFFSET(p) (SYMLOC(p)->string_offset)
#define FILE_STRING_OFFSET(p) (SYMLOC(p)->file_string_offset)
#define PST_LANGUAGE(p) (SYMLOC(p)->pst_language)


/* The objfile we are currently reading.  */

static struct objfile *dbxread_objfile;

/* Remember what we deduced to be the source language of this psymtab.  */

static enum language psymtab_language = language_unknown;

/* The BFD for this file -- implicit parameter to next_symbol_text.  */

static bfd *symfile_bfd;

/* The size of each symbol in the symbol file (in external form).
   This is set by dbx_symfile_read when building psymtabs, and by
   dbx_psymtab_to_symtab when building symtabs.  */

static unsigned symbol_size;

/* This is the offset of the symbol table in the executable file.  */

static unsigned symbol_table_offset;

/* This is the offset of the string table in the executable file.  */

static unsigned string_table_offset;

/* For elf+stab executables, the n_strx field is not a simple index
   into the string table.  Instead, each .o file has a base offset in
   the string table, and the associated symbols contain offsets from
   this base.  The following two variables contain the base offset for
   the current and next .o files.  */

static unsigned int file_string_table_offset;
static unsigned int next_file_string_table_offset;

/* .o and NLM files contain unrelocated addresses which are based at
   0.  When non-zero, this flag disables some of the special cases for
   Solaris elf+stab text addresses at location 0.  */

static int symfile_relocatable = 0;

/* When set, we are processing a .o file compiled by sun acc.  This is
   misnamed; it refers to all stabs-in-elf implementations which use
   N_UNDF the way Sun does, including Solaris gcc.  Hopefully all
   stabs-in-elf implementations ever invented will choose to be
   compatible.  */

static unsigned char processing_acc_compilation;


/* The lowest text address we have yet encountered.  This is needed
   because in an a.out file, there is no header field which tells us
   what address the program is actually going to be loaded at, so we
   need to make guesses based on the symbols (which *are* relocated to
   reflect the address it will be loaded at).  */

static unrelocated_addr lowest_text_address;

/* Non-zero if there is any line number info in the objfile.  Prevents
   dbx_end_psymtab from discarding an otherwise empty psymtab.  */

static int has_line_numbers;

/* Complaints about the symbols we have encountered.  */

static void
unknown_symtype_complaint (const char *arg1)
{
  complaint (_("unknown symbol type %s"), arg1);
}

static void
lbrac_mismatch_complaint (int arg1)
{
  complaint (_("N_LBRAC/N_RBRAC symbol mismatch at symtab pos %d"), arg1);
}

static void
repeated_header_complaint (const char *arg1, int arg2)
{
  complaint (_("\"repeated\" header file %s not "
	       "previously seen, at symtab pos %d"),
	     arg1, arg2);
}

/* find_text_range --- find start and end of loadable code sections

   The find_text_range function finds the shortest address range that
   encloses all sections containing executable code, and stores it in
   objfile's text_addr and text_size members.

   dbx_symfile_read will use this to finish off the partial symbol
   table, in some cases.  */

static void
find_text_range (bfd * sym_bfd, struct objfile *objfile)
{
  asection *sec;
  int found_any = 0;
  CORE_ADDR start = 0;
  CORE_ADDR end = 0;

  for (sec = sym_bfd->sections; sec; sec = sec->next)
    if (bfd_section_flags (sec) & SEC_CODE)
      {
	CORE_ADDR sec_start = bfd_section_vma (sec);
	CORE_ADDR sec_end = sec_start + bfd_section_size (sec);

	if (found_any)
	  {
	    if (sec_start < start)
	      start = sec_start;
	    if (sec_end > end)
	      end = sec_end;
	  }
	else
	  {
	    start = sec_start;
	    end = sec_end;
	  }

	found_any = 1;
      }

  if (!found_any)
    error (_("Can't find any code sections in symbol file"));

  DBX_TEXT_ADDR (objfile) = start;
  DBX_TEXT_SIZE (objfile) = end - start;
}



/* During initial symbol readin, we need to have a structure to keep
   track of which psymtabs have which bincls in them.  This structure
   is used during readin to setup the list of dependencies within each
   partial symbol table.  */

struct header_file_location
{
  header_file_location (const char *name_, int instance_,
			legacy_psymtab *pst_)
    : name (name_),
      instance (instance_),
      pst (pst_)
  {
  }

  const char *name;		/* Name of header file */
  int instance;			/* See above */
  legacy_psymtab *pst;	/* Partial symtab that has the
				   BINCL/EINCL defs for this file.  */
};

/* The list of bincls.  */
static std::vector<struct header_file_location> *bincl_list;

/* Local function prototypes.  */

static void read_ofile_symtab (struct objfile *, legacy_psymtab *);

static void dbx_read_symtab (legacy_psymtab *self,
			     struct objfile *objfile);

static void dbx_expand_psymtab (legacy_psymtab *, struct objfile *);

static void read_dbx_symtab (minimal_symbol_reader &, psymtab_storage *,
			     struct objfile *);

static legacy_psymtab *find_corresponding_bincl_psymtab (const char *,
								int);

static const char *dbx_next_symbol_text (struct objfile *);

static void fill_symbuf (bfd *);

static void dbx_symfile_init (struct objfile *);

static void dbx_new_init (struct objfile *);

static void dbx_symfile_read (struct objfile *, symfile_add_flags);

static void dbx_symfile_finish (struct objfile *);

static void record_minimal_symbol (minimal_symbol_reader &,
				   const char *, unrelocated_addr, int,
				   struct objfile *);

static void add_new_header_file (const char *, int);

static void add_old_header_file (const char *, int);

static void add_this_object_header_file (int);

static legacy_psymtab *start_psymtab (psymtab_storage *, struct objfile *,
				      const char *, unrelocated_addr, int);

/* Free up old header file tables.  */

void
free_header_files (void)
{
  if (this_object_header_files)
    {
      xfree (this_object_header_files);
      this_object_header_files = NULL;
    }
  n_allocated_this_object_header_files = 0;
}

/* Allocate new header file tables.  */

void
init_header_files (void)
{
  n_allocated_this_object_header_files = 10;
  this_object_header_files = XNEWVEC (int, 10);
}

/* Add header file number I for this object file
   at the next successive FILENUM.  */

static void
add_this_object_header_file (int i)
{
  if (n_this_object_header_files == n_allocated_this_object_header_files)
    {
      n_allocated_this_object_header_files *= 2;
      this_object_header_files
	= (int *) xrealloc ((char *) this_object_header_files,
		       n_allocated_this_object_header_files * sizeof (int));
    }

  this_object_header_files[n_this_object_header_files++] = i;
}

/* Add to this file an "old" header file, one already seen in
   a previous object file.  NAME is the header file's name.
   INSTANCE is its instance code, to select among multiple
   symbol tables for the same header file.  */

static void
add_old_header_file (const char *name, int instance)
{
  struct header_file *p = HEADER_FILES (dbxread_objfile);
  int i;

  for (i = 0; i < N_HEADER_FILES (dbxread_objfile); i++)
    if (filename_cmp (p[i].name, name) == 0 && instance == p[i].instance)
      {
	add_this_object_header_file (i);
	return;
      }
  repeated_header_complaint (name, symnum);
}

/* Add to this file a "new" header file: definitions for its types follow.
   NAME is the header file's name.
   Most often this happens only once for each distinct header file,
   but not necessarily.  If it happens more than once, INSTANCE has
   a different value each time, and references to the header file
   use INSTANCE values to select among them.

   dbx output contains "begin" and "end" markers for each new header file,
   but at this level we just need to know which files there have been;
   so we record the file when its "begin" is seen and ignore the "end".  */

static void
add_new_header_file (const char *name, int instance)
{
  int i;
  struct header_file *hfile;

  /* Make sure there is room for one more header file.  */

  i = N_ALLOCATED_HEADER_FILES (dbxread_objfile);

  if (N_HEADER_FILES (dbxread_objfile) == i)
    {
      if (i == 0)
	{
	  N_ALLOCATED_HEADER_FILES (dbxread_objfile) = 10;
	  HEADER_FILES (dbxread_objfile) = (struct header_file *)
	    xmalloc (10 * sizeof (struct header_file));
	}
      else
	{
	  i *= 2;
	  N_ALLOCATED_HEADER_FILES (dbxread_objfile) = i;
	  HEADER_FILES (dbxread_objfile) = (struct header_file *)
	    xrealloc ((char *) HEADER_FILES (dbxread_objfile),
		      (i * sizeof (struct header_file)));
	}
    }

  /* Create an entry for this header file.  */

  i = N_HEADER_FILES (dbxread_objfile)++;
  hfile = HEADER_FILES (dbxread_objfile) + i;
  hfile->name = xstrdup (name);
  hfile->instance = instance;
  hfile->length = 10;
  hfile->vector = XCNEWVEC (struct type *, 10);

  add_this_object_header_file (i);
}

#if 0
static struct type **
explicit_lookup_type (int real_filenum, int index)
{
  struct header_file *f = &HEADER_FILES (dbxread_objfile)[real_filenum];

  if (index >= f->length)
    {
      f->length *= 2;
      f->vector = (struct type **)
	xrealloc (f->vector, f->length * sizeof (struct type *));
      memset (&f->vector[f->length / 2],
	      '\0', f->length * sizeof (struct type *) / 2);
    }
  return &f->vector[index];
}
#endif

static void
record_minimal_symbol (minimal_symbol_reader &reader,
		       const char *name, unrelocated_addr address, int type,
		       struct objfile *objfile)
{
  enum minimal_symbol_type ms_type;
  int section;

  switch (type)
    {
    case N_TEXT | N_EXT:
      ms_type = mst_text;
      section = SECT_OFF_TEXT (objfile);
      break;
    case N_DATA | N_EXT:
      ms_type = mst_data;
      section = SECT_OFF_DATA (objfile);
      break;
    case N_BSS | N_EXT:
      ms_type = mst_bss;
      section = SECT_OFF_BSS (objfile);
      break;
    case N_ABS | N_EXT:
      ms_type = mst_abs;
      section = -1;
      break;
#ifdef N_SETV
    case N_SETV | N_EXT:
      ms_type = mst_data;
      section = SECT_OFF_DATA (objfile);
      break;
    case N_SETV:
      /* I don't think this type actually exists; since a N_SETV is the result
	 of going over many .o files, it doesn't make sense to have one
	 file local.  */
      ms_type = mst_file_data;
      section = SECT_OFF_DATA (objfile);
      break;
#endif
    case N_TEXT:
    case N_NBTEXT:
    case N_FN:
    case N_FN_SEQ:
      ms_type = mst_file_text;
      section = SECT_OFF_TEXT (objfile);
      break;
    case N_DATA:
      ms_type = mst_file_data;

      /* Check for __DYNAMIC, which is used by Sun shared libraries. 
	 Record it as global even if it's local, not global, so
	 lookup_minimal_symbol can find it.  We don't check symbol_leading_char
	 because for SunOS4 it always is '_'.  */
      if (strcmp ("__DYNAMIC", name) == 0)
	ms_type = mst_data;

      /* Same with virtual function tables, both global and static.  */
      {
	const char *tempstring = name;

	if (*tempstring != '\0'
	    && *tempstring == bfd_get_symbol_leading_char (objfile->obfd.get ()))
	  ++tempstring;
	if (is_vtable_name (tempstring))
	  ms_type = mst_data;
      }
      section = SECT_OFF_DATA (objfile);
      break;
    case N_BSS:
      ms_type = mst_file_bss;
      section = SECT_OFF_BSS (objfile);
      break;
    default:
      ms_type = mst_unknown;
      section = -1;
      break;
    }

  if ((ms_type == mst_file_text || ms_type == mst_text)
      && address < lowest_text_address)
    lowest_text_address = address;

  reader.record_with_info (name, address, ms_type, section);
}

/* Scan and build partial symbols for a symbol file.
   We have been initialized by a call to dbx_symfile_init, which 
   put all the relevant info into a "struct dbx_symfile_info",
   hung off the objfile structure.  */

static void
dbx_symfile_read (struct objfile *objfile, symfile_add_flags symfile_flags)
{
  bfd *sym_bfd;
  int val;

  sym_bfd = objfile->obfd.get ();

  /* .o and .nlm files are relocatables with text, data and bss segs based at
     0.  This flag disables special (Solaris stabs-in-elf only) fixups for
     symbols with a value of 0.  */

  symfile_relocatable = bfd_get_file_flags (sym_bfd) & HAS_RELOC;

  val = bfd_seek (sym_bfd, DBX_SYMTAB_OFFSET (objfile), SEEK_SET);
  if (val < 0)
    perror_with_name (objfile_name (objfile));

  symbol_size = DBX_SYMBOL_SIZE (objfile);
  symbol_table_offset = DBX_SYMTAB_OFFSET (objfile);

  scoped_free_pendings free_pending;

  minimal_symbol_reader reader (objfile);

  /* Read stabs data from executable file and define symbols.  */

  psymbol_functions *psf = new psymbol_functions ();
  psymtab_storage *partial_symtabs = psf->get_partial_symtabs ().get ();
  objfile->qf.emplace_front (psf);
  read_dbx_symtab (reader, partial_symtabs, objfile);

  /* Install any minimal symbols that have been collected as the current
     minimal symbols for this objfile.  */

  reader.install ();
}

/* Initialize anything that needs initializing when a completely new
   symbol file is specified (not just adding some symbols from another
   file, e.g. a shared library).  */

static void
dbx_new_init (struct objfile *ignore)
{
  stabsread_new_init ();
  init_header_files ();
}


/* dbx_symfile_init ()
   is the dbx-specific initialization routine for reading symbols.
   It is passed a struct objfile which contains, among other things,
   the BFD for the file whose symbols are being read, and a slot for a pointer
   to "private data" which we fill with goodies.

   We read the string table into malloc'd space and stash a pointer to it.

   Since BFD doesn't know how to read debug symbols in a format-independent
   way (and may never do so...), we have to do it ourselves.  We will never
   be called unless this is an a.out (or very similar) file.
   FIXME, there should be a cleaner peephole into the BFD environment here.  */

#define DBX_STRINGTAB_SIZE_SIZE sizeof(long)	/* FIXME */

static void
dbx_symfile_init (struct objfile *objfile)
{
  int val;
  bfd *sym_bfd = objfile->obfd.get ();
  const char *name = bfd_get_filename (sym_bfd);
  asection *text_sect;
  unsigned char size_temp[DBX_STRINGTAB_SIZE_SIZE];

  /* Allocate struct to keep track of the symfile.  */
  dbx_objfile_data_key.emplace (objfile);

  DBX_TEXT_SECTION (objfile) = bfd_get_section_by_name (sym_bfd, ".text");
  DBX_DATA_SECTION (objfile) = bfd_get_section_by_name (sym_bfd, ".data");
  DBX_BSS_SECTION (objfile) = bfd_get_section_by_name (sym_bfd, ".bss");

  /* FIXME POKING INSIDE BFD DATA STRUCTURES.  */
#define	STRING_TABLE_OFFSET	(sym_bfd->origin + obj_str_filepos (sym_bfd))
#define	SYMBOL_TABLE_OFFSET	(sym_bfd->origin + obj_sym_filepos (sym_bfd))

  /* FIXME POKING INSIDE BFD DATA STRUCTURES.  */

  text_sect = bfd_get_section_by_name (sym_bfd, ".text");
  if (!text_sect)
    error (_("Can't find .text section in symbol file"));
  DBX_TEXT_ADDR (objfile) = bfd_section_vma (text_sect);
  DBX_TEXT_SIZE (objfile) = bfd_section_size (text_sect);

  DBX_SYMBOL_SIZE (objfile) = obj_symbol_entry_size (sym_bfd);
  DBX_SYMCOUNT (objfile) = bfd_get_symcount (sym_bfd);
  DBX_SYMTAB_OFFSET (objfile) = SYMBOL_TABLE_OFFSET;

  /* Read the string table and stash it away in the objfile_obstack.
     When we blow away the objfile the string table goes away as well.
     Note that gdb used to use the results of attempting to malloc the
     string table, based on the size it read, as a form of sanity check
     for botched byte swapping, on the theory that a byte swapped string
     table size would be so totally bogus that the malloc would fail.  Now
     that we put in on the objfile_obstack, we can't do this since gdb gets
     a fatal error (out of virtual memory) if the size is bogus.  We can
     however at least check to see if the size is less than the size of
     the size field itself, or larger than the size of the entire file.
     Note that all valid string tables have a size greater than zero, since
     the bytes used to hold the size are included in the count.  */

  if (STRING_TABLE_OFFSET == 0)
    {
      /* It appears that with the existing bfd code, STRING_TABLE_OFFSET
	 will never be zero, even when there is no string table.  This
	 would appear to be a bug in bfd.  */
      DBX_STRINGTAB_SIZE (objfile) = 0;
      DBX_STRINGTAB (objfile) = NULL;
    }
  else
    {
      val = bfd_seek (sym_bfd, STRING_TABLE_OFFSET, SEEK_SET);
      if (val < 0)
	perror_with_name (name);

      memset (size_temp, 0, sizeof (size_temp));
      val = bfd_read (size_temp, sizeof (size_temp), sym_bfd);
      if (val < 0)
	{
	  perror_with_name (name);
	}
      else if (val == 0)
	{
	  /* With the existing bfd code, STRING_TABLE_OFFSET will be set to
	     EOF if there is no string table, and attempting to read the size
	     from EOF will read zero bytes.  */
	  DBX_STRINGTAB_SIZE (objfile) = 0;
	  DBX_STRINGTAB (objfile) = NULL;
	}
      else
	{
	  /* Read some data that would appear to be the string table size.
	     If there really is a string table, then it is probably the right
	     size.  Byteswap if necessary and validate the size.  Note that
	     the minimum is DBX_STRINGTAB_SIZE_SIZE.  If we just read some
	     random data that happened to be at STRING_TABLE_OFFSET, because
	     bfd can't tell us there is no string table, the sanity checks may
	     or may not catch this.  */
	  DBX_STRINGTAB_SIZE (objfile) = bfd_h_get_32 (sym_bfd, size_temp);

	  if (DBX_STRINGTAB_SIZE (objfile) < sizeof (size_temp)
	      || DBX_STRINGTAB_SIZE (objfile) > bfd_get_size (sym_bfd))
	    error (_("ridiculous string table size (%d bytes)."),
		   DBX_STRINGTAB_SIZE (objfile));

	  DBX_STRINGTAB (objfile) =
	    (char *) obstack_alloc (&objfile->objfile_obstack,
				    DBX_STRINGTAB_SIZE (objfile));
	  OBJSTAT (objfile, sz_strtab += DBX_STRINGTAB_SIZE (objfile));

	  /* Now read in the string table in one big gulp.  */

	  val = bfd_seek (sym_bfd, STRING_TABLE_OFFSET, SEEK_SET);
	  if (val < 0)
	    perror_with_name (name);
	  val = bfd_read (DBX_STRINGTAB (objfile),
			  DBX_STRINGTAB_SIZE (objfile),
			  sym_bfd);
	  if (val != DBX_STRINGTAB_SIZE (objfile))
	    perror_with_name (name);
	}
    }
}

/* Perform any local cleanups required when we are done with a particular
   objfile.  I.E, we are in the process of discarding all symbol information
   for an objfile, freeing up all memory held for it, and unlinking the
   objfile struct from the global list of known objfiles.  */

static void
dbx_symfile_finish (struct objfile *objfile)
{
  free_header_files ();
}

dbx_symfile_info::~dbx_symfile_info ()
{
  if (header_files != NULL)
    {
      int i = n_header_files;
      struct header_file *hfiles = header_files;

      while (--i >= 0)
	{
	  xfree (hfiles[i].name);
	  xfree (hfiles[i].vector);
	}
      xfree (hfiles);
    }
}



/* Buffer for reading the symbol table entries.  */
static struct external_nlist symbuf[4096];
static int symbuf_idx;
static int symbuf_end;

/* Name of last function encountered.  Used in Solaris to approximate
   object file boundaries.  */
static const char *last_function_name;

/* The address in memory of the string table of the object file we are
   reading (which might not be the "main" object file, but might be a
   shared library or some other dynamically loaded thing).  This is
   set by read_dbx_symtab when building psymtabs, and by
   read_ofile_symtab when building symtabs, and is used only by
   next_symbol_text.  FIXME: If that is true, we don't need it when
   building psymtabs, right?  */
static char *stringtab_global;

/* These variables are used to control fill_symbuf when the stabs
   symbols are not contiguous (as may be the case when a COFF file is
   linked using --split-by-reloc).  */
static const std::vector<asection *> *symbuf_sections;
static size_t sect_idx;
static unsigned int symbuf_left;
static unsigned int symbuf_read;

/* This variable stores a global stabs buffer, if we read stabs into
   memory in one chunk in order to process relocations.  */
static bfd_byte *stabs_data;

/* Refill the symbol table input buffer
   and set the variables that control fetching entries from it.
   Reports an error if no data available.
   This function can read past the end of the symbol table
   (into the string table) but this does no harm.  */

static void
fill_symbuf (bfd *sym_bfd)
{
  unsigned int count;
  int nbytes;

  if (stabs_data)
    {
      nbytes = sizeof (symbuf);
      if (nbytes > symbuf_left)
	nbytes = symbuf_left;
      memcpy (symbuf, stabs_data + symbuf_read, nbytes);
    }
  else if (symbuf_sections == NULL)
    {
      count = sizeof (symbuf);
      nbytes = bfd_read (symbuf, count, sym_bfd);
    }
  else
    {
      if (symbuf_left <= 0)
	{
	  file_ptr filepos = (*symbuf_sections)[sect_idx]->filepos;

	  if (bfd_seek (sym_bfd, filepos, SEEK_SET) != 0)
	    perror_with_name (bfd_get_filename (sym_bfd));
	  symbuf_left = bfd_section_size ((*symbuf_sections)[sect_idx]);
	  symbol_table_offset = filepos - symbuf_read;
	  ++sect_idx;
	}

      count = symbuf_left;
      if (count > sizeof (symbuf))
	count = sizeof (symbuf);
      nbytes = bfd_read (symbuf, count, sym_bfd);
    }

  if (nbytes < 0)
    perror_with_name (bfd_get_filename (sym_bfd));
  else if (nbytes == 0)
    error (_("Premature end of file reading symbol table"));
  symbuf_end = nbytes / symbol_size;
  symbuf_idx = 0;
  symbuf_left -= nbytes;
  symbuf_read += nbytes;
}

static void
stabs_seek (int sym_offset)
{
  if (stabs_data)
    {
      symbuf_read += sym_offset;
      symbuf_left -= sym_offset;
    }
  else
    if (bfd_seek (symfile_bfd, sym_offset, SEEK_CUR) != 0)
      perror_with_name (bfd_get_filename (symfile_bfd));
}

#define INTERNALIZE_SYMBOL(intern, extern, abfd)			\
  {									\
    (intern).n_strx = bfd_h_get_32 (abfd, (extern)->e_strx);		\
    (intern).n_type = bfd_h_get_8 (abfd, (extern)->e_type);		\
    (intern).n_other = 0;						\
    (intern).n_desc = bfd_h_get_16 (abfd, (extern)->e_desc);  		\
    if (bfd_get_sign_extend_vma (abfd))					\
      (intern).n_value = bfd_h_get_signed_32 (abfd, (extern)->e_value);	\
    else								\
      (intern).n_value = bfd_h_get_32 (abfd, (extern)->e_value);	\
  }

/* Invariant: The symbol pointed to by symbuf_idx is the first one
   that hasn't been swapped.  Swap the symbol at the same time
   that symbuf_idx is incremented.  */

/* dbx allows the text of a symbol name to be continued into the
   next symbol name!  When such a continuation is encountered
   (a \ at the end of the text of a name)
   call this function to get the continuation.  */

static const char *
dbx_next_symbol_text (struct objfile *objfile)
{
  struct internal_nlist nlist;

  if (symbuf_idx == symbuf_end)
    fill_symbuf (symfile_bfd);

  symnum++;
  INTERNALIZE_SYMBOL (nlist, &symbuf[symbuf_idx], symfile_bfd);
  OBJSTAT (objfile, n_stabs++);

  symbuf_idx++;

  return nlist.n_strx + stringtab_global + file_string_table_offset;
}


/* Given a name, value pair, find the corresponding
   bincl in the list.  Return the partial symtab associated
   with that header_file_location.  */

static legacy_psymtab *
find_corresponding_bincl_psymtab (const char *name, int instance)
{
  for (const header_file_location &bincl : *bincl_list)
    if (bincl.instance == instance
	&& strcmp (name, bincl.name) == 0)
      return bincl.pst;

  repeated_header_complaint (name, symnum);
  return (legacy_psymtab *) 0;
}

/* Set namestring based on nlist.  If the string table index is invalid, 
   give a fake name, and print a single error message per symbol file read,
   rather than abort the symbol reading or flood the user with messages.  */

static const char *
set_namestring (struct objfile *objfile, const struct internal_nlist *nlist)
{
  const char *namestring;

  if (nlist->n_strx + file_string_table_offset
      >= DBX_STRINGTAB_SIZE (objfile)
      || nlist->n_strx + file_string_table_offset < nlist->n_strx)
    {
      complaint (_("bad string table offset in symbol %d"),
		 symnum);
      namestring = "<bad string table offset>";
    } 
  else
    namestring = (nlist->n_strx + file_string_table_offset
		  + DBX_STRINGTAB (objfile));
  return namestring;
}

static struct bound_minimal_symbol
find_stab_function (const char *namestring, const char *filename,
		    struct objfile *objfile)
{
  struct bound_minimal_symbol msym;
  int n;

  const char *colon = strchr (namestring, ':');
  if (colon == NULL)
    n = 0;
  else
    n = colon - namestring;

  char *p = (char *) alloca (n + 2);
  strncpy (p, namestring, n);
  p[n] = 0;

  msym = lookup_minimal_symbol (p, filename, objfile);
  if (msym.minsym == NULL)
    {
      /* Sun Fortran appends an underscore to the minimal symbol name,
	 try again with an appended underscore if the minimal symbol
	 was not found.  */
      p[n] = '_';
      p[n + 1] = 0;
      msym = lookup_minimal_symbol (p, filename, objfile);
    }

  if (msym.minsym == NULL && filename != NULL)
    {
      /* Try again without the filename.  */
      p[n] = 0;
      msym = lookup_minimal_symbol (p, NULL, objfile);
    }
  if (msym.minsym == NULL && filename != NULL)
    {
      /* And try again for Sun Fortran, but without the filename.  */
      p[n] = '_';
      p[n + 1] = 0;
      msym = lookup_minimal_symbol (p, NULL, objfile);
    }

  return msym;
}

static void
function_outside_compilation_unit_complaint (const char *arg1)
{
  complaint (_("function `%s' appears to be defined "
	       "outside of all compilation units"),
	     arg1);
}

/* Setup partial_symtab's describing each source file for which
   debugging information is available.  */

static void
read_dbx_symtab (minimal_symbol_reader &reader,
		 psymtab_storage *partial_symtabs,
		 struct objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  struct external_nlist *bufp = 0;	/* =0 avoids gcc -Wall glitch.  */
  struct internal_nlist nlist;
  CORE_ADDR text_addr;
  int text_size;
  const char *sym_name;
  int sym_len;

  const char *namestring;
  int nsl;
  int past_first_source_file = 0;
  CORE_ADDR last_function_start = 0;
  bfd *abfd;
  int textlow_not_set;
  int data_sect_index;

  /* Current partial symtab.  */
  legacy_psymtab *pst;

  /* List of current psymtab's include files.  */
  const char **psymtab_include_list;
  int includes_allocated;
  int includes_used;

  /* Index within current psymtab dependency list.  */
  legacy_psymtab **dependency_list;
  int dependencies_used, dependencies_allocated;

  text_addr = DBX_TEXT_ADDR (objfile);
  text_size = DBX_TEXT_SIZE (objfile);

  /* FIXME.  We probably want to change stringtab_global rather than add this
     while processing every symbol entry.  FIXME.  */
  file_string_table_offset = 0;
  next_file_string_table_offset = 0;

  stringtab_global = DBX_STRINGTAB (objfile);

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

  /* Init bincl list */
  std::vector<struct header_file_location> bincl_storage;
  scoped_restore restore_bincl_global
    = make_scoped_restore (&bincl_list, &bincl_storage);

  set_last_source_file (NULL);

  lowest_text_address = (unrelocated_addr) -1;

  symfile_bfd = objfile->obfd.get ();	/* For next_text_symbol.  */
  abfd = objfile->obfd.get ();
  symbuf_end = symbuf_idx = 0;
  next_symbol_text_func = dbx_next_symbol_text;
  textlow_not_set = 1;
  has_line_numbers = 0;

  /* FIXME: jimb/2003-09-12: We don't apply the right section's offset
     to global and static variables.  The stab for a global or static
     variable doesn't give us any indication of which section it's in,
     so we can't tell immediately which offset in
     objfile->section_offsets we should apply to the variable's
     address.

     We could certainly find out which section contains the variable
     by looking up the variable's unrelocated address with
     find_pc_section, but that would be expensive; this is the
     function that constructs the partial symbol tables by examining
     every symbol in the entire executable, and it's
     performance-critical.  So that expense would not be welcome.  I'm
     not sure what to do about this at the moment.

     What we have done for years is to simply assume that the .data
     section's offset is appropriate for all global and static
     variables.  Recently, this was expanded to fall back to the .bss
     section's offset if there is no .data section, and then to the
     .rodata section's offset.  */
  data_sect_index = objfile->sect_index_data;
  if (data_sect_index == -1)
    data_sect_index = SECT_OFF_BSS (objfile);
  if (data_sect_index == -1)
    data_sect_index = SECT_OFF_RODATA (objfile);

  /* If data_sect_index is still -1, that's okay.  It's perfectly fine
     for the file to have no .data, no .bss, and no .text at all, if
     it also has no global or static variables.  */

  for (symnum = 0; symnum < DBX_SYMCOUNT (objfile); symnum++)
    {
      /* Get the symbol for this run and pull out some info.  */
      QUIT;			/* Allow this to be interruptable.  */
      if (symbuf_idx == symbuf_end)
	fill_symbuf (abfd);
      bufp = &symbuf[symbuf_idx++];

      /*
       * Special case to speed up readin.
       */
      if (bfd_h_get_8 (abfd, bufp->e_type) == N_SLINE)
	{
	  has_line_numbers = 1;
	  continue;
	}

      INTERNALIZE_SYMBOL (nlist, bufp, abfd);
      OBJSTAT (objfile, n_stabs++);

      /* Ok.  There is a lot of code duplicated in the rest of this
	 switch statement (for efficiency reasons).  Since I don't
	 like duplicating code, I will do my penance here, and
	 describe the code which is duplicated:

	 *) The assignment to namestring.
	 *) The call to strchr.
	 *) The addition of a partial symbol the two partial
	 symbol lists.  This last is a large section of code, so
	 I've embedded it in the following macro.  */

      switch (nlist.n_type)
	{
	  /*
	   * Standard, external, non-debugger, symbols
	   */

	case N_TEXT | N_EXT:
	case N_NBTEXT | N_EXT:
	  goto record_it;

	case N_DATA | N_EXT:
	case N_NBDATA | N_EXT:
	  goto record_it;

	case N_BSS:
	case N_BSS | N_EXT:
	case N_NBBSS | N_EXT:
	case N_SETV | N_EXT:		/* FIXME, is this in BSS? */
	  goto record_it;

	case N_ABS | N_EXT:
	  record_it:
	  namestring = set_namestring (objfile, &nlist);

	  record_minimal_symbol (reader, namestring,
				 unrelocated_addr (nlist.n_value),
				 nlist.n_type, objfile);	/* Always */
	  continue;

	  /* Standard, local, non-debugger, symbols.  */

	case N_NBTEXT:

	  /* We need to be able to deal with both N_FN or N_TEXT,
	     because we have no way of knowing whether the sys-supplied ld
	     or GNU ld was used to make the executable.  Sequents throw
	     in another wrinkle -- they renumbered N_FN.  */

	case N_FN:
	case N_FN_SEQ:
	case N_TEXT:
	  namestring = set_namestring (objfile, &nlist);

	  if ((namestring[0] == '-' && namestring[1] == 'l')
	      || (namestring[(nsl = strlen (namestring)) - 1] == 'o'
		  && namestring[nsl - 2] == '.'))
	    {
	      unrelocated_addr unrel_val = unrelocated_addr (nlist.n_value);

	      if (past_first_source_file && pst
		  /* The gould NP1 uses low values for .o and -l symbols
		     which are not the address.  */
		  && unrel_val >= pst->unrelocated_text_low ())
		{
		  dbx_end_psymtab (objfile, partial_symtabs,
				   pst, psymtab_include_list,
				   includes_used, symnum * symbol_size,
				   unrel_val > pst->unrelocated_text_high ()
				   ? unrel_val : pst->unrelocated_text_high (),
				   dependency_list, dependencies_used,
				   textlow_not_set);
		  pst = (legacy_psymtab *) 0;
		  includes_used = 0;
		  dependencies_used = 0;
		  has_line_numbers = 0;
		}
	      else
		past_first_source_file = 1;
	    }
	  else
	    goto record_it;
	  continue;

	case N_DATA:
	  goto record_it;

	case N_UNDF | N_EXT:
	  /* The case (nlist.n_value != 0) is a "Fortran COMMON" symbol.
	     We used to rely on the target to tell us whether it knows
	     where the symbol has been relocated to, but none of the
	     target implementations actually provided that operation.
	     So we just ignore the symbol, the same way we would do if
	     we had a target-side symbol lookup which returned no match.

	     All other symbols (with nlist.n_value == 0), are really
	     undefined, and so we ignore them too.  */
	  continue;

	case N_UNDF:
	  if (processing_acc_compilation && nlist.n_strx == 1)
	    {
	      /* Deal with relative offsets in the string table
		 used in ELF+STAB under Solaris.  If we want to use the
		 n_strx field, which contains the name of the file,
		 we must adjust file_string_table_offset *before* calling
		 set_namestring().  */
	      past_first_source_file = 1;
	      file_string_table_offset = next_file_string_table_offset;
	      next_file_string_table_offset =
		file_string_table_offset + nlist.n_value;
	      if (next_file_string_table_offset < file_string_table_offset)
		error (_("string table offset backs up at %d"), symnum);
	      /* FIXME -- replace error() with complaint.  */
	      continue;
	    }
	  continue;

	  /* Lots of symbol types we can just ignore.  */

	case N_ABS:
	case N_NBDATA:
	case N_NBBSS:
	  continue;

	  /* Keep going . . .  */

	  /*
	   * Special symbol types for GNU
	   */
	case N_INDR:
	case N_INDR | N_EXT:
	case N_SETA:
	case N_SETA | N_EXT:
	case N_SETT:
	case N_SETT | N_EXT:
	case N_SETD:
	case N_SETD | N_EXT:
	case N_SETB:
	case N_SETB | N_EXT:
	case N_SETV:
	  continue;

	  /*
	   * Debugger symbols
	   */

	case N_SO:
	  {
	    CORE_ADDR valu;
	    static int prev_so_symnum = -10;
	    static int first_so_symnum;
	    const char *p;
	    static const char *dirname_nso;
	    int prev_textlow_not_set;

	    valu = nlist.n_value;

	    prev_textlow_not_set = textlow_not_set;

	    /* A zero value is probably an indication for the SunPRO 3.0
	       compiler.  dbx_end_psymtab explicitly tests for zero, so
	       don't relocate it.  */

	    if (nlist.n_value == 0
		&& gdbarch_sofun_address_maybe_missing (gdbarch))
	      {
		textlow_not_set = 1;
		valu = 0;
	      }
	    else
	      textlow_not_set = 0;

	    past_first_source_file = 1;

	    if (prev_so_symnum != symnum - 1)
	      {			/* Here if prev stab wasn't N_SO.  */
		first_so_symnum = symnum;

		if (pst)
		  {
		    unrelocated_addr unrel_value = unrelocated_addr (valu);
		    dbx_end_psymtab (objfile, partial_symtabs,
				     pst, psymtab_include_list,
				     includes_used, symnum * symbol_size,
				     unrel_value > pst->unrelocated_text_high ()
				     ? unrel_value
				     : pst->unrelocated_text_high (),
				     dependency_list, dependencies_used,
				     prev_textlow_not_set);
		    pst = (legacy_psymtab *) 0;
		    includes_used = 0;
		    dependencies_used = 0;
		    has_line_numbers = 0;
		  }
	      }

	    prev_so_symnum = symnum;

	    /* End the current partial symtab and start a new one.  */

	    namestring = set_namestring (objfile, &nlist);

	    /* Null name means end of .o file.  Don't start a new one.  */
	    if (*namestring == '\000')
	      continue;

	    /* Some compilers (including gcc) emit a pair of initial N_SOs.
	       The first one is a directory name; the second the file name.
	       If pst exists, is empty, and has a filename ending in '/',
	       we assume the previous N_SO was a directory name.  */

	    p = lbasename (namestring);
	    if (p != namestring && *p == '\000')
	      {
		/* Save the directory name SOs locally, then save it into
		   the psymtab when it's created below.  */
		dirname_nso = namestring;
		continue;		
	      }

	    /* Some other compilers (C++ ones in particular) emit useless
	       SOs for non-existant .c files.  We ignore all subsequent SOs
	       that immediately follow the first.  */

	    if (!pst)
	      {
		pst = start_psymtab (partial_symtabs, objfile,
				     namestring,
				     unrelocated_addr (valu),
				     first_so_symnum * symbol_size);
		pst->dirname = dirname_nso;
		dirname_nso = NULL;
	      }
	    continue;
	  }

	case N_BINCL:
	  {
	    enum language tmp_language;

	    /* Add this bincl to the bincl_list for future EXCLs.  No
	       need to save the string; it'll be around until
	       read_dbx_symtab function returns.  */

	    namestring = set_namestring (objfile, &nlist);
	    tmp_language = deduce_language_from_filename (namestring);

	    /* Only change the psymtab's language if we've learned
	       something useful (eg. tmp_language is not language_unknown).
	       In addition, to match what start_subfile does, never change
	       from C++ to C.  */
	    if (tmp_language != language_unknown
		&& (tmp_language != language_c
		    || psymtab_language != language_cplus))
	      psymtab_language = tmp_language;

	    if (pst == NULL)
	      {
		/* FIXME: we should not get here without a PST to work on.
		   Attempt to recover.  */
		complaint (_("N_BINCL %s not in entries for "
			     "any file, at symtab pos %d"),
			   namestring, symnum);
		continue;
	      }
	    bincl_list->emplace_back (namestring, nlist.n_value, pst);

	    /* Mark down an include file in the current psymtab.  */

	    goto record_include_file;
	  }

	case N_SOL:
	  {
	    enum language tmp_language;

	    /* Mark down an include file in the current psymtab.  */
	    namestring = set_namestring (objfile, &nlist);
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
	    if (pst && filename_cmp (namestring, pst->filename) == 0)
	      continue;
	    {
	      int i;

	      for (i = 0; i < includes_used; i++)
		if (filename_cmp (namestring, psymtab_include_list[i]) == 0)
		  {
		    i = -1;
		    break;
		  }
	      if (i == -1)
		continue;
	    }

	  record_include_file:

	    psymtab_include_list[includes_used++] = namestring;
	    if (includes_used >= includes_allocated)
	      {
		const char **orig = psymtab_include_list;

		psymtab_include_list = (const char **)
		  alloca ((includes_allocated *= 2) * sizeof (const char *));
		memcpy (psymtab_include_list, orig,
			includes_used * sizeof (const char *));
	      }
	    continue;
	  }
	case N_LSYM:		/* Typedef or automatic variable.  */
	case N_STSYM:		/* Data seg var -- static.  */
	case N_LCSYM:		/* BSS      "  */
	case N_ROSYM:		/* Read-only data seg var -- static.  */
	case N_NBSTS:		/* Gould nobase.  */
	case N_NBLCS:		/* symbols.  */
	case N_FUN:
	case N_GSYM:		/* Global (extern) variable; can be
				   data or bss (sigh FIXME).  */

	  /* Following may probably be ignored; I'll leave them here
	     for now (until I do Pascal and Modula 2 extensions).  */

	case N_PC:		/* I may or may not need this; I
				   suspect not.  */
	case N_M2C:		/* I suspect that I can ignore this here.  */
	case N_SCOPE:		/* Same.   */
	{
	  const char *p;

	  namestring = set_namestring (objfile, &nlist);

	  /* See if this is an end of function stab.  */
	  if (pst && nlist.n_type == N_FUN && *namestring == '\000')
	    {
	      unrelocated_addr valu;

	      /* It's value is the size (in bytes) of the function for
		 function relative stabs, or the address of the function's
		 end for old style stabs.  */
	      valu = unrelocated_addr (nlist.n_value + last_function_start);
	      if (pst->unrelocated_text_high () == unrelocated_addr (0)
		  || valu > pst->unrelocated_text_high ())
		pst->set_text_high (valu);
	      break;
	    }

	  p = (char *) strchr (namestring, ':');
	  if (!p)
	    continue;		/* Not a debugging symbol.   */

	  sym_len = 0;
	  sym_name = NULL;	/* pacify "gcc -Werror" */
	  if (psymtab_language == language_cplus)
	    {
	      std::string name (namestring, p - namestring);
	      gdb::unique_xmalloc_ptr<char> new_name
		= cp_canonicalize_string (name.c_str ());
	      if (new_name != nullptr)
		{
		  sym_len = strlen (new_name.get ());
		  sym_name = obstack_strdup (&objfile->objfile_obstack,
					     new_name.get ());
		}
	    }
	  else if (psymtab_language == language_c)
	    {
	      std::string name (namestring, p - namestring);
	      gdb::unique_xmalloc_ptr<char> new_name
		= c_canonicalize_name (name.c_str ());
	      if (new_name != nullptr)
		{
		  sym_len = strlen (new_name.get ());
		  sym_name = obstack_strdup (&objfile->objfile_obstack,
					     new_name.get ());
		}
	    }

	  if (sym_len == 0)
	    {
	      sym_name = namestring;
	      sym_len = p - namestring;
	    }

	  /* Main processing section for debugging symbols which
	     the initial read through the symbol tables needs to worry
	     about.  If we reach this point, the symbol which we are
	     considering is definitely one we are interested in.
	     p must also contain the (valid) index into the namestring
	     which indicates the debugging type symbol.  */

	  switch (p[1])
	    {
	    case 'S':
	      if (pst != nullptr)
		pst->add_psymbol (std::string_view (sym_name, sym_len), true,
				  VAR_DOMAIN, LOC_STATIC,
				  data_sect_index,
				  psymbol_placement::STATIC,
				  unrelocated_addr (nlist.n_value),
				  psymtab_language,
				  partial_symtabs, objfile);
	      else
		complaint (_("static `%*s' appears to be defined "
			     "outside of all compilation units"),
			   sym_len, sym_name);
	      continue;

	    case 'G':
	      /* The addresses in these entries are reported to be
		 wrong.  See the code that reads 'G's for symtabs.  */
	      if (pst != nullptr)
		pst->add_psymbol (std::string_view (sym_name, sym_len), true,
				  VAR_DOMAIN, LOC_STATIC,
				  data_sect_index,
				  psymbol_placement::GLOBAL,
				  unrelocated_addr (nlist.n_value),
				  psymtab_language,
				  partial_symtabs, objfile);
	      else
		complaint (_("global `%*s' appears to be defined "
			     "outside of all compilation units"),
			   sym_len, sym_name);
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
		  if (pst != nullptr)
		    pst->add_psymbol (std::string_view (sym_name, sym_len),
				      true, STRUCT_DOMAIN, LOC_TYPEDEF, -1,
				      psymbol_placement::STATIC,
				      unrelocated_addr (0),
				      psymtab_language,
				      partial_symtabs, objfile);
		  else
		    complaint (_("enum, struct, or union `%*s' appears "
				 "to be defined outside of all "
				 "compilation units"),
			       sym_len, sym_name);
		  if (p[2] == 't')
		    {
		      /* Also a typedef with the same name.  */
		      if (pst != nullptr)
			pst->add_psymbol (std::string_view (sym_name, sym_len),
					  true, VAR_DOMAIN, LOC_TYPEDEF, -1,
					  psymbol_placement::STATIC,
					  unrelocated_addr (0),
					  psymtab_language,
					  partial_symtabs, objfile);
		      else
			complaint (_("typedef `%*s' appears to be defined "
				     "outside of all compilation units"),
				   sym_len, sym_name);
		      p += 1;
		    }
		}
	      goto check_enum;

	    case 't':
	      if (p != namestring)	/* a name is there, not just :T...  */
		{
		  if (pst != nullptr)
		    pst->add_psymbol (std::string_view (sym_name, sym_len),
				      true, VAR_DOMAIN, LOC_TYPEDEF, -1,
				      psymbol_placement::STATIC,
				      unrelocated_addr (0),
				      psymtab_language,
				      partial_symtabs, objfile);
		  else
		    complaint (_("typename `%*s' appears to be defined "
				 "outside of all compilation units"),
			       sym_len, sym_name);
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
		  /* The aix4 compiler emits extra crud before the members.  */
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
		      if (pst != nullptr)
			pst->add_psymbol (std::string_view (p, q - p), true,
					  VAR_DOMAIN, LOC_CONST, -1,
					  psymbol_placement::STATIC,
					  unrelocated_addr (0),
					  psymtab_language,
					  partial_symtabs, objfile);
		      else
			complaint (_("enum constant `%*s' appears to be defined "
				     "outside of all compilation units"),
				   ((int) (q - p)), p);
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
	      if (pst != nullptr)
		pst->add_psymbol (std::string_view (sym_name, sym_len), true,
				  VAR_DOMAIN, LOC_CONST, -1,
				  psymbol_placement::STATIC,
				  unrelocated_addr (0),
				  psymtab_language,
				  partial_symtabs, objfile);
	      else
		complaint (_("constant `%*s' appears to be defined "
			     "outside of all compilation units"),
			   sym_len, sym_name);

	      continue;

	    case 'f':
	      if (! pst)
		{
		  std::string name (namestring, (p - namestring));
		  function_outside_compilation_unit_complaint (name.c_str ());
		}
	      /* Kludges for ELF/STABS with Sun ACC.  */
	      last_function_name = namestring;
	      /* Do not fix textlow==0 for .o or NLM files, as 0 is a legit
		 value for the bottom of the text seg in those cases.  */
	      if (nlist.n_value == 0
		  && gdbarch_sofun_address_maybe_missing (gdbarch))
		{
		  struct bound_minimal_symbol minsym
		    = find_stab_function (namestring,
					  pst ? pst->filename : NULL,
					  objfile);
		  if (minsym.minsym != NULL)
		    nlist.n_value
		      = CORE_ADDR (minsym.minsym->unrelocated_address ());
		}
	      if (pst && textlow_not_set
		  && gdbarch_sofun_address_maybe_missing (gdbarch))
		{
		  pst->set_text_low (unrelocated_addr (nlist.n_value));
		  textlow_not_set = 0;
		}
	      /* End kludge.  */

	      /* Keep track of the start of the last function so we
		 can handle end of function symbols.  */
	      last_function_start = nlist.n_value;

	      /* In reordered executables this function may lie outside
		 the bounds created by N_SO symbols.  If that's the case
		 use the address of this function as the low bound for
		 the partial symbol table.  */
	      if (pst
		  && (textlow_not_set
		      || (unrelocated_addr (nlist.n_value)
			  < pst->unrelocated_text_low ()
			  && (nlist.n_value != 0))))
		{
		  pst->set_text_low (unrelocated_addr (nlist.n_value));
		  textlow_not_set = 0;
		}
	      if (pst != nullptr)
		pst->add_psymbol (std::string_view (sym_name, sym_len), true,
				  VAR_DOMAIN, LOC_BLOCK,
				  SECT_OFF_TEXT (objfile),
				  psymbol_placement::STATIC,
				  unrelocated_addr (nlist.n_value),
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
	      /* Kludges for ELF/STABS with Sun ACC.  */
	      last_function_name = namestring;
	      /* Do not fix textlow==0 for .o or NLM files, as 0 is a legit
		 value for the bottom of the text seg in those cases.  */
	      if (nlist.n_value == 0
		  && gdbarch_sofun_address_maybe_missing (gdbarch))
		{
		  struct bound_minimal_symbol minsym
		    = find_stab_function (namestring,
					  pst ? pst->filename : NULL,
					  objfile);
		  if (minsym.minsym != NULL)
		    nlist.n_value
		      = CORE_ADDR (minsym.minsym->unrelocated_address ());
		}
	      if (pst && textlow_not_set
		  && gdbarch_sofun_address_maybe_missing (gdbarch))
		{
		  pst->set_text_low (unrelocated_addr (nlist.n_value));
		  textlow_not_set = 0;
		}
	      /* End kludge.  */

	      /* Keep track of the start of the last function so we
		 can handle end of function symbols.  */
	      last_function_start = nlist.n_value;

	      /* In reordered executables this function may lie outside
		 the bounds created by N_SO symbols.  If that's the case
		 use the address of this function as the low bound for
		 the partial symbol table.  */
	      if (pst
		  && (textlow_not_set
		      || (unrelocated_addr (nlist.n_value)
			  < pst->unrelocated_text_low ()
			  && (nlist.n_value != 0))))
		{
		  pst->set_text_low (unrelocated_addr (nlist.n_value));
		  textlow_not_set = 0;
		}
	      if (pst != nullptr)
		pst->add_psymbol (std::string_view (sym_name, sym_len), true,
				  VAR_DOMAIN, LOC_BLOCK,
				  SECT_OFF_TEXT (objfile),
				  psymbol_placement::GLOBAL,
				  unrelocated_addr (nlist.n_value),
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
	    case '#':	/* For symbol identification (used in live ranges).  */
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
	      /* Unexpected symbol descriptor.  The second and subsequent stabs
		 of a continued stab can show up here.  The question is
		 whether they ever can mimic a normal stab--it would be
		 nice if not, since we certainly don't want to spend the
		 time searching to the end of every string looking for
		 a backslash.  */

	      complaint (_("unknown symbol descriptor `%c'"),
			 p[1]);

	      /* Ignore it; perhaps it is an extension that we don't
		 know about.  */
	      continue;
	    }
	}

	case N_EXCL:

	  namestring = set_namestring (objfile, &nlist);

	  /* Find the corresponding bincl and mark that psymtab on the
	     psymtab dependency list.  */
	  {
	    legacy_psymtab *needed_pst =
	      find_corresponding_bincl_psymtab (namestring, nlist.n_value);

	    /* If this include file was defined earlier in this file,
	       leave it alone.  */
	    if (needed_pst == pst)
	      continue;

	    if (needed_pst)
	      {
		int i;
		int found = 0;

		for (i = 0; i < dependencies_used; i++)
		  if (dependency_list[i] == needed_pst)
		    {
		      found = 1;
		      break;
		    }

		/* If it's already in the list, skip the rest.  */
		if (found)
		  continue;

		dependency_list[dependencies_used++] = needed_pst;
		if (dependencies_used >= dependencies_allocated)
		  {
		    legacy_psymtab **orig = dependency_list;

		    dependency_list =
		      (legacy_psymtab **)
		      alloca ((dependencies_allocated *= 2)
			      * sizeof (legacy_psymtab *));
		    memcpy (dependency_list, orig,
			    (dependencies_used
			     * sizeof (legacy_psymtab *)));
#ifdef DEBUG_INFO
		    gdb_printf (gdb_stderr,
				"Had to reallocate "
				"dependency list.\n");
		    gdb_printf (gdb_stderr,
				"New dependencies allocated: %d\n",
				dependencies_allocated);
#endif
		  }
	      }
	  }
	  continue;

	case N_ENDM:
	  /* Solaris 2 end of module, finish current partial symbol
	     table.  dbx_end_psymtab will set the high text address of
	     PST to the proper value, which is necessary if a module
	     compiled without debugging info follows this module.  */
	  if (pst && gdbarch_sofun_address_maybe_missing (gdbarch))
	    {
	      dbx_end_psymtab (objfile, partial_symtabs, pst,
			       psymtab_include_list, includes_used,
			       symnum * symbol_size,
			       (unrelocated_addr) 0, dependency_list,
			       dependencies_used, textlow_not_set);
	      pst = (legacy_psymtab *) 0;
	      includes_used = 0;
	      dependencies_used = 0;
	      has_line_numbers = 0;
	    }
	  continue;

	case N_RBRAC:
#ifdef HANDLE_RBRAC
	  HANDLE_RBRAC (nlist.n_value);
	  continue;
#endif
	case N_EINCL:
	case N_DSLINE:
	case N_BSLINE:
	case N_SSYM:		/* Claim: Structure or union element.
				   Hopefully, I can ignore this.  */
	case N_ENTRY:		/* Alternate entry point; can ignore.  */
	case N_MAIN:		/* Can definitely ignore this.   */
	case N_CATCH:		/* These are GNU C++ extensions */
	case N_EHDECL:		/* that can safely be ignored here.  */
	case N_LENG:
	case N_BCOMM:
	case N_ECOMM:
	case N_ECOML:
	case N_FNAME:
	case N_SLINE:
	case N_RSYM:
	case N_PSYM:
	case N_BNSYM:
	case N_ENSYM:
	case N_LBRAC:
	case N_NSYMS:		/* Ultrix 4.0: symbol count */
	case N_DEFD:		/* GNU Modula-2 */
	case N_ALIAS:		/* SunPro F77: alias name, ignore for now.  */

	case N_OBJ:		/* Useless types from Solaris.  */
	case N_OPT:
	case N_PATCH:
	  /* These symbols aren't interesting; don't worry about them.  */
	  continue;

	default:
	  /* If we haven't found it yet, ignore it.  It's probably some
	     new type we don't know about yet.  */
	  unknown_symtype_complaint (hex_string (nlist.n_type));
	  continue;
	}
    }

  /* If there's stuff to be cleaned up, clean it up.  */
  if (pst)
    {
      /* Don't set high text address of PST lower than it already
	 is.  */
      unrelocated_addr text_end
	= (unrelocated_addr
	   ((lowest_text_address == (unrelocated_addr) -1
	     ? text_addr
	     : CORE_ADDR (lowest_text_address))
	    + text_size));

      dbx_end_psymtab (objfile, partial_symtabs,
		       pst, psymtab_include_list, includes_used,
		       symnum * symbol_size,
		       (text_end > pst->unrelocated_text_high ()
			? text_end : pst->unrelocated_text_high ()),
		       dependency_list, dependencies_used, textlow_not_set);
    }
}

/* Allocate and partially fill a partial symtab.  It will be
   completely filled at the end of the symbol list.

   SYMFILE_NAME is the name of the symbol-file we are reading from, and ADDR
   is the address relative to which its symbols are (incremental) or 0
   (normal).  */

static legacy_psymtab *
start_psymtab (psymtab_storage *partial_symtabs, struct objfile *objfile,
	       const char *filename, unrelocated_addr textlow, int ldsymoff)
{
  legacy_psymtab *result = new legacy_psymtab (filename, partial_symtabs,
					       objfile->per_bfd, textlow);

  result->read_symtab_private =
    XOBNEW (&objfile->objfile_obstack, struct symloc);
  LDSYMOFF (result) = ldsymoff;
  result->legacy_read_symtab = dbx_read_symtab;
  result->legacy_expand_psymtab = dbx_expand_psymtab;
  SYMBOL_SIZE (result) = symbol_size;
  SYMBOL_OFFSET (result) = symbol_table_offset;
  STRING_OFFSET (result) = string_table_offset;
  FILE_STRING_OFFSET (result) = file_string_table_offset;

  /* Deduce the source language from the filename for this psymtab.  */
  psymtab_language = deduce_language_from_filename (filename);
  PST_LANGUAGE (result) = psymtab_language;

  return result;
}

/* Close off the current usage of PST.
   Returns PST or NULL if the partial symtab was empty and thrown away.

   FIXME:  List variables and peculiarities of same.  */

legacy_psymtab *
dbx_end_psymtab (struct objfile *objfile, psymtab_storage *partial_symtabs,
		 legacy_psymtab *pst,
		 const char **include_list, int num_includes,
		 int capping_symbol_offset, unrelocated_addr capping_text,
		 legacy_psymtab **dependency_list,
		 int number_dependencies,
		 int textlow_not_set)
{
  int i;
  struct gdbarch *gdbarch = objfile->arch ();

  if (capping_symbol_offset != -1)
    LDSYMLEN (pst) = capping_symbol_offset - LDSYMOFF (pst);
  pst->set_text_high (capping_text);

  /* Under Solaris, the N_SO symbols always have a value of 0,
     instead of the usual address of the .o file.  Therefore,
     we have to do some tricks to fill in texthigh and textlow.
     The first trick is: if we see a static
     or global function, and the textlow for the current pst
     is not set (ie: textlow_not_set), then we use that function's
     address for the textlow of the pst.  */

  /* Now, to fill in texthigh, we remember the last function seen
     in the .o file.  Also, there's a hack in
     bfd/elf.c and gdb/elfread.c to pass the ELF st_size field
     to here via the misc_info field.  Therefore, we can fill in
     a reliable texthigh by taking the address plus size of the
     last function in the file.  */

  if (!pst->text_high_valid && last_function_name
      && gdbarch_sofun_address_maybe_missing (gdbarch))
    {
      int n;
      struct bound_minimal_symbol minsym;

      const char *colon = strchr (last_function_name, ':');
      if (colon == NULL)
	n = 0;
      else
	n = colon - last_function_name;
      char *p = (char *) alloca (n + 2);
      strncpy (p, last_function_name, n);
      p[n] = 0;

      minsym = lookup_minimal_symbol (p, pst->filename, objfile);
      if (minsym.minsym == NULL)
	{
	  /* Sun Fortran appends an underscore to the minimal symbol name,
	     try again with an appended underscore if the minimal symbol
	     was not found.  */
	  p[n] = '_';
	  p[n + 1] = 0;
	  minsym = lookup_minimal_symbol (p, pst->filename, objfile);
	}

      if (minsym.minsym)
	pst->set_text_high
	  (unrelocated_addr (CORE_ADDR (minsym.minsym->unrelocated_address ())
			     + minsym.minsym->size ()));

      last_function_name = NULL;
    }

  if (!gdbarch_sofun_address_maybe_missing (gdbarch))
    ;
  /* This test will be true if the last .o file is only data.  */
  else if (textlow_not_set)
    pst->set_text_low (pst->unrelocated_text_high ());
  else
    {
      /* If we know our own starting text address, then walk through all other
	 psymtabs for this objfile, and if any didn't know their ending text
	 address, set it to our starting address.  Take care to not set our
	 own ending address to our starting address.  */

      for (partial_symtab *p1 : partial_symtabs->range ())
	if (!p1->text_high_valid && p1->text_low_valid && p1 != pst)
	  p1->set_text_high (pst->unrelocated_text_low ());
    }

  /* End of kludge for patching Solaris textlow and texthigh.  */

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

      subpst->read_symtab_private =
	XOBNEW (&objfile->objfile_obstack, struct symloc);
      LDSYMOFF (subpst) =
	LDSYMLEN (subpst) = 0;

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
      && pst->empty ()
      && has_line_numbers == 0)
    {
      /* Throw away this psymtab, it's empty.  */
      /* Empty psymtabs happen as a result of header files which don't have
	 any symbols in them.  There can be a lot of them.  But this check
	 is wrong, in that a psymtab with N_SLINE entries but nothing else
	 is not empty, but we don't realize that.  Fixing that without slowing
	 things down might be tricky.  */

      partial_symtabs->discard_psymtab (pst);

      /* Indicate that psymtab was thrown away.  */
      pst = NULL;
    }
  return pst;
}

static void
dbx_expand_psymtab (legacy_psymtab *pst, struct objfile *objfile)
{
  gdb_assert (!pst->readin);

  /* Read in all partial symtabs on which this one is dependent.  */
  pst->expand_dependencies (objfile);

  if (LDSYMLEN (pst))		/* Otherwise it's a dummy.  */
    {
      /* Init stuff necessary for reading in symbols */
      stabsread_init ();
      scoped_free_pendings free_pending;
      file_string_table_offset = FILE_STRING_OFFSET (pst);
      symbol_size = SYMBOL_SIZE (pst);

      /* Read in this file's symbols.  */
      if (bfd_seek (objfile->obfd.get (), SYMBOL_OFFSET (pst), SEEK_SET) == 0)
	read_ofile_symtab (objfile, pst);
    }

  pst->readin = true;
}

/* Read in all of the symbols for a given psymtab for real.
   Be verbose about it if the user wants that.  SELF is not NULL.  */

static void
dbx_read_symtab (legacy_psymtab *self, struct objfile *objfile)
{
  gdb_assert (!self->readin);

  if (LDSYMLEN (self) || self->number_of_dependencies)
    {
      next_symbol_text_func = dbx_next_symbol_text;

      {
	scoped_restore restore_stabs_data = make_scoped_restore (&stabs_data);
	gdb::unique_xmalloc_ptr<gdb_byte> data_holder;
	if (DBX_STAB_SECTION (objfile))
	  {
	    stabs_data
	      = symfile_relocate_debug_section (objfile,
						DBX_STAB_SECTION (objfile),
						NULL);
	    data_holder.reset (stabs_data);
	  }

	self->expand_psymtab (objfile);
      }

      /* Match with global symbols.  This only needs to be done once,
	 after all of the symtabs and dependencies have been read in.   */
      scan_file_globals (objfile);
    }
}

/* Read in a defined section of a specific object file's symbols.  */

static void
read_ofile_symtab (struct objfile *objfile, legacy_psymtab *pst)
{
  const char *namestring;
  struct external_nlist *bufp;
  struct internal_nlist nlist;
  unsigned char type;
  unsigned max_symnum;
  bfd *abfd;
  int sym_offset;		/* Offset to start of symbols to read */
  int sym_size;			/* Size of symbols to read */
  CORE_ADDR text_offset;	/* Start of text segment for symbols */
  int text_size;		/* Size of text segment for symbols */

  sym_offset = LDSYMOFF (pst);
  sym_size = LDSYMLEN (pst);
  text_offset = pst->text_low (objfile);
  text_size = pst->text_high (objfile) - pst->text_low (objfile);
  const section_offsets &section_offsets = objfile->section_offsets;

  dbxread_objfile = objfile;

  stringtab_global = DBX_STRINGTAB (objfile);
  set_last_source_file (NULL);

  abfd = objfile->obfd.get ();
  symfile_bfd = objfile->obfd.get ();	/* Implicit param to next_text_symbol.  */
  symbuf_end = symbuf_idx = 0;
  symbuf_read = 0;
  symbuf_left = sym_offset + sym_size;

  /* It is necessary to actually read one symbol *before* the start
     of this symtab's symbols, because the GCC_COMPILED_FLAG_SYMBOL
     occurs before the N_SO symbol.

     Detecting this in read_dbx_symtab
     would slow down initial readin, so we look for it here instead.  */
  if (!processing_acc_compilation && sym_offset >= (int) symbol_size)
    {
      stabs_seek (sym_offset - symbol_size);
      fill_symbuf (abfd);
      bufp = &symbuf[symbuf_idx++];
      INTERNALIZE_SYMBOL (nlist, bufp, abfd);
      OBJSTAT (objfile, n_stabs++);

      namestring = set_namestring (objfile, &nlist);

      processing_gcc_compilation = 0;
      if (nlist.n_type == N_TEXT)
	{
	  const char *tempstring = namestring;

	  if (strcmp (namestring, GCC_COMPILED_FLAG_SYMBOL) == 0)
	    processing_gcc_compilation = 1;
	  else if (strcmp (namestring, GCC2_COMPILED_FLAG_SYMBOL) == 0)
	    processing_gcc_compilation = 2;
	  if (*tempstring != '\0'
	      && *tempstring == bfd_get_symbol_leading_char (symfile_bfd))
	    ++tempstring;
	  if (startswith (tempstring, "__gnu_compiled"))
	    processing_gcc_compilation = 2;
	}
    }
  else
    {
      /* The N_SO starting this symtab is the first symbol, so we
	 better not check the symbol before it.  I'm not this can
	 happen, but it doesn't hurt to check for it.  */
      stabs_seek (sym_offset);
      processing_gcc_compilation = 0;
    }

  if (symbuf_idx == symbuf_end)
    fill_symbuf (abfd);
  bufp = &symbuf[symbuf_idx];
  if (bfd_h_get_8 (abfd, bufp->e_type) != N_SO)
    error (_("First symbol in segment of executable not a source symbol"));

  max_symnum = sym_size / symbol_size;

  for (symnum = 0;
       symnum < max_symnum;
       symnum++)
    {
      QUIT;			/* Allow this to be interruptable.  */
      if (symbuf_idx == symbuf_end)
	fill_symbuf (abfd);
      bufp = &symbuf[symbuf_idx++];
      INTERNALIZE_SYMBOL (nlist, bufp, abfd);
      OBJSTAT (objfile, n_stabs++);

      type = bfd_h_get_8 (abfd, bufp->e_type);

      namestring = set_namestring (objfile, &nlist);

      if (type & N_STAB)
	{
	  if (sizeof (nlist.n_value) > 4
	      /* We are a 64-bit debugger debugging a 32-bit program.  */
	      && (type == N_LSYM || type == N_PSYM))
	      /* We have to be careful with the n_value in the case of N_LSYM
		 and N_PSYM entries, because they are signed offsets from frame
		 pointer, but we actually read them as unsigned 32-bit values.
		 This is not a problem for 32-bit debuggers, for which negative
		 values end up being interpreted correctly (as negative
		 offsets) due to integer overflow.
		 But we need to sign-extend the value for 64-bit debuggers,
		 or we'll end up interpreting negative values as very large
		 positive offsets.  */
	    nlist.n_value = (nlist.n_value ^ 0x80000000) - 0x80000000;
	  process_one_symbol (type, nlist.n_desc, nlist.n_value,
			      namestring, section_offsets, objfile,
			      PST_LANGUAGE (pst));
	}
      /* We skip checking for a new .o or -l file; that should never
	 happen in this routine.  */
      else if (type == N_TEXT)
	{
	  /* I don't think this code will ever be executed, because
	     the GCC_COMPILED_FLAG_SYMBOL usually is right before
	     the N_SO symbol which starts this source file.
	     However, there is no reason not to accept
	     the GCC_COMPILED_FLAG_SYMBOL anywhere.  */

	  if (strcmp (namestring, GCC_COMPILED_FLAG_SYMBOL) == 0)
	    processing_gcc_compilation = 1;
	  else if (strcmp (namestring, GCC2_COMPILED_FLAG_SYMBOL) == 0)
	    processing_gcc_compilation = 2;
	}
      else if (type & N_EXT || type == (unsigned char) N_TEXT
	       || type == (unsigned char) N_NBTEXT)
	{
	  /* Global symbol: see if we came across a dbx definition for
	     a corresponding symbol.  If so, store the value.  Remove
	     syms from the chain when their values are stored, but
	     search the whole chain, as there may be several syms from
	     different files with the same name.  */
	  /* This is probably not true.  Since the files will be read
	     in one at a time, each reference to a global symbol will
	     be satisfied in each file as it appears.  So we skip this
	     section.  */
	  ;
	}
    }

  /* In a Solaris elf file, this variable, which comes from the value
     of the N_SO symbol, will still be 0.  Luckily, text_offset, which
     comes from low text address of PST, is correct.  */
  if (get_last_source_start_addr () == 0)
    set_last_source_start_addr (text_offset);

  /* In reordered executables last_source_start_addr may not be the
     lower bound for this symtab, instead use text_offset which comes
     from the low text address of PST, which is correct.  */
  if (get_last_source_start_addr () > text_offset)
    set_last_source_start_addr (text_offset);

  pst->compunit_symtab = end_compunit_symtab (text_offset + text_size);

  end_stabs ();

  dbxread_objfile = NULL;
}


/* Record the namespace that the function defined by SYMBOL was
   defined in, if necessary.  BLOCK is the associated block; use
   OBSTACK for allocation.  */

static void
cp_set_block_scope (const struct symbol *symbol,
		    struct block *block,
		    struct obstack *obstack)
{
  if (symbol->demangled_name () != NULL)
    {
      /* Try to figure out the appropriate namespace from the
	 demangled name.  */

      /* FIXME: carlton/2003-04-15: If the function in question is
	 a method of a class, the name will actually include the
	 name of the class as well.  This should be harmless, but
	 is a little unfortunate.  */

      const char *name = symbol->demangled_name ();
      unsigned int prefix_len = cp_entire_prefix_len (name);

      block->set_scope (obstack_strndup (obstack, name, prefix_len),
			obstack);
    }
}

/* This handles a single symbol from the symbol-file, building symbols
   into a GDB symtab.  It takes these arguments and an implicit argument.

   TYPE is the type field of the ".stab" symbol entry.
   DESC is the desc field of the ".stab" entry.
   VALU is the value field of the ".stab" entry.
   NAME is the symbol name, in our address space.
   SECTION_OFFSETS is a set of amounts by which the sections of this
   object file were relocated when it was loaded into memory.  Note
   that these section_offsets are not the objfile->section_offsets but
   the pst->section_offsets.  All symbols that refer to memory
   locations need to be offset by these amounts.
   OBJFILE is the object file from which we are reading symbols.  It
   is used in end_compunit_symtab.
   LANGUAGE is the language of the symtab.
*/

void
process_one_symbol (int type, int desc, CORE_ADDR valu, const char *name,
		    const section_offsets &section_offsets,
		    struct objfile *objfile, enum language language)
{
  struct gdbarch *gdbarch = objfile->arch ();
  struct context_stack *newobj;
  struct context_stack cstk;
  /* This remembers the address of the start of a function.  It is
     used because in Solaris 2, N_LBRAC, N_RBRAC, and N_SLINE entries
     are relative to the current function's start address.  On systems
     other than Solaris 2, this just holds the SECT_OFF_TEXT value,
     and is used to relocate these symbol types rather than
     SECTION_OFFSETS.  */
  static CORE_ADDR function_start_offset;

  /* This holds the address of the start of a function, without the
     system peculiarities of function_start_offset.  */
  static CORE_ADDR last_function_start;

  /* If this is nonzero, we've seen an N_SLINE since the start of the
     current function.  We use this to tell us to move the first sline
     to the beginning of the function regardless of what its given
     value is.  */
  static int sline_found_in_function = 1;

  /* If this is nonzero, we've seen a non-gcc N_OPT symbol for this
     source file.  Used to detect the SunPRO solaris compiler.  */
  static int n_opt_found;

  /* The section index for this symbol.  */
  int section_index = -1;

  /* Something is wrong if we see real data before seeing a source
     file name.  */

  if (get_last_source_file () == NULL && type != (unsigned char) N_SO)
    {
      /* Ignore any symbols which appear before an N_SO symbol.
	 Currently no one puts symbols there, but we should deal
	 gracefully with the case.  A complain()t might be in order,
	 but this should not be an error ().  */
      return;
    }

  switch (type)
    {
    case N_FUN:
    case N_FNAME:

      if (*name == '\000')
	{
	  /* This N_FUN marks the end of a function.  This closes off
	     the current block.  */
	  struct block *block;

	  if (outermost_context_p ())
	    {
	      lbrac_mismatch_complaint (symnum);
	      break;
	    }

	  /* The following check is added before recording line 0 at
	     end of function so as to handle hand-generated stabs
	     which may have an N_FUN stabs at the end of the function,
	     but no N_SLINE stabs.  */
	  if (sline_found_in_function)
	    {
	      CORE_ADDR addr = last_function_start + valu;

	      record_line
		(get_current_subfile (), 0,
		 unrelocated_addr (gdbarch_addr_bits_remove (gdbarch, addr)
				   - objfile->text_section_offset ()));
	    }

	  within_function = 0;
	  cstk = pop_context ();

	  /* Make a block for the local symbols within.  */
	  block = finish_block (cstk.name,
				cstk.old_blocks, NULL,
				cstk.start_addr, cstk.start_addr + valu);

	  /* For C++, set the block's scope.  */
	  if (cstk.name->language () == language_cplus)
	    cp_set_block_scope (cstk.name, block, &objfile->objfile_obstack);

	  /* May be switching to an assembler file which may not be using
	     block relative stabs, so reset the offset.  */
	  function_start_offset = 0;

	  break;
	}

      sline_found_in_function = 0;

      /* Relocate for dynamic loading.  */
      section_index = SECT_OFF_TEXT (objfile);
      valu += section_offsets[SECT_OFF_TEXT (objfile)];
      valu = gdbarch_addr_bits_remove (gdbarch, valu);
      last_function_start = valu;

      goto define_a_symbol;

    case N_LBRAC:
      /* This "symbol" just indicates the start of an inner lexical
	 context within a function.  */

      /* Ignore extra outermost context from SunPRO cc and acc.  */
      if (n_opt_found && desc == 1)
	break;

      valu += function_start_offset;

      push_context (desc, valu);
      break;

    case N_RBRAC:
      /* This "symbol" just indicates the end of an inner lexical
	 context that was started with N_LBRAC.  */

      /* Ignore extra outermost context from SunPRO cc and acc.  */
      if (n_opt_found && desc == 1)
	break;

      valu += function_start_offset;

      if (outermost_context_p ())
	{
	  lbrac_mismatch_complaint (symnum);
	  break;
	}

      cstk = pop_context ();
      if (desc != cstk.depth)
	lbrac_mismatch_complaint (symnum);

      if (*get_local_symbols () != NULL)
	{
	  /* GCC development snapshots from March to December of
	     2000 would output N_LSYM entries after N_LBRAC
	     entries.  As a consequence, these symbols are simply
	     discarded.  Complain if this is the case.  */
	  complaint (_("misplaced N_LBRAC entry; discarding local "
		       "symbols which have no enclosing block"));
	}
      *get_local_symbols () = cstk.locals;

      if (get_context_stack_depth () > 1)
	{
	  /* This is not the outermost LBRAC...RBRAC pair in the
	     function, its local symbols preceded it, and are the ones
	     just recovered from the context stack.  Define the block
	     for them (but don't bother if the block contains no
	     symbols.  Should we complain on blocks without symbols?
	     I can't think of any useful purpose for them).  */
	  if (*get_local_symbols () != NULL)
	    {
	      /* Muzzle a compiler bug that makes end < start.

		 ??? Which compilers?  Is this ever harmful?.  */
	      if (cstk.start_addr > valu)
		{
		  complaint (_("block start larger than block end"));
		  cstk.start_addr = valu;
		}
	      /* Make a block for the local symbols within.  */
	      finish_block (0, cstk.old_blocks, NULL,
			    cstk.start_addr, valu);
	    }
	}
      else
	{
	  /* This is the outermost LBRAC...RBRAC pair.  There is no
	     need to do anything; leave the symbols that preceded it
	     to be attached to the function's own block.  We need to
	     indicate that we just moved outside of the function.  */
	  within_function = 0;
	}

      break;

    case N_FN:
    case N_FN_SEQ:
      /* This kind of symbol indicates the start of an object file.
	 Relocate for dynamic loading.  */
      section_index = SECT_OFF_TEXT (objfile);
      valu += section_offsets[SECT_OFF_TEXT (objfile)];
      break;

    case N_SO:
      /* This type of symbol indicates the start of data for one
	 source file.  Finish the symbol table of the previous source
	 file (if any) and start accumulating a new symbol table.
	 Relocate for dynamic loading.  */
      section_index = SECT_OFF_TEXT (objfile);
      valu += section_offsets[SECT_OFF_TEXT (objfile)];

      n_opt_found = 0;

      if (get_last_source_file ())
	{
	  /* Check if previous symbol was also an N_SO (with some
	     sanity checks).  If so, that one was actually the
	     directory name, and the current one is the real file
	     name.  Patch things up.  */
	  if (previous_stab_code == (unsigned char) N_SO)
	    {
	      patch_subfile_names (get_current_subfile (), name);
	      break;		/* Ignore repeated SOs.  */
	    }
	  end_compunit_symtab (valu);
	  end_stabs ();
	}

      /* Null name means this just marks the end of text for this .o
	 file.  Don't start a new symtab in this case.  */
      if (*name == '\000')
	break;

      function_start_offset = 0;

      start_stabs ();
      start_compunit_symtab (objfile, name, NULL, valu, language);
      record_debugformat ("stabs");
      break;

    case N_SOL:
      /* This type of symbol indicates the start of data for a
	 sub-source-file, one whose contents were copied or included
	 in the compilation of the main source file (whose name was
	 given in the N_SO symbol).  Relocate for dynamic loading.  */
      section_index = SECT_OFF_TEXT (objfile);
      valu += section_offsets[SECT_OFF_TEXT (objfile)];
      start_subfile (name);
      break;

    case N_BINCL:
      push_subfile ();
      add_new_header_file (name, valu);
      start_subfile (name);
      break;

    case N_EINCL:
      start_subfile (pop_subfile ());
      break;

    case N_EXCL:
      add_old_header_file (name, valu);
      break;

    case N_SLINE:
      /* This type of "symbol" really just records one line-number --
	 core-address correspondence.  Enter it in the line list for
	 this symbol table.  */

      /* Relocate for dynamic loading and for ELF acc
	 function-relative symbols.  */
      valu += function_start_offset;

      /* GCC 2.95.3 emits the first N_SLINE stab somewhere in the
	 middle of the prologue instead of right at the start of the
	 function.  To deal with this we record the address for the
	 first N_SLINE stab to be the start of the function instead of
	 the listed location.  We really shouldn't to this.  When
	 compiling with optimization, this first N_SLINE stab might be
	 optimized away.  Other (non-GCC) compilers don't emit this
	 stab at all.  There is no real harm in having an extra
	 numbered line, although it can be a bit annoying for the
	 user.  However, it totally screws up our testsuite.

	 So for now, keep adjusting the address of the first N_SLINE
	 stab, but only for code compiled with GCC.  */

      if (within_function && sline_found_in_function == 0)
	{
	  CORE_ADDR addr = processing_gcc_compilation == 2 ?
			   last_function_start : valu;

	  record_line
	    (get_current_subfile (), desc,
	     unrelocated_addr (gdbarch_addr_bits_remove (gdbarch, addr)
			       - objfile->text_section_offset ()));
	  sline_found_in_function = 1;
	}
      else
	record_line
	  (get_current_subfile (), desc,
	   unrelocated_addr (gdbarch_addr_bits_remove (gdbarch, valu)
			     - objfile->text_section_offset ()));
      break;

    case N_BCOMM:
      common_block_start (name, objfile);
      break;

    case N_ECOMM:
      common_block_end (objfile);
      break;

      /* The following symbol types need to have the appropriate
	 offset added to their value; then we process symbol
	 definitions in the name.  */

    case N_STSYM:		/* Static symbol in data segment.  */
    case N_LCSYM:		/* Static symbol in BSS segment.  */
    case N_ROSYM:		/* Static symbol in read-only data segment.  */
      /* HORRID HACK DEPT.  However, it's Sun's furgin' fault.
	 Solaris 2's stabs-in-elf makes *most* symbols relative but
	 leaves a few absolute (at least for Solaris 2.1 and version
	 2.0.1 of the SunPRO compiler).  N_STSYM and friends sit on
	 the fence.  .stab "foo:S...",N_STSYM is absolute (ld
	 relocates it) .stab "foo:V...",N_STSYM is relative (section
	 base subtracted).  This leaves us no choice but to search for
	 the 'S' or 'V'...  (or pass the whole section_offsets stuff
	 down ONE MORE function call level, which we really don't want
	 to do).  */
      {
	const char *p;

	/* Normal object file and NLMs have non-zero text seg offsets,
	   but don't need their static syms offset in this fashion.
	   XXX - This is really a crock that should be fixed in the
	   solib handling code so that I don't have to work around it
	   here.  */

	if (!symfile_relocatable)
	  {
	    p = strchr (name, ':');
	    if (p != 0 && p[1] == 'S')
	      {
		/* The linker relocated it.  We don't want to add a
		   Sun-stabs Tfoo.foo-like offset, but we *do*
		   want to add whatever solib.c passed to
		   symbol_file_add as addr (this is known to affect
		   SunOS 4, and I suspect ELF too).  Since there is no
		   Ttext.text symbol, we can get addr from the text offset.  */
		section_index = SECT_OFF_TEXT (objfile);
		valu += section_offsets[SECT_OFF_TEXT (objfile)];
		goto define_a_symbol;
	      }
	  }
	/* Since it's not the kludge case, re-dispatch to the right
	   handler.  */
	switch (type)
	  {
	  case N_STSYM:
	    goto case_N_STSYM;
	  case N_LCSYM:
	    goto case_N_LCSYM;
	  case N_ROSYM:
	    goto case_N_ROSYM;
	  default:
	    internal_error (_("failed internal consistency check"));
	  }
      }

    case_N_STSYM:		/* Static symbol in data segment.  */
    case N_DSLINE:		/* Source line number, data segment.  */
      section_index = SECT_OFF_DATA (objfile);
      valu += section_offsets[SECT_OFF_DATA (objfile)];
      goto define_a_symbol;

    case_N_LCSYM:		/* Static symbol in BSS segment.  */
    case N_BSLINE:		/* Source line number, BSS segment.  */
      /* N_BROWS: overlaps with N_BSLINE.  */
      section_index = SECT_OFF_BSS (objfile);
      valu += section_offsets[SECT_OFF_BSS (objfile)];
      goto define_a_symbol;

    case_N_ROSYM:		/* Static symbol in read-only data segment.  */
      section_index = SECT_OFF_RODATA (objfile);
      valu += section_offsets[SECT_OFF_RODATA (objfile)];
      goto define_a_symbol;

    case N_ENTRY:		/* Alternate entry point.  */
      /* Relocate for dynamic loading.  */
      section_index = SECT_OFF_TEXT (objfile);
      valu += section_offsets[SECT_OFF_TEXT (objfile)];
      goto define_a_symbol;

      /* The following symbol types we don't know how to process.
	 Handle them in a "default" way, but complain to people who
	 care.  */
    default:
    case N_CATCH:		/* Exception handler catcher.  */
    case N_EHDECL:		/* Exception handler name.  */
    case N_PC:			/* Global symbol in Pascal.  */
    case N_M2C:			/* Modula-2 compilation unit.  */
      /* N_MOD2: overlaps with N_EHDECL.  */
    case N_SCOPE:		/* Modula-2 scope information.  */
    case N_ECOML:		/* End common (local name).  */
    case N_NBTEXT:		/* Gould Non-Base-Register symbols???  */
    case N_NBDATA:
    case N_NBBSS:
    case N_NBSTS:
    case N_NBLCS:
      unknown_symtype_complaint (hex_string (type));

    define_a_symbol:
      [[fallthrough]];
      /* These symbol types don't need the address field relocated,
	 since it is either unused, or is absolute.  */
    case N_GSYM:		/* Global variable.  */
    case N_NSYMS:		/* Number of symbols (Ultrix).  */
    case N_NOMAP:		/* No map?  (Ultrix).  */
    case N_RSYM:		/* Register variable.  */
    case N_DEFD:		/* Modula-2 GNU module dependency.  */
    case N_SSYM:		/* Struct or union element.  */
    case N_LSYM:		/* Local symbol in stack.  */
    case N_PSYM:		/* Parameter variable.  */
    case N_LENG:		/* Length of preceding symbol type.  */
      if (name)
	{
	  int deftype;
	  const char *colon_pos = strchr (name, ':');

	  if (colon_pos == NULL)
	    deftype = '\0';
	  else
	    deftype = colon_pos[1];

	  switch (deftype)
	    {
	    case 'f':
	    case 'F':
	      /* Deal with the SunPRO 3.0 compiler which omits the
		 address from N_FUN symbols.  */
	      if (type == N_FUN
		  && valu == section_offsets[SECT_OFF_TEXT (objfile)]
		  && gdbarch_sofun_address_maybe_missing (gdbarch))
		{
		  struct bound_minimal_symbol minsym
		    = find_stab_function (name, get_last_source_file (),
					  objfile);
		  if (minsym.minsym != NULL)
		    valu = minsym.value_address ();
		}

	      /* These addresses are absolute.  */
	      function_start_offset = valu;

	      within_function = 1;

	      if (get_context_stack_depth () > 1)
		{
		  complaint (_("unmatched N_LBRAC before symtab pos %d"),
			     symnum);
		  break;
		}

	      if (!outermost_context_p ())
		{
		  struct block *block;

		  cstk = pop_context ();
		  /* Make a block for the local symbols within.  */
		  block = finish_block (cstk.name,
					cstk.old_blocks, NULL,
					cstk.start_addr, valu);

		  /* For C++, set the block's scope.  */
		  if (cstk.name->language () == language_cplus)
		    cp_set_block_scope (cstk.name, block,
					&objfile->objfile_obstack);
		}

	      newobj = push_context (0, valu);
	      newobj->name = define_symbol (valu, name, desc, type, objfile);
	      if (newobj->name != nullptr)
		newobj->name->set_section_index (section_index);
	      break;

	    default:
	      {
		struct symbol *sym = define_symbol (valu, name, desc, type,
						    objfile);
		if (sym != nullptr)
		  sym->set_section_index (section_index);
	      }
	      break;
	    }
	}
      break;

      /* We use N_OPT to carry the gcc2_compiled flag.  Sun uses it
	 for a bunch of other flags, too.  Someday we may parse their
	 flags; for now we ignore theirs and hope they'll ignore ours.  */
    case N_OPT:			/* Solaris 2: Compiler options.  */
      if (name)
	{
	  if (strcmp (name, GCC2_COMPILED_FLAG_SYMBOL) == 0)
	    {
	      processing_gcc_compilation = 2;
	    }
	  else
	    n_opt_found = 1;
	}
      break;

    case N_MAIN:		/* Name of main routine.  */
      /* FIXME: If one has a symbol file with N_MAIN and then replaces
	 it with a symbol file with "main" and without N_MAIN.  I'm
	 not sure exactly what rule to follow but probably something
	 like: N_MAIN takes precedence over "main" no matter what
	 objfile it is in; If there is more than one N_MAIN, choose
	 the one in the symfile_objfile; If there is more than one
	 N_MAIN within a given objfile, complain() and choose
	 arbitrarily.  (kingdon) */
      if (name != NULL)
	set_objfile_main_name (objfile, name, language_unknown);
      break;

      /* The following symbol types can be ignored.  */
    case N_OBJ:			/* Solaris 2: Object file dir and name.  */
    case N_PATCH:		/* Solaris 2: Patch Run Time Checker.  */
      /* N_UNDF:                   Solaris 2: File separator mark.  */
      /* N_UNDF: -- we will never encounter it, since we only process
	 one file's symbols at once.  */
    case N_ENDM:		/* Solaris 2: End of module.  */
    case N_ALIAS:		/* SunPro F77: alias name, ignore for now.  */
      break;
    }

  /* '#' is a GNU C extension to allow one symbol to refer to another
     related symbol.

     Generally this is used so that an alias can refer to its main
     symbol.  */
  gdb_assert (name);
  if (name[0] == '#')
    {
      /* Initialize symbol reference names and determine if this is a
	 definition.  If a symbol reference is being defined, go ahead
	 and add it.  Otherwise, just return.  */

      const char *s = name;
      int refnum;

      /* If this stab defines a new reference ID that is not on the
	 reference list, then put it on the reference list.

	 We go ahead and advance NAME past the reference, even though
	 it is not strictly necessary at this time.  */
      refnum = symbol_reference_defined (&s);
      if (refnum >= 0)
	if (!ref_search (refnum))
	  ref_add (refnum, 0, name, valu);
      name = s;
    }

  previous_stab_code = type;
}

/* FIXME: The only difference between this and elfstab_build_psymtabs
   is the call to install_minimal_symbols for elf, and the support for
   split sections.  If the differences are really that small, the code
   should be shared.  */

/* Scan and build partial symbols for an coff symbol file.
   The coff file has already been processed to get its minimal symbols.

   This routine is the equivalent of dbx_symfile_init and dbx_symfile_read
   rolled into one.

   OBJFILE is the object file we are reading symbols from.
   ADDR is the address relative to which the symbols are (e.g.
   the base address of the text segment).
   TEXTADDR is the address of the text section.
   TEXTSIZE is the size of the text section.
   STABSECTS is the list of .stab sections in OBJFILE.
   STABSTROFFSET and STABSTRSIZE define the location in OBJFILE where the
   .stabstr section exists.

   This routine is mostly copied from dbx_symfile_init and dbx_symfile_read,
   adjusted for coff details.  */

void
coffstab_build_psymtabs (struct objfile *objfile,
			 CORE_ADDR textaddr, unsigned int textsize,
			 const std::vector<asection *> &stabsects,
			 file_ptr stabstroffset, unsigned int stabstrsize)
{
  int val;
  bfd *sym_bfd = objfile->obfd.get ();
  const char *name = bfd_get_filename (sym_bfd);
  unsigned int stabsize;

  /* Allocate struct to keep track of stab reading.  */
  dbx_objfile_data_key.emplace (objfile);

  DBX_TEXT_ADDR (objfile) = textaddr;
  DBX_TEXT_SIZE (objfile) = textsize;

#define	COFF_STABS_SYMBOL_SIZE	12	/* XXX FIXME XXX */
  DBX_SYMBOL_SIZE (objfile) = COFF_STABS_SYMBOL_SIZE;
  DBX_STRINGTAB_SIZE (objfile) = stabstrsize;

  if (stabstrsize > bfd_get_size (sym_bfd))
    error (_("ridiculous string table size: %d bytes"), stabstrsize);
  DBX_STRINGTAB (objfile) = (char *)
    obstack_alloc (&objfile->objfile_obstack, stabstrsize + 1);
  OBJSTAT (objfile, sz_strtab += stabstrsize + 1);

  /* Now read in the string table in one big gulp.  */

  val = bfd_seek (sym_bfd, stabstroffset, SEEK_SET);
  if (val < 0)
    perror_with_name (name);
  val = bfd_read (DBX_STRINGTAB (objfile), stabstrsize, sym_bfd);
  if (val != stabstrsize)
    perror_with_name (name);

  stabsread_new_init ();
  free_header_files ();
  init_header_files ();

  processing_acc_compilation = 1;

  /* In a coff file, we've already installed the minimal symbols that came
     from the coff (non-stab) symbol table, so always act like an
     incremental load here.  */
  scoped_restore save_symbuf_sections
    = make_scoped_restore (&symbuf_sections);
  if (stabsects.size () == 1)
    {
      stabsize = bfd_section_size (stabsects[0]);
      DBX_SYMCOUNT (objfile) = stabsize / DBX_SYMBOL_SIZE (objfile);
      DBX_SYMTAB_OFFSET (objfile) = stabsects[0]->filepos;
    }
  else
    {
      DBX_SYMCOUNT (objfile) = 0;
      for (asection *section : stabsects)
	{
	  stabsize = bfd_section_size (section);
	  DBX_SYMCOUNT (objfile) += stabsize / DBX_SYMBOL_SIZE (objfile);
	}

      DBX_SYMTAB_OFFSET (objfile) = stabsects[0]->filepos;

      sect_idx = 1;
      symbuf_sections = &stabsects;
      symbuf_left = bfd_section_size (stabsects[0]);
      symbuf_read = 0;
    }

  dbx_symfile_read (objfile, 0);
}

/* Scan and build partial symbols for an ELF symbol file.
   This ELF file has already been processed to get its minimal symbols.

   This routine is the equivalent of dbx_symfile_init and dbx_symfile_read
   rolled into one.

   OBJFILE is the object file we are reading symbols from.
   ADDR is the address relative to which the symbols are (e.g.
   the base address of the text segment).
   STABSECT is the BFD section information for the .stab section.
   STABSTROFFSET and STABSTRSIZE define the location in OBJFILE where the
   .stabstr section exists.

   This routine is mostly copied from dbx_symfile_init and dbx_symfile_read,
   adjusted for elf details.  */

void
elfstab_build_psymtabs (struct objfile *objfile, asection *stabsect,
			file_ptr stabstroffset, unsigned int stabstrsize)
{
  int val;
  bfd *sym_bfd = objfile->obfd.get ();
  const char *name = bfd_get_filename (sym_bfd);

  stabsread_new_init ();

  /* Allocate struct to keep track of stab reading.  */
  dbx_objfile_data_key.emplace (objfile);

  /* Find the first and last text address.  dbx_symfile_read seems to
     want this.  */
  find_text_range (sym_bfd, objfile);

#define	ELF_STABS_SYMBOL_SIZE	12	/* XXX FIXME XXX */
  DBX_SYMBOL_SIZE (objfile) = ELF_STABS_SYMBOL_SIZE;
  DBX_SYMCOUNT (objfile)
    = bfd_section_size (stabsect) / DBX_SYMBOL_SIZE (objfile);
  DBX_STRINGTAB_SIZE (objfile) = stabstrsize;
  DBX_SYMTAB_OFFSET (objfile) = stabsect->filepos;
  DBX_STAB_SECTION (objfile) = stabsect;

  if (stabstrsize > bfd_get_size (sym_bfd))
    error (_("ridiculous string table size: %d bytes"), stabstrsize);
  DBX_STRINGTAB (objfile) = (char *)
    obstack_alloc (&objfile->objfile_obstack, stabstrsize + 1);
  OBJSTAT (objfile, sz_strtab += stabstrsize + 1);

  /* Now read in the string table in one big gulp.  */

  val = bfd_seek (sym_bfd, stabstroffset, SEEK_SET);
  if (val < 0)
    perror_with_name (name);
  val = bfd_read (DBX_STRINGTAB (objfile), stabstrsize, sym_bfd);
  if (val != stabstrsize)
    perror_with_name (name);

  stabsread_new_init ();
  free_header_files ();
  init_header_files ();

  processing_acc_compilation = 1;

  symbuf_read = 0;
  symbuf_left = bfd_section_size (stabsect);

  scoped_restore restore_stabs_data = make_scoped_restore (&stabs_data);
  gdb::unique_xmalloc_ptr<gdb_byte> data_holder;

  stabs_data = symfile_relocate_debug_section (objfile, stabsect, NULL);
  if (stabs_data)
    data_holder.reset (stabs_data);

  /* In an elf file, we've already installed the minimal symbols that came
     from the elf (non-stab) symbol table, so always act like an
     incremental load here.  dbx_symfile_read should not generate any new
     minimal symbols, since we will have already read the ELF dynamic symbol
     table and normal symbol entries won't be in the ".stab" section; but in
     case it does, it will install them itself.  */
  dbx_symfile_read (objfile, 0);
}

/* Scan and build partial symbols for a file with special sections for stabs
   and stabstrings.  The file has already been processed to get its minimal
   symbols, and any other symbols that might be necessary to resolve GSYMs.

   This routine is the equivalent of dbx_symfile_init and dbx_symfile_read
   rolled into one.

   OBJFILE is the object file we are reading symbols from.
   ADDR is the address relative to which the symbols are (e.g. the base address
   of the text segment).
   STAB_NAME is the name of the section that contains the stabs.
   STABSTR_NAME is the name of the section that contains the stab strings.

   This routine is mostly copied from dbx_symfile_init and
   dbx_symfile_read.  */

void
stabsect_build_psymtabs (struct objfile *objfile, char *stab_name,
			 char *stabstr_name, char *text_name)
{
  int val;
  bfd *sym_bfd = objfile->obfd.get ();
  const char *name = bfd_get_filename (sym_bfd);
  asection *stabsect;
  asection *stabstrsect;
  asection *text_sect;

  stabsect = bfd_get_section_by_name (sym_bfd, stab_name);
  stabstrsect = bfd_get_section_by_name (sym_bfd, stabstr_name);

  if (!stabsect)
    return;

  if (!stabstrsect)
    error (_("stabsect_build_psymtabs:  Found stabs (%s), "
	     "but not string section (%s)"),
	   stab_name, stabstr_name);

  dbx_objfile_data_key.emplace (objfile);

  text_sect = bfd_get_section_by_name (sym_bfd, text_name);
  if (!text_sect)
    error (_("Can't find %s section in symbol file"), text_name);
  DBX_TEXT_ADDR (objfile) = bfd_section_vma (text_sect);
  DBX_TEXT_SIZE (objfile) = bfd_section_size (text_sect);

  DBX_SYMBOL_SIZE (objfile) = sizeof (struct external_nlist);
  DBX_SYMCOUNT (objfile) = bfd_section_size (stabsect)
    / DBX_SYMBOL_SIZE (objfile);
  DBX_STRINGTAB_SIZE (objfile) = bfd_section_size (stabstrsect);
  DBX_SYMTAB_OFFSET (objfile) = stabsect->filepos;	/* XXX - FIXME: POKING
							   INSIDE BFD DATA
							   STRUCTURES */

  if (DBX_STRINGTAB_SIZE (objfile) > bfd_get_size (sym_bfd))
    error (_("ridiculous string table size: %d bytes"),
	   DBX_STRINGTAB_SIZE (objfile));
  DBX_STRINGTAB (objfile) = (char *)
    obstack_alloc (&objfile->objfile_obstack,
		   DBX_STRINGTAB_SIZE (objfile) + 1);
  OBJSTAT (objfile, sz_strtab += DBX_STRINGTAB_SIZE (objfile) + 1);

  /* Now read in the string table in one big gulp.  */

  val = bfd_get_section_contents (sym_bfd,	/* bfd */
				  stabstrsect,	/* bfd section */
				  DBX_STRINGTAB (objfile), /* input buffer */
				  0,		/* offset into section */
				  DBX_STRINGTAB_SIZE (objfile)); /* amount to
								    read */

  if (!val)
    perror_with_name (name);

  stabsread_new_init ();
  free_header_files ();
  init_header_files ();

  /* Now, do an incremental load.  */

  processing_acc_compilation = 1;
  dbx_symfile_read (objfile, 0);
}

static const struct sym_fns aout_sym_fns =
{
  dbx_new_init,			/* init anything gbl to entire symtab */
  dbx_symfile_init,		/* read initial info, setup for sym_read() */
  dbx_symfile_read,		/* read a symbol file into symtab */
  dbx_symfile_finish,		/* finished with file, cleanup */
  default_symfile_offsets, 	/* parse user's offsets to internal form */
  default_symfile_segments,	/* Get segment information from a file.  */
  NULL,
  default_symfile_relocate,	/* Relocate a debug section.  */
  NULL,				/* sym_probe_fns */
};

void _initialize_dbxread ();
void
_initialize_dbxread ()
{
  add_symtab_fns (bfd_target_aout_flavour, &aout_sym_fns);
}
