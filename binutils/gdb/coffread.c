/* Read coff symbol tables and convert to internal format, for GDB.
   Copyright (C) 1987-2024 Free Software Foundation, Inc.
   Contributed by David D. Johnson, Brown University (ddj@cs.brown.edu).

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
#include "symtab.h"
#include "gdbtypes.h"
#include "demangle.h"
#include "breakpoint.h"

#include "bfd.h"
#include "gdbsupport/gdb_obstack.h"
#include <ctype.h>

#include "coff/internal.h"
#include "libcoff.h"
#include "objfiles.h"
#include "buildsym-legacy.h"
#include "stabsread.h"
#include "complaints.h"
#include "target.h"
#include "block.h"
#include "dictionary.h"
#include "dwarf2/public.h"

#include "coff-pe-read.h"

/* The objfile we are currently reading.  */

static struct objfile *coffread_objfile;

struct coff_symfile_info
  {
    file_ptr min_lineno_offset = 0;	/* Where in file lowest line#s are.  */
    file_ptr max_lineno_offset = 0;	/* 1+last byte of line#s in file.  */

    CORE_ADDR textaddr = 0;		/* Addr of .text section.  */
    unsigned int textsize = 0;	/* Size of .text section.  */
    std::vector<asection *> *stabsects;	/* .stab sections.  */
    asection *stabstrsect = nullptr;	/* Section pointer for .stab section.  */
    char *stabstrdata = nullptr;
  };

/* Key for COFF-associated data.  */

static const registry<objfile>::key<coff_symfile_info> coff_objfile_data_key;

/* Translate an external name string into a user-visible name.  */
#define	EXTERNAL_NAME(string, abfd) \
  (*string != '\0' && *string == bfd_get_symbol_leading_char (abfd)	\
   ? string + 1 : string)

/* To be an sdb debug type, type must have at least a basic or primary
   derived type.  Using this rather than checking against T_NULL is
   said to prevent core dumps if we try to operate on Michael Bloom
   dbx-in-coff file.  */

#define SDB_TYPE(type) (BTYPE(type) | (type & N_TMASK))

/* Core address of start and end of text of current source file.
   This comes from a ".text" symbol where x_nlinno > 0.  */

static CORE_ADDR current_source_start_addr;
static CORE_ADDR current_source_end_addr;

/* The addresses of the symbol table stream and number of symbols
   of the object file we are reading (as copied into core).  */

static bfd *nlist_bfd_global;
static int nlist_nsyms_global;


/* Pointers to scratch storage, used for reading raw symbols and
   auxents.  */

static char *temp_sym;
static char *temp_aux;

/* Local variables that hold the shift and mask values for the
   COFF file that we are currently reading.  These come back to us
   from BFD, and are referenced by their macro names, as well as
   internally to the BTYPE, ISPTR, ISFCN, ISARY, ISTAG, and DECREF
   macros from include/coff/internal.h .  */

static unsigned local_n_btmask;
static unsigned local_n_btshft;
static unsigned local_n_tmask;
static unsigned local_n_tshift;

#define	N_BTMASK	local_n_btmask
#define	N_BTSHFT	local_n_btshft
#define	N_TMASK		local_n_tmask
#define	N_TSHIFT	local_n_tshift

/* Local variables that hold the sizes in the file of various COFF
   structures.  (We only need to know this to read them from the file
   -- BFD will then translate the data in them, into `internal_xxx'
   structs in the right byte order, alignment, etc.)  */

static unsigned local_linesz;
static unsigned local_symesz;
static unsigned local_auxesz;

/* This is set if this is a PE format file.  */

static int pe_file;

/* Chain of typedefs of pointers to empty struct/union types.
   They are chained thru the SYMBOL_VALUE_CHAIN.  */

static struct symbol *opaque_type_chain[HASHSIZE];

/* Simplified internal version of coff symbol table information.  */

struct coff_symbol
  {
    char *c_name;
    int c_symnum;		/* Symbol number of this entry.  */
    int c_naux;			/* 0 if syment only, 1 if syment +
				   auxent, etc.  */
    CORE_ADDR c_value;
    int c_sclass;
    int c_secnum;
    unsigned int c_type;
  };

/* Vector of types defined so far, indexed by their type numbers.  */

static struct type **type_vector;

/* Number of elements allocated for type_vector currently.  */

static int type_vector_length;

/* Initial size of type vector.  Is realloc'd larger if needed, and
   realloc'd down to the size actually used, when completed.  */

#define INITIAL_TYPE_VECTOR_LENGTH 160

static char *linetab = NULL;
static file_ptr linetab_offset;
static file_ptr linetab_size;

static char *stringtab = NULL;
static long stringtab_length = 0;

extern void stabsread_clear_cache (void);

static struct type *coff_read_struct_type (int, int, int,
					   struct objfile *);

static struct type *decode_base_type (struct coff_symbol *,
				      unsigned int,
				      union internal_auxent *,
				      struct objfile *);

static struct type *decode_type (struct coff_symbol *, unsigned int,
				 union internal_auxent *,
				 struct objfile *);

static struct type *decode_function_type (struct coff_symbol *,
					  unsigned int,
					  union internal_auxent *,
					  struct objfile *);

static struct type *coff_read_enum_type (int, int, int,
					 struct objfile *);

static struct symbol *process_coff_symbol (struct coff_symbol *,
					   union internal_auxent *,
					   struct objfile *);

static void patch_opaque_types (struct symtab *);

static void enter_linenos (file_ptr, int, int, struct objfile *);

static int init_lineno (bfd *, file_ptr, file_ptr, gdb::unique_xmalloc_ptr<char> *);

static char *getsymname (struct internal_syment *);

static const char *coff_getfilename (union internal_auxent *);

static int init_stringtab (bfd *, file_ptr, gdb::unique_xmalloc_ptr<char> *);

static void read_one_sym (struct coff_symbol *,
			  struct internal_syment *,
			  union internal_auxent *);

static void coff_symtab_read (minimal_symbol_reader &,
			      file_ptr, unsigned int, struct objfile *);

/* We are called once per section from coff_symfile_read.  We
   need to examine each section we are passed, check to see
   if it is something we are interested in processing, and
   if so, stash away some access information for the section.

   FIXME: The section names should not be hardwired strings (what
   should they be?  I don't think most object file formats have enough
   section flags to specify what kind of debug section it is
   -kingdon).  */

static void
coff_locate_sections (bfd *abfd, asection *sectp, void *csip)
{
  struct coff_symfile_info *csi;
  const char *name;

  csi = (struct coff_symfile_info *) csip;
  name = bfd_section_name (sectp);
  if (strcmp (name, ".text") == 0)
    {
      csi->textaddr = bfd_section_vma (sectp);
      csi->textsize += bfd_section_size (sectp);
    }
  else if (startswith (name, ".text"))
    {
      csi->textsize += bfd_section_size (sectp);
    }
  else if (strcmp (name, ".stabstr") == 0)
    {
      csi->stabstrsect = sectp;
    }
  else if (startswith (name, ".stab"))
    {
      const char *s;

      /* We can have multiple .stab sections if linked with
	 --split-by-reloc.  */
      for (s = name + sizeof ".stab" - 1; *s != '\0'; s++)
	if (!isdigit (*s))
	  break;
      if (*s == '\0')
	csi->stabsects->push_back (sectp);
    }
}

/* Return the section_offsets* that CS points to.  */
static int cs_to_section (struct coff_symbol *, struct objfile *);

struct coff_find_targ_sec_arg
  {
    int targ_index;
    asection **resultp;
  };

static void
find_targ_sec (bfd *abfd, asection *sect, void *obj)
{
  struct coff_find_targ_sec_arg *args = (struct coff_find_targ_sec_arg *) obj;

  if (sect->target_index == args->targ_index)
    *args->resultp = sect;
}

/* Return the bfd_section that CS points to.  */
static struct bfd_section*
cs_to_bfd_section (struct coff_symbol *cs, struct objfile *objfile)
{
  asection *sect = NULL;
  struct coff_find_targ_sec_arg args;

  args.targ_index = cs->c_secnum;
  args.resultp = &sect;
  bfd_map_over_sections (objfile->obfd.get (), find_targ_sec, &args);
  return sect;
}

/* Return the section number (SECT_OFF_*) that CS points to.  */
static int
cs_to_section (struct coff_symbol *cs, struct objfile *objfile)
{
  asection *sect = cs_to_bfd_section (cs, objfile);

  if (sect == NULL)
    return SECT_OFF_TEXT (objfile);
  return gdb_bfd_section_index (objfile->obfd.get (), sect);
}

/* Return the address of the section of a COFF symbol.  */

static CORE_ADDR cs_section_address (struct coff_symbol *, bfd *);

static CORE_ADDR
cs_section_address (struct coff_symbol *cs, bfd *abfd)
{
  asection *sect = NULL;
  struct coff_find_targ_sec_arg args;
  CORE_ADDR addr = 0;

  args.targ_index = cs->c_secnum;
  args.resultp = &sect;
  bfd_map_over_sections (abfd, find_targ_sec, &args);
  if (sect != NULL)
    addr = bfd_section_vma (sect);
  return addr;
}

/* Look up a coff type-number index.  Return the address of the slot
   where the type for that index is stored.
   The type-number is in INDEX. 

   This can be used for finding the type associated with that index
   or for associating a new type with the index.  */

static struct type **
coff_lookup_type (int index)
{
  if (index >= type_vector_length)
    {
      int old_vector_length = type_vector_length;

      type_vector_length *= 2;
      if (index /* is still */  >= type_vector_length)
	type_vector_length = index * 2;

      type_vector = (struct type **)
	xrealloc ((char *) type_vector,
		  type_vector_length * sizeof (struct type *));
      memset (&type_vector[old_vector_length], 0,
	 (type_vector_length - old_vector_length) * sizeof (struct type *));
    }
  return &type_vector[index];
}

/* Make sure there is a type allocated for type number index
   and return the type object.
   This can create an empty (zeroed) type object.  */

static struct type *
coff_alloc_type (int index)
{
  struct type **type_addr = coff_lookup_type (index);
  struct type *type = *type_addr;

  /* If we are referring to a type not known at all yet,
     allocate an empty type for it.
     We will fill it in later if we find out how.  */
  if (type == NULL)
    {
      type = type_allocator (coffread_objfile, language_c).new_type ();
      *type_addr = type;
    }
  return type;
}

/* Start a new symtab for a new source file.
   This is called when a COFF ".file" symbol is seen;
   it indicates the start of data for one original source file.  */

static void
coff_start_compunit_symtab (struct objfile *objfile, const char *name)
{
  within_function = 0;
  start_compunit_symtab (objfile,
			 name,
  /* We never know the directory name for COFF.  */
			 NULL,
  /* The start address is irrelevant, since we call
     set_last_source_start_addr in coff_end_compunit_symtab.  */
			 0,
  /* Let buildsym.c deduce the language for this symtab.  */
			 language_unknown);
  record_debugformat ("COFF");
}

/* Save the vital information from when starting to read a file,
   for use when closing off the current file.
   NAME is the file name the symbols came from, START_ADDR is the
   first text address for the file, and SIZE is the number of bytes of
   text.  */

static void
complete_symtab (const char *name, CORE_ADDR start_addr, unsigned int size)
{
  set_last_source_file (name);
  current_source_start_addr = start_addr;
  current_source_end_addr = start_addr + size;
}

/* Finish the symbol definitions for one main source file, close off
   all the lexical contexts for that file (creating struct block's for
   them), then make the struct symtab for that file and put it in the
   list of all such.  */

static void
coff_end_compunit_symtab (struct objfile *objfile)
{
  set_last_source_start_addr (current_source_start_addr);

  end_compunit_symtab (current_source_end_addr);

  /* Reinitialize for beginning of new file.  */
  set_last_source_file (NULL);
}

/* The linker sometimes generates some non-function symbols inside
   functions referencing variables imported from another DLL.
   Return nonzero if the given symbol corresponds to one of them.  */

static int
is_import_fixup_symbol (struct coff_symbol *cs,
			enum minimal_symbol_type type)
{
  /* The following is a bit of a heuristic using the characteristics
     of these fixup symbols, but should work well in practice...  */
  int i;

  /* Must be a non-static text symbol.  */
  if (type != mst_text)
    return 0;

  /* Must be a non-function symbol.  */
  if (ISFCN (cs->c_type))
    return 0;

  /* The name must start with "__fu<digits>__".  */
  if (!startswith (cs->c_name, "__fu"))
    return 0;
  if (! isdigit (cs->c_name[4]))
    return 0;
  for (i = 5; cs->c_name[i] != '\0' && isdigit (cs->c_name[i]); i++)
    /* Nothing, just incrementing index past all digits.  */;
  if (cs->c_name[i] != '_' || cs->c_name[i + 1] != '_')
    return 0;

  return 1;
}

static struct minimal_symbol *
record_minimal_symbol (minimal_symbol_reader &reader,
		       struct coff_symbol *cs, unrelocated_addr address,
		       enum minimal_symbol_type type, int section, 
		       struct objfile *objfile)
{
  /* We don't want TDESC entry points in the minimal symbol table.  */
  if (cs->c_name[0] == '@')
    return NULL;

  if (is_import_fixup_symbol (cs, type))
    {
      /* Because the value of these symbols is within a function code
	 range, these symbols interfere with the symbol-from-address
	 reverse lookup; this manifests itself in backtraces, or any
	 other commands that prints symbolic addresses.  Just pretend
	 these symbols do not exist.  */
      return NULL;
    }

  return reader.record_full (cs->c_name, true, address, type, section);
}

/* coff_symfile_init ()
   is the coff-specific initialization routine for reading symbols.
   It is passed a struct objfile which contains, among other things,
   the BFD for the file whose symbols are being read, and a slot for
   a pointer to "private data" which we fill with cookies and other
   treats for coff_symfile_read ().

   We will only be called if this is a COFF or COFF-like file.  BFD
   handles figuring out the format of the file, and code in symtab.c
   uses BFD's determination to vector to us.

   The ultimate result is a new symtab (or, FIXME, eventually a
   psymtab).  */

static void
coff_symfile_init (struct objfile *objfile)
{
  /* Allocate struct to keep track of the symfile.  */
  coff_objfile_data_key.emplace (objfile);
}

/* This function is called for every section; it finds the outer
   limits of the line table (minimum and maximum file offset) so that
   the mainline code can read the whole thing for efficiency.  */

static void
find_linenos (bfd *abfd, struct bfd_section *asect, void *vpinfo)
{
  struct coff_symfile_info *info;
  int size, count;
  file_ptr offset, maxoff;

  /* WARNING WILL ROBINSON!  ACCESSING BFD-PRIVATE DATA HERE!  FIXME!  */
  count = asect->lineno_count;
  /* End of warning.  */

  if (count == 0)
    return;
  size = count * local_linesz;

  info = (struct coff_symfile_info *) vpinfo;
  /* WARNING WILL ROBINSON!  ACCESSING BFD-PRIVATE DATA HERE!  FIXME!  */
  offset = asect->line_filepos;
  /* End of warning.  */

  if (offset < info->min_lineno_offset || info->min_lineno_offset == 0)
    info->min_lineno_offset = offset;

  maxoff = offset + size;
  if (maxoff > info->max_lineno_offset)
    info->max_lineno_offset = maxoff;
}


/* A helper function for coff_symfile_read that reads minimal
   symbols.  It may also read other forms of symbol as well.  */

static void
coff_read_minsyms (file_ptr symtab_offset, unsigned int nsyms,
		   struct objfile *objfile)

{
  /* If minimal symbols were already read, and if we know we aren't
     going to read any other kind of symbol here, then we can just
     return.  */
  if (objfile->per_bfd->minsyms_read && pe_file && nsyms == 0)
    return;

  minimal_symbol_reader reader (objfile);

  if (pe_file && nsyms == 0)
    {
      /* We've got no debugging symbols, but it's a portable
	 executable, so try to read the export table.  */
      read_pe_exported_syms (reader, objfile);
    }
  else
    {
      /* Now that the executable file is positioned at symbol table,
	 process it and define symbols accordingly.  */
      coff_symtab_read (reader, symtab_offset, nsyms, objfile);
    }

  /* Install any minimal symbols that have been collected as the
     current minimal symbols for this objfile.  */

  reader.install ();

  if (pe_file)
    {
      for (minimal_symbol *msym : objfile->msymbols ())
	{
	  const char *name = msym->linkage_name ();

	  /* If the minimal symbols whose name are prefixed by "__imp_"
	     or "_imp_", get rid of the prefix, and search the minimal
	     symbol in OBJFILE.  Note that 'maintenance print msymbols'
	     shows that type of these "_imp_XXXX" symbols is mst_data.  */
	  if (msym->type () == mst_data)
	    {
	      const char *name1 = NULL;

	      if (startswith (name, "_imp_"))
		name1 = name + 5;
	      else if (startswith (name, "__imp_"))
		name1 = name + 6;
	      if (name1 != NULL)
		{
		  int lead
		    = bfd_get_symbol_leading_char (objfile->obfd.get ());
		  struct bound_minimal_symbol found;

		  if (lead != '\0' && *name1 == lead)
		    name1 += 1;

		  found = lookup_minimal_symbol (name1, NULL, objfile);

		  /* If found, there are symbols named "_imp_foo" and "foo"
		     respectively in OBJFILE.  Set the type of symbol "foo"
		     as 'mst_solib_trampoline'.  */
		  if (found.minsym != NULL
		      && found.minsym->type () == mst_text)
		    found.minsym->set_type (mst_solib_trampoline);
		}
	    }
	}
    }
}

/* The BFD for this file -- only good while we're actively reading
   symbols into a psymtab or a symtab.  */

static bfd *symfile_bfd;

/* Read a symbol file, after initialization by coff_symfile_init.  */

static void
coff_symfile_read (struct objfile *objfile, symfile_add_flags symfile_flags)
{
  struct coff_symfile_info *info;
  bfd *abfd = objfile->obfd.get ();
  coff_data_type *cdata = coff_data (abfd);
  const char *filename = bfd_get_filename (abfd);
  int val;
  unsigned int num_symbols;
  file_ptr symtab_offset;
  file_ptr stringtab_offset;
  unsigned int stabstrsize;
  
  info = coff_objfile_data_key.get (objfile);
  symfile_bfd = abfd;		/* Kludge for swap routines.  */

  std::vector<asection *> stabsects;
  scoped_restore restore_stabsects
    = make_scoped_restore (&info->stabsects, &stabsects);

/* WARNING WILL ROBINSON!  ACCESSING BFD-PRIVATE DATA HERE!  FIXME!  */
  num_symbols = bfd_get_symcount (abfd);	/* How many syms */
  symtab_offset = cdata->sym_filepos;	/* Symbol table file offset */
  stringtab_offset = symtab_offset +	/* String table file offset */
    num_symbols * cdata->local_symesz;

  /* Set a few file-statics that give us specific information about
     the particular COFF file format we're reading.  */
  local_n_btmask = cdata->local_n_btmask;
  local_n_btshft = cdata->local_n_btshft;
  local_n_tmask = cdata->local_n_tmask;
  local_n_tshift = cdata->local_n_tshift;
  local_linesz = cdata->local_linesz;
  local_symesz = cdata->local_symesz;
  local_auxesz = cdata->local_auxesz;

  /* Allocate space for raw symbol and aux entries, based on their
     space requirements as reported by BFD.  */
  gdb::def_vector<char> temp_storage (cdata->local_symesz
				      + cdata->local_auxesz);
  temp_sym = temp_storage.data ();
  temp_aux = temp_sym + cdata->local_symesz;

  /* We need to know whether this is a PE file, because in PE files,
     unlike standard COFF files, symbol values are stored as offsets
     from the section address, rather than as absolute addresses.
     FIXME: We should use BFD to read the symbol table, and thus avoid
     this problem.  */
  pe_file =
    startswith (bfd_get_target (objfile->obfd.get ()), "pe")
    || startswith (bfd_get_target (objfile->obfd.get ()), "epoc-pe");

  /* End of warning.  */

  info->min_lineno_offset = 0;
  info->max_lineno_offset = 0;

  /* Only read line number information if we have symbols.

     On Windows NT, some of the system's DLL's have sections with
     PointerToLinenumbers fields that are non-zero, but point at
     random places within the image file.  (In the case I found,
     KERNEL32.DLL's .text section has a line number info pointer that
     points into the middle of the string `lib\\i386\kernel32.dll'.)

     However, these DLL's also have no symbols.  The line number
     tables are meaningless without symbols.  And in fact, GDB never
     uses the line number information unless there are symbols.  So we
     can avoid spurious error messages (and maybe run a little
     faster!) by not even reading the line number table unless we have
     symbols.  */
  scoped_restore restore_linetab = make_scoped_restore (&linetab);
  gdb::unique_xmalloc_ptr<char> linetab_storage;
  if (num_symbols > 0)
    {
      /* Read the line number table, all at once.  */
      bfd_map_over_sections (abfd, find_linenos, (void *) info);

      val = init_lineno (abfd, info->min_lineno_offset,
			 info->max_lineno_offset - info->min_lineno_offset,
			 &linetab_storage);
      if (val < 0)
	error (_("\"%s\": error reading line numbers."), filename);
    }

  /* Now read the string table, all at once.  */

  scoped_restore restore_stringtab = make_scoped_restore (&stringtab);
  gdb::unique_xmalloc_ptr<char> stringtab_storage;
  val = init_stringtab (abfd, stringtab_offset, &stringtab_storage);
  if (val < 0)
    error (_("\"%s\": can't get string table"), filename);

  coff_read_minsyms (symtab_offset, num_symbols, objfile);

  if (!(objfile->flags & OBJF_READNEVER))
    bfd_map_over_sections (abfd, coff_locate_sections, (void *) info);

  if (!info->stabsects->empty())
    {
      if (!info->stabstrsect)
	{
	  error (_("The debugging information in `%s' is corrupted.\nThe "
		   "file has a `.stabs' section, but no `.stabstr' section."),
		 filename);
	}

      /* FIXME: dubious.  Why can't we use something normal like
	 bfd_get_section_contents?  */
      stabstrsize = bfd_section_size (info->stabstrsect);

      coffstab_build_psymtabs (objfile,
			       info->textaddr, info->textsize,
			       *info->stabsects,
			       info->stabstrsect->filepos, stabstrsize);
    }

  if (dwarf2_initialize_objfile (objfile))
    {
      /* Nothing.  */
    }

  /* Try to add separate debug file if no symbols table found.   */
  else if (!objfile->has_partial_symbols ()
	   && objfile->separate_debug_objfile == NULL
	   && objfile->separate_debug_objfile_backlink == NULL)
    {
      if (objfile->find_and_add_separate_symbol_file (symfile_flags))
	gdb_assert (objfile->separate_debug_objfile != nullptr);
    }
}

static void
coff_new_init (struct objfile *ignore)
{
}

/* Perform any local cleanups required when we are done with a
   particular objfile.  I.E, we are in the process of discarding all
   symbol information for an objfile, freeing up all memory held for
   it, and unlinking the objfile struct from the global list of known
   objfiles.  */

static void
coff_symfile_finish (struct objfile *objfile)
{
  /* Let stabs reader clean up.  */
  stabsread_clear_cache ();
}


/* Given pointers to a symbol table in coff style exec file,
   analyze them and create struct symtab's describing the symbols.
   NSYMS is the number of symbols in the symbol table.
   We read them one at a time using read_one_sym ().  */

static void
coff_symtab_read (minimal_symbol_reader &reader,
		  file_ptr symtab_offset, unsigned int nsyms,
		  struct objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  struct context_stack *newobj = nullptr;
  struct coff_symbol coff_symbol;
  struct coff_symbol *cs = &coff_symbol;
  static struct internal_syment main_sym;
  static union internal_auxent main_aux;
  struct coff_symbol fcn_cs_saved;
  static struct internal_syment fcn_sym_saved;
  static union internal_auxent fcn_aux_saved;
  /* A .file is open.  */
  int in_source_file = 0;
  int next_file_symnum = -1;
  /* Name of the current file.  */
  const char *filestring = "";
  int depth = 0;
  int fcn_first_line = 0;
  CORE_ADDR fcn_first_line_addr = 0;
  int fcn_last_line = 0;
  int fcn_start_addr = 0;
  long fcn_line_ptr = 0;
  int val;
  CORE_ADDR tmpaddr;
  struct minimal_symbol *msym;

  scoped_free_pendings free_pending;

  /* Position to read the symbol table.  */
  val = bfd_seek (objfile->obfd.get (), symtab_offset, 0);
  if (val < 0)
    perror_with_name (objfile_name (objfile));

  coffread_objfile = objfile;
  nlist_bfd_global = objfile->obfd.get ();
  nlist_nsyms_global = nsyms;
  set_last_source_file (NULL);
  memset (opaque_type_chain, 0, sizeof opaque_type_chain);

  if (type_vector)		/* Get rid of previous one.  */
    xfree (type_vector);
  type_vector_length = INITIAL_TYPE_VECTOR_LENGTH;
  type_vector = XCNEWVEC (struct type *, type_vector_length);

  coff_start_compunit_symtab (objfile, "");

  symnum = 0;
  while (symnum < nsyms)
    {
      QUIT;			/* Make this command interruptable.  */

      read_one_sym (cs, &main_sym, &main_aux);

      if (cs->c_symnum == next_file_symnum && cs->c_sclass != C_FILE)
	{
	  if (get_last_source_file ())
	    coff_end_compunit_symtab (objfile);

	  coff_start_compunit_symtab (objfile, "_globals_");
	  /* coff_start_compunit_symtab will set the language of this symtab to
	     language_unknown, since such a ``file name'' is not
	     recognized.  Override that with the minimal language to
	     allow printing values in this symtab.  */
	  get_current_subfile ()->language = language_minimal;
	  complete_symtab ("_globals_", 0, 0);
	  /* Done with all files, everything from here on out is
	     globals.  */
	}

      /* Special case for file with type declarations only, no
	 text.  */
      if (!get_last_source_file () && SDB_TYPE (cs->c_type)
	  && cs->c_secnum == N_DEBUG)
	complete_symtab (filestring, 0, 0);

      /* Typedefs should not be treated as symbol definitions.  */
      if (ISFCN (cs->c_type) && cs->c_sclass != C_TPDEF)
	{
	  /* Record all functions -- external and static -- in
	     minsyms.  */
	  int section = cs_to_section (cs, objfile);

	  tmpaddr = cs->c_value;
	  /* Don't record unresolved symbols.  */
	  if (!(cs->c_secnum <= 0 && cs->c_value == 0))
	    record_minimal_symbol (reader, cs,
				   unrelocated_addr (tmpaddr),
				   mst_text, section, objfile);

	  fcn_line_ptr = main_aux.x_sym.x_fcnary.x_fcn.x_lnnoptr;
	  fcn_start_addr = tmpaddr;
	  fcn_cs_saved = *cs;
	  fcn_sym_saved = main_sym;
	  fcn_aux_saved = main_aux;
	  continue;
	}

      switch (cs->c_sclass)
	{
	case C_EFCN:
	case C_EXTDEF:
	case C_ULABEL:
	case C_USTATIC:
	case C_LINE:
	case C_ALIAS:
	case C_HIDDEN:
	  complaint (_("Bad n_sclass for symbol %s"),
		     cs->c_name);
	  break;

	case C_FILE:
	  /* c_value field contains symnum of next .file entry in
	     table or symnum of first global after last .file.  */
	  next_file_symnum = cs->c_value;
	  if (cs->c_naux > 0)
	    filestring = coff_getfilename (&main_aux);
	  else
	    filestring = "";

	  /* Complete symbol table for last object file
	     containing debugging information.  */
	  if (get_last_source_file ())
	    {
	      coff_end_compunit_symtab (objfile);
	      coff_start_compunit_symtab (objfile, filestring);
	    }
	  in_source_file = 1;
	  break;

	  /* C_LABEL is used for labels and static functions.
	     Including it here allows gdb to see static functions when
	     no debug info is available.  */
	case C_LABEL:
	  /* However, labels within a function can make weird
	     backtraces, so filter them out (from phdm@macqel.be).  */
	  if (within_function)
	    break;
	  [[fallthrough]];
	case C_STAT:
	case C_THUMBLABEL:
	case C_THUMBSTAT:
	case C_THUMBSTATFUNC:
	  if (cs->c_name[0] == '.')
	    {
	      if (strcmp (cs->c_name, ".text") == 0)
		{
		  /* FIXME: don't wire in ".text" as section name or
		     symbol name!  */
		  /* Check for in_source_file deals with case of a
		     file with debugging symbols followed by a later
		     file with no symbols.  */
		  if (in_source_file)
		    complete_symtab (filestring,
				     (cs->c_value
				      + objfile->text_section_offset ()),
				     main_aux.x_scn.x_scnlen);
		  in_source_file = 0;
		}
	      /* Flush rest of '.' symbols.  */
	      break;
	    }
	  else if (!SDB_TYPE (cs->c_type)
		   && cs->c_name[0] == 'L'
		   && (startswith (cs->c_name, "LI%")
		       || startswith (cs->c_name, "LF%")
		       || startswith (cs->c_name, "LC%")
		       || startswith (cs->c_name, "LP%")
		       || startswith (cs->c_name, "LPB%")
		       || startswith (cs->c_name, "LBB%")
		       || startswith (cs->c_name, "LBE%")
		       || startswith (cs->c_name, "LPBX%")))
	    /* At least on a 3b1, gcc generates swbeg and string labels
	       that look like this.  Ignore them.  */
	    break;
	  /* For static symbols that don't start with '.'...  */
	  [[fallthrough]];
	case C_THUMBEXT:
	case C_THUMBEXTFUNC:
	case C_EXT:
	  {
	    /* Record it in the minimal symbols regardless of
	       SDB_TYPE.  This parallels what we do for other debug
	       formats, and probably is needed to make
	       print_address_symbolic work right without the (now
	       gone) "set fast-symbolic-addr off" kludge.  */

	    enum minimal_symbol_type ms_type;
	    int sec;
	    CORE_ADDR offset = 0;

	    if (cs->c_secnum == N_UNDEF)
	      {
		/* This is a common symbol.  We used to rely on
		   the target to tell us whether it knows where
		   the symbol has been relocated to, but none of
		   the target implementations actually provided
		   that operation.  So we just ignore the symbol,
		   the same way we would do if we had a target-side
		   symbol lookup which returned no match.  */
		break;
	      }
	    else if (cs->c_secnum == N_ABS)
	      {
		/* Use the correct minimal symbol type (and don't
		   relocate) for absolute values.  */
		ms_type = mst_abs;
		sec = cs_to_section (cs, objfile);
		tmpaddr = cs->c_value;
	      }
	    else
	      {
		asection *bfd_section = cs_to_bfd_section (cs, objfile);

		sec = cs_to_section (cs, objfile);
		tmpaddr = cs->c_value;
		/* Statics in a PE file also get relocated.  */
		if (cs->c_sclass == C_EXT
		    || cs->c_sclass == C_THUMBEXTFUNC
		    || cs->c_sclass == C_THUMBEXT
		    || (pe_file && (cs->c_sclass == C_STAT)))
		  offset = objfile->section_offsets[sec];

		if (bfd_section->flags & SEC_CODE)
		  {
		    ms_type =
		      cs->c_sclass == C_EXT || cs->c_sclass == C_THUMBEXTFUNC
		      || cs->c_sclass == C_THUMBEXT ?
		      mst_text : mst_file_text;
		    tmpaddr = gdbarch_addr_bits_remove (gdbarch, tmpaddr);
		  }
		else if (bfd_section->flags & SEC_ALLOC
			 && bfd_section->flags & SEC_LOAD)
		  {
		    ms_type =
		      cs->c_sclass == C_EXT || cs->c_sclass == C_THUMBEXT
		      ? mst_data : mst_file_data;
		  }
		else if (bfd_section->flags & SEC_ALLOC)
		  {
		    ms_type =
		      cs->c_sclass == C_EXT || cs->c_sclass == C_THUMBEXT
		      ? mst_bss : mst_file_bss;
		  }
		else
		  ms_type = mst_unknown;
	      }

	    msym = record_minimal_symbol (reader, cs,
					  unrelocated_addr (tmpaddr),
					  ms_type, sec, objfile);
	    if (msym)
	      gdbarch_coff_make_msymbol_special (gdbarch,
						 cs->c_sclass, msym);

	    if (SDB_TYPE (cs->c_type))
	      {
		struct symbol *sym;

		sym = process_coff_symbol
		  (cs, &main_aux, objfile);
		sym->set_value_longest (tmpaddr + offset);
		sym->set_section_index (sec);
	      }
	  }
	  break;

	case C_FCN:
	  if (strcmp (cs->c_name, ".bf") == 0)
	    {
	      within_function = 1;

	      /* Value contains address of first non-init type
		 code.  */
	      /* main_aux.x_sym.x_misc.x_lnsz.x_lnno
		 contains line number of '{' }.  */
	      if (cs->c_naux != 1)
		complaint (_("`.bf' symbol %d has no aux entry"),
			   cs->c_symnum);
	      fcn_first_line = main_aux.x_sym.x_misc.x_lnsz.x_lnno;
	      fcn_first_line_addr = cs->c_value;

	      /* Might want to check that locals are 0 and
		 context_stack_depth is zero, and complain if not.  */

	      depth = 0;
	      newobj = push_context (depth, fcn_start_addr);
	      fcn_cs_saved.c_name = getsymname (&fcn_sym_saved);
	      newobj->name =
		process_coff_symbol (&fcn_cs_saved, 
				     &fcn_aux_saved, objfile);
	    }
	  else if (strcmp (cs->c_name, ".ef") == 0)
	    {
	      if (!within_function)
		error (_("Bad coff function information."));
	      /* The value of .ef is the address of epilogue code;
		 not useful for gdb.  */
	      /* { main_aux.x_sym.x_misc.x_lnsz.x_lnno
		 contains number of lines to '}' */

	      if (outermost_context_p ())
		{	/* We attempted to pop an empty context stack.  */
		  complaint (_("`.ef' symbol without matching `.bf' "
			       "symbol ignored starting at symnum %d"),
			     cs->c_symnum);
		  within_function = 0;
		  break;
		}

	      struct context_stack cstk = pop_context ();
	      /* Stack must be empty now.  */
	      if (!outermost_context_p () || newobj == NULL)
		{
		  complaint (_("Unmatched .ef symbol(s) ignored "
			       "starting at symnum %d"),
			     cs->c_symnum);
		  within_function = 0;
		  break;
		}
	      if (cs->c_naux != 1)
		{
		  complaint (_("`.ef' symbol %d has no aux entry"),
			     cs->c_symnum);
		  fcn_last_line = 0x7FFFFFFF;
		}
	      else
		{
		  fcn_last_line = main_aux.x_sym.x_misc.x_lnsz.x_lnno;
		}
	      /* fcn_first_line is the line number of the opening '{'.
		 Do not record it - because it would affect gdb's idea
		 of the line number of the first statement of the
		 function - except for one-line functions, for which
		 it is also the line number of all the statements and
		 of the closing '}', and for which we do not have any
		 other statement-line-number.  */
	      if (fcn_last_line == 1)
		record_line
		  (get_current_subfile (), fcn_first_line,
		   unrelocated_addr (gdbarch_addr_bits_remove (gdbarch,
							       fcn_first_line_addr)));
	      else
		enter_linenos (fcn_line_ptr, fcn_first_line,
			       fcn_last_line, objfile);

	      finish_block (cstk.name, cstk.old_blocks,
			    NULL, cstk.start_addr,
			    fcn_cs_saved.c_value
			    + fcn_aux_saved.x_sym.x_misc.x_fsize
			    + objfile->text_section_offset ());
	      within_function = 0;
	    }
	  break;

	case C_BLOCK:
	  if (strcmp (cs->c_name, ".bb") == 0)
	    {
	      tmpaddr = cs->c_value;
	      tmpaddr += objfile->text_section_offset ();
	      push_context (++depth, tmpaddr);
	    }
	  else if (strcmp (cs->c_name, ".eb") == 0)
	    {
	      if (outermost_context_p ())
		{	/* We attempted to pop an empty context stack.  */
		  complaint (_("`.eb' symbol without matching `.bb' "
			       "symbol ignored starting at symnum %d"),
			     cs->c_symnum);
		  break;
		}

	      struct context_stack cstk = pop_context ();
	      if (depth-- != cstk.depth)
		{
		  complaint (_("Mismatched .eb symbol ignored "
			       "starting at symnum %d"),
			     symnum);
		  break;
		}
	      if (*get_local_symbols () && !outermost_context_p ())
		{
		  tmpaddr = cs->c_value + objfile->text_section_offset ();
		  /* Make a block for the local symbols within.  */
		  finish_block (0, cstk.old_blocks, NULL,
				cstk.start_addr, tmpaddr);
		}
	      /* Now pop locals of block just finished.  */
	      *get_local_symbols () = cstk.locals;
	    }
	  break;

	default:
	  process_coff_symbol (cs, &main_aux, objfile);
	  break;
	}
    }

  if (get_last_source_file ())
    coff_end_compunit_symtab (objfile);

  /* Patch up any opaque types (references to types that are not defined
     in the file where they are referenced, e.g. "struct foo *bar").  */
  {
    for (compunit_symtab *cu : objfile->compunits ())
      {
	for (symtab *s : cu->filetabs ())
	  patch_opaque_types (s);
      }
  }

  coffread_objfile = NULL;
}

/* Routines for reading headers and symbols from executable.  */

/* Read the next symbol, swap it, and return it in both
   internal_syment form, and coff_symbol form.  Also return its first
   auxent, if any, in internal_auxent form, and skip any other
   auxents.  */

static void
read_one_sym (struct coff_symbol *cs,
	      struct internal_syment *sym,
	      union internal_auxent *aux)
{
  int i;
  bfd_size_type bytes;

  cs->c_symnum = symnum;
  bytes = bfd_read (temp_sym, local_symesz, nlist_bfd_global);
  if (bytes != local_symesz)
    error (_("%s: error reading symbols"), objfile_name (coffread_objfile));
  bfd_coff_swap_sym_in (symfile_bfd, temp_sym, (char *) sym);
  cs->c_naux = sym->n_numaux & 0xff;
  if (cs->c_naux >= 1)
    {
      bytes = bfd_read (temp_aux, local_auxesz, nlist_bfd_global);
      if (bytes != local_auxesz)
	error (_("%s: error reading symbols"), objfile_name (coffread_objfile));
      bfd_coff_swap_aux_in (symfile_bfd, temp_aux,
			    sym->n_type, sym->n_sclass,
			    0, cs->c_naux, (char *) aux);
      /* If more than one aux entry, read past it (only the first aux
	 is important).  */
      for (i = 1; i < cs->c_naux; i++)
	{
	  bytes = bfd_read (temp_aux, local_auxesz, nlist_bfd_global);
	  if (bytes != local_auxesz)
	    error (_("%s: error reading symbols"),
		   objfile_name (coffread_objfile));
	}
    }
  cs->c_name = getsymname (sym);
  cs->c_value = sym->n_value;
  cs->c_sclass = (sym->n_sclass & 0xff);
  cs->c_secnum = sym->n_scnum;
  cs->c_type = (unsigned) sym->n_type;
  if (!SDB_TYPE (cs->c_type))
    cs->c_type = 0;

#if 0
  if (cs->c_sclass & 128)
    printf (_("thumb symbol %s, class 0x%x\n"), cs->c_name, cs->c_sclass);
#endif

  symnum += 1 + cs->c_naux;

  /* The PE file format stores symbol values as offsets within the
     section, rather than as absolute addresses.  We correct that
     here, if the symbol has an appropriate storage class.  FIXME: We
     should use BFD to read the symbols, rather than duplicating the
     work here.  */
  if (pe_file)
    {
      switch (cs->c_sclass)
	{
	case C_EXT:
	case C_THUMBEXT:
	case C_THUMBEXTFUNC:
	case C_SECTION:
	case C_NT_WEAK:
	case C_STAT:
	case C_THUMBSTAT:
	case C_THUMBSTATFUNC:
	case C_LABEL:
	case C_THUMBLABEL:
	case C_BLOCK:
	case C_FCN:
	case C_EFCN:
	  if (cs->c_secnum != 0)
	    cs->c_value += cs_section_address (cs, symfile_bfd);
	  break;
	}
    }
}

/* Support for string table handling.  */

static int
init_stringtab (bfd *abfd, file_ptr offset, gdb::unique_xmalloc_ptr<char> *storage)
{
  long length;
  int val;
  unsigned char lengthbuf[4];

  /* If the file is stripped, the offset might be zero, indicating no
     string table.  Just return with `stringtab' set to null.  */
  if (offset == 0)
    return 0;

  if (bfd_seek (abfd, offset, 0) < 0)
    return -1;

  val = bfd_read (lengthbuf, sizeof lengthbuf, abfd);
  /* If no string table is needed, then the file may end immediately
     after the symbols.  Just return with `stringtab' set to null.  */
  if (val != sizeof lengthbuf)
    return 0;
  length = bfd_h_get_32 (symfile_bfd, lengthbuf);
  if (length < sizeof lengthbuf)
    return 0;

  storage->reset ((char *) xmalloc (length));
  stringtab = storage->get ();
  /* This is in target format (probably not very useful, and not
     currently used), not host format.  */
  memcpy (stringtab, lengthbuf, sizeof lengthbuf);
  stringtab_length = length;
  if (length == sizeof length)	/* Empty table -- just the count.  */
    return 0;

  val = bfd_read (stringtab + sizeof lengthbuf,
		  length - sizeof lengthbuf, abfd);
  if (val != length - sizeof lengthbuf || stringtab[length - 1] != '\0')
    return -1;

  return 0;
}

static char *
getsymname (struct internal_syment *symbol_entry)
{
  static char buffer[SYMNMLEN + 1];
  char *result;

  if (symbol_entry->_n._n_n._n_zeroes == 0)
    {
      if (symbol_entry->_n._n_n._n_offset > stringtab_length)
	error (_("COFF Error: string table offset (%s) outside string table (length %ld)"),
	       hex_string (symbol_entry->_n._n_n._n_offset), stringtab_length);
      result = stringtab + symbol_entry->_n._n_n._n_offset;
    }
  else
    {
      strncpy (buffer, symbol_entry->_n._n_name, SYMNMLEN);
      buffer[SYMNMLEN] = '\0';
      result = buffer;
    }
  return result;
}

/* Extract the file name from the aux entry of a C_FILE symbol.
   Return only the last component of the name.  Result is in static
   storage and is only good for temporary use.  */

static const char *
coff_getfilename (union internal_auxent *aux_entry)
{
  static char buffer[BUFSIZ];
  const char *result;

  if (aux_entry->x_file.x_n.x_n.x_zeroes == 0)
    {
      if (strlen (stringtab + aux_entry->x_file.x_n.x_n.x_offset) >= BUFSIZ)
	internal_error (_("coff file name too long"));
      strcpy (buffer, stringtab + aux_entry->x_file.x_n.x_n.x_offset);
    }
  else
    {
      size_t x_fname_len = sizeof (aux_entry->x_file.x_n.x_fname);
      strncpy (buffer, aux_entry->x_file.x_n.x_fname, x_fname_len);
      buffer[x_fname_len] = '\0';
    }
  result = buffer;

  /* FIXME: We should not be throwing away the information about what
     directory.  It should go into dirname of the symtab, or some such
     place.  */
  result = lbasename (result);
  return (result);
}

/* Support for line number handling.  */

/* Read in all the line numbers for fast lookups later.  Leave them in
   external (unswapped) format in memory; we'll swap them as we enter
   them into GDB's data structures.  */

static int
init_lineno (bfd *abfd, file_ptr offset, file_ptr size,
	     gdb::unique_xmalloc_ptr<char> *storage)
{
  int val;

  linetab_offset = offset;
  linetab_size = size;

  if (size == 0)
    return 0;

  if (bfd_seek (abfd, offset, 0) < 0)
    return -1;

  /* Allocate the desired table, plus a sentinel.  */
  storage->reset ((char *) xmalloc (size + local_linesz));
  linetab = storage->get ();

  val = bfd_read (storage->get (), size, abfd);
  if (val != size)
    return -1;

  /* Terminate it with an all-zero sentinel record.  */
  memset (linetab + size, 0, local_linesz);

  return 0;
}

#if !defined (L_LNNO32)
#define L_LNNO32(lp) ((lp)->l_lnno)
#endif

static void
enter_linenos (file_ptr file_offset, int first_line,
	       int last_line, struct objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  char *rawptr;
  struct internal_lineno lptr;

  if (!linetab)
    return;
  if (file_offset < linetab_offset)
    {
      complaint (_("Line number pointer %s lower than start of line numbers"),
		 plongest (file_offset));
      if (file_offset > linetab_size)	/* Too big to be an offset?  */
	return;
      file_offset += linetab_offset;	/* Try reading at that linetab
					   offset.  */
    }

  rawptr = &linetab[file_offset - linetab_offset];

  /* Skip first line entry for each function.  */
  rawptr += local_linesz;
  /* Line numbers start at one for the first line of the function.  */
  first_line--;

  /* If the line number table is full (e.g. 64K lines in COFF debug
     info), the next function's L_LNNO32 might not be zero, so don't
     overstep the table's end in any case.  */
  while (rawptr <= &linetab[0] + linetab_size)
    {
      bfd_coff_swap_lineno_in (symfile_bfd, rawptr, &lptr);
      rawptr += local_linesz;
      /* The next function, or the sentinel, will have L_LNNO32 zero;
	 we exit.  */
      if (L_LNNO32 (&lptr) && L_LNNO32 (&lptr) <= last_line)
	{
	  CORE_ADDR addr = lptr.l_addr.l_paddr;
	  record_line (get_current_subfile (),
		       first_line + L_LNNO32 (&lptr),
		       unrelocated_addr (gdbarch_addr_bits_remove (gdbarch,
								   addr)));
	}
      else
	break;
    }
}

static void
patch_type (struct type *type, struct type *real_type)
{
  struct type *target = type->target_type ();
  struct type *real_target = real_type->target_type ();

  target->set_length (real_target->length ());
  target->copy_fields (real_target);

  if (real_target->name ())
    {
      /* The previous copy of TYPE_NAME is allocated by
	 process_coff_symbol.  */
      xfree ((char *) target->name ());
      target->set_name (xstrdup (real_target->name ()));
    }
}

/* Patch up all appropriate typedef symbols in the opaque_type_chains
   so that they can be used to print out opaque data structures
   properly.  */

static void
patch_opaque_types (struct symtab *s)
{
  /* Go through the per-file symbols only.  */
  const struct block *b = s->compunit ()->blockvector ()->static_block ();
  for (struct symbol *real_sym : block_iterator_range (b))
    {
      /* Find completed typedefs to use to fix opaque ones.
	 Remove syms from the chain when their types are stored,
	 but search the whole chain, as there may be several syms
	 from different files with the same name.  */
      if (real_sym->aclass () == LOC_TYPEDEF
	  && real_sym->domain () == VAR_DOMAIN
	  && real_sym->type ()->code () == TYPE_CODE_PTR
	  && real_sym->type ()->target_type ()->length () != 0)
	{
	  const char *name = real_sym->linkage_name ();
	  int hash = hashname (name);
	  struct symbol *sym, *prev;

	  prev = 0;
	  for (sym = opaque_type_chain[hash]; sym;)
	    {
	      if (name[0] == sym->linkage_name ()[0]
		  && strcmp (name + 1, sym->linkage_name () + 1) == 0)
		{
		  if (prev)
		    prev->set_value_chain (sym->value_chain ());
		  else
		    opaque_type_chain[hash] = sym->value_chain ();

		  patch_type (sym->type (), real_sym->type ());

		  if (prev)
		    sym = prev->value_chain ();
		  else
		    sym = opaque_type_chain[hash];
		}
	      else
		{
		  prev = sym;
		  sym->set_value_chain (sym);
		}
	    }
	}
    }
}

static int
coff_reg_to_regnum (struct symbol *sym, struct gdbarch *gdbarch)
{
  return gdbarch_sdb_reg_to_regnum (gdbarch, sym->value_longest ());
}

static const struct symbol_register_ops coff_register_funcs = {
  coff_reg_to_regnum
};

/* The "aclass" index for computed COFF symbols.  */

static int coff_register_index;

static struct symbol *
process_coff_symbol (struct coff_symbol *cs,
		     union internal_auxent *aux,
		     struct objfile *objfile)
{
  struct symbol *sym = new (&objfile->objfile_obstack) symbol;
  char *name;

  name = cs->c_name;
  name = EXTERNAL_NAME (name, objfile->obfd.get ());
  sym->set_language (get_current_subfile ()->language,
		     &objfile->objfile_obstack);
  sym->compute_and_set_names (name, true, objfile->per_bfd);

  /* default assumptions */
  sym->set_value_longest (cs->c_value);
  sym->set_domain (VAR_DOMAIN);
  sym->set_section_index (cs_to_section (cs, objfile));

  if (ISFCN (cs->c_type))
    {
      sym->set_value_longest
	(sym->value_longest () + objfile->text_section_offset ());
      sym->set_type
	(lookup_function_type (decode_function_type (cs, cs->c_type,
						     aux, objfile)));

      sym->set_aclass_index (LOC_BLOCK);
      if (cs->c_sclass == C_STAT || cs->c_sclass == C_THUMBSTAT
	  || cs->c_sclass == C_THUMBSTATFUNC)
	add_symbol_to_list (sym, get_file_symbols ());
      else if (cs->c_sclass == C_EXT || cs->c_sclass == C_THUMBEXT
	       || cs->c_sclass == C_THUMBEXTFUNC)
	add_symbol_to_list (sym, get_global_symbols ());
    }
  else
    {
      sym->set_type (decode_type (cs, cs->c_type, aux, objfile));
      switch (cs->c_sclass)
	{
	case C_NULL:
	  break;

	case C_AUTO:
	  sym->set_aclass_index (LOC_LOCAL);
	  add_symbol_to_list (sym, get_local_symbols ());
	  break;

	case C_THUMBEXT:
	case C_THUMBEXTFUNC:
	case C_EXT:
	  sym->set_aclass_index (LOC_STATIC);
	  sym->set_value_address ((CORE_ADDR) cs->c_value
				  + objfile->section_offsets[SECT_OFF_TEXT (objfile)]);
	  add_symbol_to_list (sym, get_global_symbols ());
	  break;

	case C_THUMBSTAT:
	case C_THUMBSTATFUNC:
	case C_STAT:
	  sym->set_aclass_index (LOC_STATIC);
	  sym->set_value_address ((CORE_ADDR) cs->c_value
				  + objfile->section_offsets[SECT_OFF_TEXT (objfile)]);
	  if (within_function)
	    {
	      /* Static symbol of local scope.  */
	      add_symbol_to_list (sym, get_local_symbols ());
	    }
	  else
	    {
	      /* Static symbol at top level of file.  */
	      add_symbol_to_list (sym, get_file_symbols ());
	    }
	  break;

#ifdef C_GLBLREG		/* AMD coff */
	case C_GLBLREG:
#endif
	case C_REG:
	  sym->set_aclass_index (coff_register_index);
	  sym->set_value_longest (cs->c_value);
	  add_symbol_to_list (sym, get_local_symbols ());
	  break;

	case C_THUMBLABEL:
	case C_LABEL:
	  break;

	case C_ARG:
	  sym->set_aclass_index (LOC_ARG);
	  sym->set_is_argument (1);
	  add_symbol_to_list (sym, get_local_symbols ());
	  break;

	case C_REGPARM:
	  sym->set_aclass_index (coff_register_index);
	  sym->set_is_argument (1);
	  sym->set_value_longest (cs->c_value);
	  add_symbol_to_list (sym, get_local_symbols ());
	  break;

	case C_TPDEF:
	  sym->set_aclass_index (LOC_TYPEDEF);
	  sym->set_domain (VAR_DOMAIN);

	  /* If type has no name, give it one.  */
	  if (sym->type ()->name () == 0)
	    {
	      if (sym->type ()->code () == TYPE_CODE_PTR
		  || sym->type ()->code () == TYPE_CODE_FUNC)
		{
		  /* If we are giving a name to a type such as
		     "pointer to foo" or "function returning foo", we
		     better not set the TYPE_NAME.  If the program
		     contains "typedef char *caddr_t;", we don't want 
		     all variables of type char * to print as caddr_t.
		     This is not just a consequence of GDB's type
		     management; CC and GCC (at least through version
		     2.4) both output variables of either type char *
		     or caddr_t with the type refering to the C_TPDEF
		     symbol for caddr_t.  If a future compiler cleans
		     this up it GDB is not ready for it yet, but if it
		     becomes ready we somehow need to disable this
		     check (without breaking the PCC/GCC2.4 case).

		     Sigh.

		     Fortunately, this check seems not to be necessary
		     for anything except pointers or functions.  */
		  ;
		}
	      else
		sym->type ()->set_name (xstrdup (sym->linkage_name ()));
	    }

	  /* Keep track of any type which points to empty structured
	     type, so it can be filled from a definition from another
	     file.  A simple forward reference (TYPE_CODE_UNDEF) is
	     not an empty structured type, though; the forward
	     references work themselves out via the magic of
	     coff_lookup_type.  */
	  if (sym->type ()->code () == TYPE_CODE_PTR
	      && sym->type ()->target_type ()->length () == 0
	      && sym->type ()->target_type ()->code ()
	      != TYPE_CODE_UNDEF)
	    {
	      int i = hashname (sym->linkage_name ());

	      sym->set_value_chain (opaque_type_chain[i]);
	      opaque_type_chain[i] = sym;
	    }
	  add_symbol_to_list (sym, get_file_symbols ());
	  break;

	case C_STRTAG:
	case C_UNTAG:
	case C_ENTAG:
	  sym->set_aclass_index (LOC_TYPEDEF);
	  sym->set_domain (STRUCT_DOMAIN);

	  /* Some compilers try to be helpful by inventing "fake"
	     names for anonymous enums, structures, and unions, like
	     "~0fake" or ".0fake".  Thanks, but no thanks...  */
	  if (sym->type ()->name () == 0)
	    if (sym->linkage_name () != NULL
		&& *sym->linkage_name () != '~'
		&& *sym->linkage_name () != '.')
	      sym->type ()->set_name (xstrdup (sym->linkage_name ()));

	  add_symbol_to_list (sym, get_file_symbols ());
	  break;

	default:
	  break;
	}
    }
  return sym;
}

/* Decode a coff type specifier;  return the type that is meant.  */

static struct type *
decode_type (struct coff_symbol *cs, unsigned int c_type,
	     union internal_auxent *aux, struct objfile *objfile)
{
  struct type *type = 0;
  unsigned int new_c_type;

  if (c_type & ~N_BTMASK)
    {
      new_c_type = DECREF (c_type);
      if (ISPTR (c_type))
	{
	  type = decode_type (cs, new_c_type, aux, objfile);
	  type = lookup_pointer_type (type);
	}
      else if (ISFCN (c_type))
	{
	  type = decode_type (cs, new_c_type, aux, objfile);
	  type = lookup_function_type (type);
	}
      else if (ISARY (c_type))
	{
	  int i, n;
	  unsigned short *dim;
	  struct type *base_type, *index_type, *range_type;

	  /* Define an array type.  */
	  /* auxent refers to array, not base type.  */
	  if (aux->x_sym.x_tagndx.u32 == 0)
	    cs->c_naux = 0;

	  /* Shift the indices down.  */
	  dim = &aux->x_sym.x_fcnary.x_ary.x_dimen[0];
	  i = 1;
	  n = dim[0];
	  for (i = 0; *dim && i < DIMNUM - 1; i++, dim++)
	    *dim = *(dim + 1);
	  *dim = 0;

	  base_type = decode_type (cs, new_c_type, aux, objfile);
	  index_type = builtin_type (objfile)->builtin_int;
	  type_allocator alloc (objfile, language_c);
	  range_type
	    = create_static_range_type (alloc, index_type, 0, n - 1);
	  type = create_array_type (alloc, base_type, range_type);
	}
      return type;
    }

  /* Reference to existing type.  This only occurs with the struct,
     union, and enum types.  EPI a29k coff fakes us out by producing
     aux entries with a nonzero x_tagndx for definitions of structs,
     unions, and enums, so we have to check the c_sclass field.  SCO
     3.2v4 cc gets confused with pointers to pointers to defined
     structs, and generates negative x_tagndx fields.  */
  if (cs->c_naux > 0 && aux->x_sym.x_tagndx.u32 != 0)
    {
      if (cs->c_sclass != C_STRTAG
	  && cs->c_sclass != C_UNTAG
	  && cs->c_sclass != C_ENTAG
	  && (int32_t) aux->x_sym.x_tagndx.u32 >= 0)
	{
	  type = coff_alloc_type (aux->x_sym.x_tagndx.u32);
	  return type;
	}
      else
	{
	  complaint (_("Symbol table entry for %s has bad tagndx value"),
		     cs->c_name);
	  /* And fall through to decode_base_type...  */
	}
    }

  return decode_base_type (cs, BTYPE (c_type), aux, objfile);
}

/* Decode a coff type specifier for function definition;
   return the type that the function returns.  */

static struct type *
decode_function_type (struct coff_symbol *cs, 
		      unsigned int c_type,
		      union internal_auxent *aux, 
		      struct objfile *objfile)
{
  if (aux->x_sym.x_tagndx.u32 == 0)
    cs->c_naux = 0;	/* auxent refers to function, not base
			   type.  */

  return decode_type (cs, DECREF (c_type), aux, objfile);
}

/* Basic C types.  */

static struct type *
decode_base_type (struct coff_symbol *cs, 
		  unsigned int c_type,
		  union internal_auxent *aux, 
		  struct objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  struct type *type;

  switch (c_type)
    {
    case T_NULL:
      /* Shows up with "void (*foo)();" structure members.  */
      return builtin_type (objfile)->builtin_void;

#ifdef T_VOID
    case T_VOID:
      /* Intel 960 COFF has this symbol and meaning.  */
      return builtin_type (objfile)->builtin_void;
#endif

    case T_CHAR:
      return builtin_type (objfile)->builtin_char;

    case T_SHORT:
      return builtin_type (objfile)->builtin_short;

    case T_INT:
      return builtin_type (objfile)->builtin_int;

    case T_LONG:
      if (cs->c_sclass == C_FIELD
	  && aux->x_sym.x_misc.x_lnsz.x_size
	     > gdbarch_long_bit (gdbarch))
	return builtin_type (objfile)->builtin_long_long;
      else
	return builtin_type (objfile)->builtin_long;

    case T_FLOAT:
      return builtin_type (objfile)->builtin_float;

    case T_DOUBLE:
      return builtin_type (objfile)->builtin_double;

    case T_LNGDBL:
      return builtin_type (objfile)->builtin_long_double;

    case T_STRUCT:
      if (cs->c_naux != 1)
	{
	  /* Anonymous structure type.  */
	  type = coff_alloc_type (cs->c_symnum);
	  type->set_code (TYPE_CODE_STRUCT);
	  type->set_name (NULL);
	  INIT_CPLUS_SPECIFIC (type);
	  type->set_length (0);
	  type->set_fields (nullptr);
	  type->set_num_fields (0);
	}
      else
	{
	  type = coff_read_struct_type (cs->c_symnum,
					aux->x_sym.x_misc.x_lnsz.x_size,
					aux->x_sym.x_fcnary.x_fcn.x_endndx.u32,
					objfile);
	}
      return type;

    case T_UNION:
      if (cs->c_naux != 1)
	{
	  /* Anonymous union type.  */
	  type = coff_alloc_type (cs->c_symnum);
	  type->set_name (NULL);
	  INIT_CPLUS_SPECIFIC (type);
	  type->set_length (0);
	  type->set_fields (nullptr);
	  type->set_num_fields (0);
	}
      else
	{
	  type = coff_read_struct_type (cs->c_symnum,
					aux->x_sym.x_misc.x_lnsz.x_size,
					aux->x_sym.x_fcnary.x_fcn.x_endndx.u32,
					objfile);
	}
      type->set_code (TYPE_CODE_UNION);
      return type;

    case T_ENUM:
      if (cs->c_naux != 1)
	{
	  /* Anonymous enum type.  */
	  type = coff_alloc_type (cs->c_symnum);
	  type->set_code (TYPE_CODE_ENUM);
	  type->set_name (NULL);
	  type->set_length (0);
	  type->set_fields (nullptr);
	  type->set_num_fields (0);
	}
      else
	{
	  type = coff_read_enum_type (cs->c_symnum,
				      aux->x_sym.x_misc.x_lnsz.x_size,
				      aux->x_sym.x_fcnary.x_fcn.x_endndx.u32,
				      objfile);
	}
      return type;

    case T_MOE:
      /* Shouldn't show up here.  */
      break;

    case T_UCHAR:
      return builtin_type (objfile)->builtin_unsigned_char;

    case T_USHORT:
      return builtin_type (objfile)->builtin_unsigned_short;

    case T_UINT:
      return builtin_type (objfile)->builtin_unsigned_int;

    case T_ULONG:
      if (cs->c_sclass == C_FIELD
	  && aux->x_sym.x_misc.x_lnsz.x_size
	     > gdbarch_long_bit (gdbarch))
	return builtin_type (objfile)->builtin_unsigned_long_long;
      else
	return builtin_type (objfile)->builtin_unsigned_long;
    }
  complaint (_("Unexpected type for symbol %s"), cs->c_name);
  return builtin_type (objfile)->builtin_void;
}

/* This page contains subroutines of read_type.  */

/* Read the description of a structure (or union type) and return an
   object describing the type.  */

static struct type *
coff_read_struct_type (int index, int length, int lastsym,
		       struct objfile *objfile)
{
  struct nextfield
    {
      struct nextfield *next;
      struct field field;
    };

  struct type *type;
  struct nextfield *list = 0;
  struct nextfield *newobj;
  int nfields = 0;
  int n;
  char *name;
  struct coff_symbol member_sym;
  struct coff_symbol *ms = &member_sym;
  struct internal_syment sub_sym;
  union internal_auxent sub_aux;
  int done = 0;

  type = coff_alloc_type (index);
  type->set_code (TYPE_CODE_STRUCT);
  INIT_CPLUS_SPECIFIC (type);
  type->set_length (length);

  while (!done && symnum < lastsym && symnum < nlist_nsyms_global)
    {
      read_one_sym (ms, &sub_sym, &sub_aux);
      name = ms->c_name;
      name = EXTERNAL_NAME (name, objfile->obfd.get ());

      switch (ms->c_sclass)
	{
	case C_MOS:
	case C_MOU:

	  /* Get space to record the next field's data.  */
	  newobj = XALLOCA (struct nextfield);
	  newobj->next = list;
	  list = newobj;

	  /* Save the data.  */
	  list->field.set_name (obstack_strdup (&objfile->objfile_obstack,
						name));
	  list->field.set_type (decode_type (ms, ms->c_type, &sub_aux,
					     objfile));
	  list->field.set_loc_bitpos (8 * ms->c_value);
	  list->field.set_bitsize (0);
	  nfields++;
	  break;

	case C_FIELD:

	  /* Get space to record the next field's data.  */
	  newobj = XALLOCA (struct nextfield);
	  newobj->next = list;
	  list = newobj;

	  /* Save the data.  */
	  list->field.set_name (obstack_strdup (&objfile->objfile_obstack,
						name));
	  list->field.set_type (decode_type (ms, ms->c_type, &sub_aux,
					     objfile));
	  list->field.set_loc_bitpos (ms->c_value);
	  list->field.set_bitsize (sub_aux.x_sym.x_misc.x_lnsz.x_size);
	  nfields++;
	  break;

	case C_EOS:
	  done = 1;
	  break;
	}
    }
  /* Now create the vector of fields, and record how big it is.  */

  type->alloc_fields (nfields);

  /* Copy the saved-up fields into the field vector.  */

  for (n = nfields; list; list = list->next)
    type->field (--n) = list->field;

  return type;
}

/* Read a definition of an enumeration type,
   and create and return a suitable type object.
   Also defines the symbols that represent the values of the type.  */

static struct type *
coff_read_enum_type (int index, int length, int lastsym,
		     struct objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  struct symbol *sym;
  struct type *type;
  int nsyms = 0;
  int done = 0;
  struct pending **symlist;
  struct coff_symbol member_sym;
  struct coff_symbol *ms = &member_sym;
  struct internal_syment sub_sym;
  union internal_auxent sub_aux;
  struct pending *osyms, *syms;
  int o_nsyms;
  int n;
  char *name;
  int unsigned_enum = 1;

  type = coff_alloc_type (index);
  if (within_function)
    symlist = get_local_symbols ();
  else
    symlist = get_file_symbols ();
  osyms = *symlist;
  o_nsyms = osyms ? osyms->nsyms : 0;

  while (!done && symnum < lastsym && symnum < nlist_nsyms_global)
    {
      read_one_sym (ms, &sub_sym, &sub_aux);
      name = ms->c_name;
      name = EXTERNAL_NAME (name, objfile->obfd.get ());

      switch (ms->c_sclass)
	{
	case C_MOE:
	  sym = new (&objfile->objfile_obstack) symbol;

	  name = obstack_strdup (&objfile->objfile_obstack, name);
	  sym->set_linkage_name (name);
	  sym->set_aclass_index (LOC_CONST);
	  sym->set_domain (VAR_DOMAIN);
	  sym->set_value_longest (ms->c_value);
	  add_symbol_to_list (sym, symlist);
	  nsyms++;
	  break;

	case C_EOS:
	  /* Sometimes the linker (on 386/ix 2.0.2 at least) screws
	     up the count of how many symbols to read.  So stop
	     on .eos.  */
	  done = 1;
	  break;
	}
    }

  /* Now fill in the fields of the type-structure.  */

  if (length > 0)
    type->set_length (length);
  else /* Assume ints.  */
    type->set_length (gdbarch_int_bit (gdbarch) / TARGET_CHAR_BIT);
  type->set_code (TYPE_CODE_ENUM);
  type->alloc_fields (nsyms);

  /* Find the symbols for the values and put them into the type.
     The symbols can be found in the symlist that we put them on
     to cause them to be defined.  osyms contains the old value
     of that symlist; everything up to there was defined by us.  */
  /* Note that we preserve the order of the enum constants, so
     that in something like "enum {FOO, LAST_THING=FOO}" we print
     FOO, not LAST_THING.  */

  for (syms = *symlist, n = 0; syms; syms = syms->next)
    {
      int j = 0;

      if (syms == osyms)
	j = o_nsyms;
      for (; j < syms->nsyms; j++, n++)
	{
	  struct symbol *xsym = syms->symbol[j];

	  xsym->set_type (type);
	  type->field (n).set_name (xsym->linkage_name ());
	  type->field (n).set_loc_enumval (xsym->value_longest ());
	  if (xsym->value_longest () < 0)
	    unsigned_enum = 0;
	  type->field (n).set_bitsize (0);
	}
      if (syms == osyms)
	break;
    }

  if (unsigned_enum)
    type->set_is_unsigned (true);

  return type;
}

/* Register our ability to parse symbols for coff BFD files.  */

static const struct sym_fns coff_sym_fns =
{
  coff_new_init,		/* sym_new_init: init anything gbl to
				   entire symtab */
  coff_symfile_init,		/* sym_init: read initial info, setup
				   for sym_read() */
  coff_symfile_read,		/* sym_read: read a symbol file into
				   symtab */
  coff_symfile_finish,		/* sym_finish: finished with file,
				   cleanup */
  default_symfile_offsets,	/* sym_offsets: xlate external to
				   internal form */
  default_symfile_segments,	/* sym_segments: Get segment
				   information from a file */
  NULL,                         /* sym_read_linetable  */

  default_symfile_relocate,	/* sym_relocate: Relocate a debug
				   section.  */
  NULL,				/* sym_probe_fns */
};

void _initialize_coffread ();
void
_initialize_coffread ()
{
  add_symtab_fns (bfd_target_coff_flavour, &coff_sym_fns);

  coff_register_index
    = register_symbol_register_impl (LOC_REGISTER, &coff_register_funcs);
}
