/* Support routines for decoding "stabs" debugging information format.

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

/* Support routines for reading and decoding debugging information in
   the "stabs" format.  This format is used by some systems that use
   COFF or ELF where the stabs data is placed in a special section (as
   well as with many old systems that used the a.out object file
   format).  Avoid placing any object file format specific code in
   this file.  */

#include "defs.h"
#include "bfd.h"
#include "gdbsupport/gdb_obstack.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "symfile.h"
#include "objfiles.h"
#include "aout/stab_gnu.h"
#include "libaout.h"
#include "aout/aout64.h"
#include "gdb-stabs.h"
#include "buildsym-legacy.h"
#include "complaints.h"
#include "demangle.h"
#include "gdb-demangle.h"
#include "language.h"
#include "target-float.h"
#include "c-lang.h"
#include "cp-abi.h"
#include "cp-support.h"
#include <ctype.h>

#include "stabsread.h"

/* See stabsread.h for these globals.  */
unsigned int symnum;
const char *(*next_symbol_text_func) (struct objfile *);
unsigned char processing_gcc_compilation;
int within_function;
struct symbol *global_sym_chain[HASHSIZE];
struct pending_stabs *global_stabs;
int previous_stab_code;
int *this_object_header_files;
int n_this_object_header_files;
int n_allocated_this_object_header_files;

struct stabs_nextfield
{
  struct stabs_nextfield *next;

  struct field field;
};

struct next_fnfieldlist
{
  struct next_fnfieldlist *next;
  struct fn_fieldlist fn_fieldlist;
};

/* The routines that read and process a complete stabs for a C struct or 
   C++ class pass lists of data member fields and lists of member function
   fields in an instance of a field_info structure, as defined below.
   This is part of some reorganization of low level C++ support and is
   expected to eventually go away...  (FIXME) */

struct stab_field_info
  {
    struct stabs_nextfield *list = nullptr;
    struct next_fnfieldlist *fnlist = nullptr;

    auto_obstack obstack;
  };

static void
read_one_struct_field (struct stab_field_info *, const char **, const char *,
		       struct type *, struct objfile *);

static struct type *dbx_alloc_type (int[2], struct objfile *);

static long read_huge_number (const char **, int, int *, int);

static struct type *error_type (const char **, struct objfile *);

static void
patch_block_stabs (struct pending *, struct pending_stabs *,
		   struct objfile *);

static int read_type_number (const char **, int *);

static struct type *read_type (const char **, struct objfile *);

static struct type *read_range_type (const char **, int[2],
				     int, struct objfile *);

static struct type *read_sun_builtin_type (const char **,
					   int[2], struct objfile *);

static struct type *read_sun_floating_type (const char **, int[2],
					    struct objfile *);

static struct type *read_enum_type (const char **, struct type *, struct objfile *);

static struct type *rs6000_builtin_type (int, struct objfile *);

static int
read_member_functions (struct stab_field_info *, const char **, struct type *,
		       struct objfile *);

static int
read_struct_fields (struct stab_field_info *, const char **, struct type *,
		    struct objfile *);

static int
read_baseclasses (struct stab_field_info *, const char **, struct type *,
		  struct objfile *);

static int
read_tilde_fields (struct stab_field_info *, const char **, struct type *,
		   struct objfile *);

static int attach_fn_fields_to_type (struct stab_field_info *, struct type *);

static int attach_fields_to_type (struct stab_field_info *, struct type *,
				  struct objfile *);

static struct type *read_struct_type (const char **, struct type *,
				      enum type_code,
				      struct objfile *);

static struct type *read_array_type (const char **, struct type *,
				     struct objfile *);

static struct field *read_args (const char **, int, struct objfile *,
				int *, int *);

static void add_undefined_type (struct type *, int[2]);

static int
read_cpp_abbrev (struct stab_field_info *, const char **, struct type *,
		 struct objfile *);

static const char *find_name_end (const char *name);

static int process_reference (const char **string);

void stabsread_clear_cache (void);

static const char vptr_name[] = "_vptr$";
static const char vb_name[] = "_vb$";

static void
invalid_cpp_abbrev_complaint (const char *arg1)
{
  complaint (_("invalid C++ abbreviation `%s'"), arg1);
}

static void
reg_value_complaint (int regnum, int num_regs, const char *sym)
{
  complaint (_("bad register number %d (max %d) in symbol %s"),
	     regnum, num_regs - 1, sym);
}

static void
stabs_general_complaint (const char *arg1)
{
  complaint ("%s", arg1);
}

/* Make a list of forward references which haven't been defined.  */

static struct type **undef_types;
static int undef_types_allocated;
static int undef_types_length;
static struct symbol *current_symbol = NULL;

/* Make a list of nameless types that are undefined.
   This happens when another type is referenced by its number
   before this type is actually defined.  For instance "t(0,1)=k(0,2)"
   and type (0,2) is defined only later.  */

struct nat
{
  int typenums[2];
  struct type *type;
};
static struct nat *noname_undefs;
static int noname_undefs_allocated;
static int noname_undefs_length;

/* Check for and handle cretinous stabs symbol name continuation!  */
#define STABS_CONTINUE(pp,objfile)				\
  do {							\
    if (**(pp) == '\\' || (**(pp) == '?' && (*(pp))[1] == '\0')) \
      *(pp) = next_symbol_text (objfile);	\
  } while (0)

/* Vector of types defined so far, indexed by their type numbers.
   (In newer sun systems, dbx uses a pair of numbers in parens,
   as in "(SUBFILENUM,NUMWITHINSUBFILE)".
   Then these numbers must be translated through the type_translations
   hash table to get the index into the type vector.)  */

static struct type **type_vector;

/* Number of elements allocated for type_vector currently.  */

static int type_vector_length;

/* Initial size of type vector.  Is realloc'd larger if needed, and
   realloc'd down to the size actually used, when completed.  */

#define INITIAL_TYPE_VECTOR_LENGTH 160


/* Look up a dbx type-number pair.  Return the address of the slot
   where the type for that number-pair is stored.
   The number-pair is in TYPENUMS.

   This can be used for finding the type associated with that pair
   or for associating a new type with the pair.  */

static struct type **
dbx_lookup_type (int typenums[2], struct objfile *objfile)
{
  int filenum = typenums[0];
  int index = typenums[1];
  unsigned old_len;
  int real_filenum;
  struct header_file *f;
  int f_orig_length;

  if (filenum == -1)		/* -1,-1 is for temporary types.  */
    return 0;

  if (filenum < 0 || filenum >= n_this_object_header_files)
    {
      complaint (_("Invalid symbol data: type number "
		   "(%d,%d) out of range at symtab pos %d."),
		 filenum, index, symnum);
      goto error_return;
    }

  if (filenum == 0)
    {
      if (index < 0)
	{
	  /* Caller wants address of address of type.  We think
	     that negative (rs6k builtin) types will never appear as
	     "lvalues", (nor should they), so we stuff the real type
	     pointer into a temp, and return its address.  If referenced,
	     this will do the right thing.  */
	  static struct type *temp_type;

	  temp_type = rs6000_builtin_type (index, objfile);
	  return &temp_type;
	}

      /* Type is defined outside of header files.
	 Find it in this object file's type vector.  */
      if (index >= type_vector_length)
	{
	  old_len = type_vector_length;
	  if (old_len == 0)
	    {
	      type_vector_length = INITIAL_TYPE_VECTOR_LENGTH;
	      type_vector = XNEWVEC (struct type *, type_vector_length);
	    }
	  while (index >= type_vector_length)
	    {
	      type_vector_length *= 2;
	    }
	  type_vector = (struct type **)
	    xrealloc ((char *) type_vector,
		      (type_vector_length * sizeof (struct type *)));
	  memset (&type_vector[old_len], 0,
		  (type_vector_length - old_len) * sizeof (struct type *));
	}
      return (&type_vector[index]);
    }
  else
    {
      real_filenum = this_object_header_files[filenum];

      if (real_filenum >= N_HEADER_FILES (objfile))
	{
	  static struct type *temp_type;

	  warning (_("GDB internal error: bad real_filenum"));

	error_return:
	  temp_type = builtin_type (objfile)->builtin_error;
	  return &temp_type;
	}

      f = HEADER_FILES (objfile) + real_filenum;

      f_orig_length = f->length;
      if (index >= f_orig_length)
	{
	  while (index >= f->length)
	    {
	      f->length *= 2;
	    }
	  f->vector = (struct type **)
	    xrealloc ((char *) f->vector, f->length * sizeof (struct type *));
	  memset (&f->vector[f_orig_length], 0,
		  (f->length - f_orig_length) * sizeof (struct type *));
	}
      return (&f->vector[index]);
    }
}

/* Make sure there is a type allocated for type numbers TYPENUMS
   and return the type object.
   This can create an empty (zeroed) type object.
   TYPENUMS may be (-1, -1) to return a new type object that is not
   put into the type vector, and so may not be referred to by number.  */

static struct type *
dbx_alloc_type (int typenums[2], struct objfile *objfile)
{
  struct type **type_addr;

  if (typenums[0] == -1)
    {
      return type_allocator (objfile,
			     get_current_subfile ()->language).new_type ();
    }

  type_addr = dbx_lookup_type (typenums, objfile);

  /* If we are referring to a type not known at all yet,
     allocate an empty type for it.
     We will fill it in later if we find out how.  */
  if (*type_addr == 0)
    {
      *type_addr = type_allocator (objfile,
				   get_current_subfile ()->language).new_type ();
    }

  return (*type_addr);
}

/* Allocate a floating-point type of size BITS.  */

static struct type *
dbx_init_float_type (struct objfile *objfile, int bits)
{
  struct gdbarch *gdbarch = objfile->arch ();
  const struct floatformat **format;
  struct type *type;

  format = gdbarch_floatformat_for_type (gdbarch, NULL, bits);
  type_allocator alloc (objfile, get_current_subfile ()->language);
  if (format)
    type = init_float_type (alloc, bits, NULL, format);
  else
    type = alloc.new_type (TYPE_CODE_ERROR, bits, NULL);

  return type;
}

/* for all the stabs in a given stab vector, build appropriate types 
   and fix their symbols in given symbol vector.  */

static void
patch_block_stabs (struct pending *symbols, struct pending_stabs *stabs,
		   struct objfile *objfile)
{
  int ii;
  char *name;
  const char *pp;
  struct symbol *sym;

  if (stabs)
    {
      /* for all the stab entries, find their corresponding symbols and 
	 patch their types!  */

      for (ii = 0; ii < stabs->count; ++ii)
	{
	  name = stabs->stab[ii];
	  pp = (char *) strchr (name, ':');
	  gdb_assert (pp);	/* Must find a ':' or game's over.  */
	  while (pp[1] == ':')
	    {
	      pp += 2;
	      pp = (char *) strchr (pp, ':');
	    }
	  sym = find_symbol_in_list (symbols, name, pp - name);
	  if (!sym)
	    {
	      /* FIXME-maybe: it would be nice if we noticed whether
		 the variable was defined *anywhere*, not just whether
		 it is defined in this compilation unit.  But neither
		 xlc or GCC seem to need such a definition, and until
		 we do psymtabs (so that the minimal symbols from all
		 compilation units are available now), I'm not sure
		 how to get the information.  */

	      /* On xcoff, if a global is defined and never referenced,
		 ld will remove it from the executable.  There is then
		 a N_GSYM stab for it, but no regular (C_EXT) symbol.  */
	      sym = new (&objfile->objfile_obstack) symbol;
	      sym->set_domain (VAR_DOMAIN);
	      sym->set_aclass_index (LOC_OPTIMIZED_OUT);
	      sym->set_linkage_name
		(obstack_strndup (&objfile->objfile_obstack, name, pp - name));
	      pp += 2;
	      if (*(pp - 1) == 'F' || *(pp - 1) == 'f')
		{
		  /* I don't think the linker does this with functions,
		     so as far as I know this is never executed.
		     But it doesn't hurt to check.  */
		  sym->set_type
		    (lookup_function_type (read_type (&pp, objfile)));
		}
	      else
		{
		  sym->set_type (read_type (&pp, objfile));
		}
	      add_symbol_to_list (sym, get_global_symbols ());
	    }
	  else
	    {
	      pp += 2;
	      if (*(pp - 1) == 'F' || *(pp - 1) == 'f')
		{
		  sym->set_type
		    (lookup_function_type (read_type (&pp, objfile)));
		}
	      else
		{
		  sym->set_type (read_type (&pp, objfile));
		}
	    }
	}
    }
}


/* Read a number by which a type is referred to in dbx data,
   or perhaps read a pair (FILENUM, TYPENUM) in parentheses.
   Just a single number N is equivalent to (0,N).
   Return the two numbers by storing them in the vector TYPENUMS.
   TYPENUMS will then be used as an argument to dbx_lookup_type.

   Returns 0 for success, -1 for error.  */

static int
read_type_number (const char **pp, int *typenums)
{
  int nbits;

  if (**pp == '(')
    {
      (*pp)++;
      typenums[0] = read_huge_number (pp, ',', &nbits, 0);
      if (nbits != 0)
	return -1;
      typenums[1] = read_huge_number (pp, ')', &nbits, 0);
      if (nbits != 0)
	return -1;
    }
  else
    {
      typenums[0] = 0;
      typenums[1] = read_huge_number (pp, 0, &nbits, 0);
      if (nbits != 0)
	return -1;
    }
  return 0;
}


#define VISIBILITY_PRIVATE	'0'	/* Stabs character for private field */
#define VISIBILITY_PROTECTED	'1'	/* Stabs character for protected fld */
#define VISIBILITY_PUBLIC	'2'	/* Stabs character for public field */
#define VISIBILITY_IGNORE	'9'	/* Optimized out or zero length */

/* Structure for storing pointers to reference definitions for fast lookup 
   during "process_later".  */

struct ref_map
{
  const char *stabs;
  CORE_ADDR value;
  struct symbol *sym;
};

#define MAX_CHUNK_REFS 100
#define REF_CHUNK_SIZE (MAX_CHUNK_REFS * sizeof (struct ref_map))
#define REF_MAP_SIZE(ref_chunk) ((ref_chunk) * REF_CHUNK_SIZE)

static struct ref_map *ref_map;

/* Ptr to free cell in chunk's linked list.  */
static int ref_count = 0;

/* Number of chunks malloced.  */
static int ref_chunk = 0;

/* This file maintains a cache of stabs aliases found in the symbol
   table.  If the symbol table changes, this cache must be cleared
   or we are left holding onto data in invalid obstacks.  */
void
stabsread_clear_cache (void)
{
  ref_count = 0;
  ref_chunk = 0;
}

/* Create array of pointers mapping refids to symbols and stab strings.
   Add pointers to reference definition symbols and/or their values as we 
   find them, using their reference numbers as our index.
   These will be used later when we resolve references.  */
void
ref_add (int refnum, struct symbol *sym, const char *stabs, CORE_ADDR value)
{
  if (ref_count == 0)
    ref_chunk = 0;
  if (refnum >= ref_count)
    ref_count = refnum + 1;
  if (ref_count > ref_chunk * MAX_CHUNK_REFS)
    {
      int new_slots = ref_count - ref_chunk * MAX_CHUNK_REFS;
      int new_chunks = new_slots / MAX_CHUNK_REFS + 1;

      ref_map = (struct ref_map *)
	xrealloc (ref_map, REF_MAP_SIZE (ref_chunk + new_chunks));
      memset (ref_map + ref_chunk * MAX_CHUNK_REFS, 0, 
	      new_chunks * REF_CHUNK_SIZE);
      ref_chunk += new_chunks;
    }
  ref_map[refnum].stabs = stabs;
  ref_map[refnum].sym = sym;
  ref_map[refnum].value = value;
}

/* Return defined sym for the reference REFNUM.  */
struct symbol *
ref_search (int refnum)
{
  if (refnum < 0 || refnum > ref_count)
    return 0;
  return ref_map[refnum].sym;
}

/* Parse a reference id in STRING and return the resulting
   reference number.  Move STRING beyond the reference id.  */

static int
process_reference (const char **string)
{
  const char *p;
  int refnum = 0;

  if (**string != '#')
    return 0;

  /* Advance beyond the initial '#'.  */
  p = *string + 1;

  /* Read number as reference id.  */
  while (*p && isdigit (*p))
    {
      refnum = refnum * 10 + *p - '0';
      p++;
    }
  *string = p;
  return refnum;
}

/* If STRING defines a reference, store away a pointer to the reference 
   definition for later use.  Return the reference number.  */

int
symbol_reference_defined (const char **string)
{
  const char *p = *string;
  int refnum = 0;

  refnum = process_reference (&p);

  /* Defining symbols end in '='.  */
  if (*p == '=')
    {
      /* Symbol is being defined here.  */
      *string = p + 1;
      return refnum;
    }
  else
    {
      /* Must be a reference.  Either the symbol has already been defined,
	 or this is a forward reference to it.  */
      *string = p;
      return -1;
    }
}

static int
stab_reg_to_regnum (struct symbol *sym, struct gdbarch *gdbarch)
{
  int regno = gdbarch_stab_reg_to_regnum (gdbarch, sym->value_longest ());

  if (regno < 0 || regno >= gdbarch_num_cooked_regs (gdbarch))
    {
      reg_value_complaint (regno, gdbarch_num_cooked_regs (gdbarch),
			   sym->print_name ());

      regno = gdbarch_sp_regnum (gdbarch); /* Known safe, though useless.  */
    }

  return regno;
}

static const struct symbol_register_ops stab_register_funcs = {
  stab_reg_to_regnum
};

/* The "aclass" indices for computed symbols.  */

static int stab_register_index;
static int stab_regparm_index;

struct symbol *
define_symbol (CORE_ADDR valu, const char *string, int desc, int type,
	       struct objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  struct symbol *sym;
  const char *p = find_name_end (string);
  int deftype;
  int synonym = 0;
  int i;

  /* We would like to eliminate nameless symbols, but keep their types.
     E.g. stab entry ":t10=*2" should produce a type 10, which is a pointer
     to type 2, but, should not create a symbol to address that type.  Since
     the symbol will be nameless, there is no way any user can refer to it.  */

  int nameless;

  /* Ignore syms with empty names.  */
  if (string[0] == 0)
    return 0;

  /* Ignore old-style symbols from cc -go.  */
  if (p == 0)
    return 0;

  while (p[1] == ':')
    {
      p += 2;
      p = strchr (p, ':');
      if (p == NULL)
	{
	  complaint (
		     _("Bad stabs string '%s'"), string);
	  return NULL;
	}
    }

  /* If a nameless stab entry, all we need is the type, not the symbol.
     e.g. ":t10=*2" or a nameless enum like " :T16=ered:0,green:1,blue:2,;" */
  nameless = (p == string || ((string[0] == ' ') && (string[1] == ':')));

  current_symbol = sym = new (&objfile->objfile_obstack) symbol;

  if (processing_gcc_compilation)
    {
      /* GCC 2.x puts the line number in desc.  SunOS apparently puts in the
	 number of bytes occupied by a type or object, which we ignore.  */
      sym->set_line (desc);
    }
  else
    {
      sym->set_line (0);	/* unknown */
    }

  sym->set_language (get_current_subfile ()->language,
		     &objfile->objfile_obstack);

  if (is_cplus_marker (string[0]))
    {
      /* Special GNU C++ names.  */
      switch (string[1])
	{
	case 't':
	  sym->set_linkage_name ("this");
	  break;

	case 'v':		/* $vtbl_ptr_type */
	  goto normal;

	case 'e':
	  sym->set_linkage_name ("eh_throw");
	  break;

	case '_':
	  /* This was an anonymous type that was never fixed up.  */
	  goto normal;

	default:
	  complaint (_("Unknown C++ symbol name `%s'"),
		     string);
	  goto normal;		/* Do *something* with it.  */
	}
    }
  else
    {
    normal:
      gdb::unique_xmalloc_ptr<char> new_name;

      if (sym->language () == language_cplus)
	{
	  std::string name (string, p - string);
	  new_name = cp_canonicalize_string (name.c_str ());
	}
      else if (sym->language () == language_c)
	{
	  std::string name (string, p - string);
	  new_name = c_canonicalize_name (name.c_str ());
	}
      if (new_name != nullptr)
	sym->compute_and_set_names (new_name.get (), true, objfile->per_bfd);
      else
	sym->compute_and_set_names (std::string_view (string, p - string), true,
				    objfile->per_bfd);

      if (sym->language () == language_cplus)
	cp_scan_for_anonymous_namespaces (get_buildsym_compunit (), sym,
					  objfile);

    }
  p++;

  /* Determine the type of name being defined.  */
#if 0
  /* Getting GDB to correctly skip the symbol on an undefined symbol
     descriptor and not ever dump core is a very dodgy proposition if
     we do things this way.  I say the acorn RISC machine can just
     fix their compiler.  */
  /* The Acorn RISC machine's compiler can put out locals that don't
     start with "234=" or "(3,4)=", so assume anything other than the
     deftypes we know how to handle is a local.  */
  if (!strchr ("cfFGpPrStTvVXCR", *p))
#else
  if (isdigit (*p) || *p == '(' || *p == '-')
#endif
    deftype = 'l';
  else
    deftype = *p++;

  switch (deftype)
    {
    case 'c':
      /* c is a special case, not followed by a type-number.
	 SYMBOL:c=iVALUE for an integer constant symbol.
	 SYMBOL:c=rVALUE for a floating constant symbol.
	 SYMBOL:c=eTYPE,INTVALUE for an enum constant symbol.
	 e.g. "b:c=e6,0" for "const b = blob1"
	 (where type 6 is defined by "blobs:t6=eblob1:0,blob2:1,;").  */
      if (*p != '=')
	{
	  sym->set_aclass_index (LOC_CONST);
	  sym->set_type (error_type (&p, objfile));
	  sym->set_domain (VAR_DOMAIN);
	  add_symbol_to_list (sym, get_file_symbols ());
	  return sym;
	}
      ++p;
      switch (*p++)
	{
	case 'r':
	  {
	    gdb_byte *dbl_valu;
	    struct type *dbl_type;

	    dbl_type = builtin_type (objfile)->builtin_double;
	    dbl_valu
	      = (gdb_byte *) obstack_alloc (&objfile->objfile_obstack,
					    dbl_type->length ());

	    target_float_from_string (dbl_valu, dbl_type, std::string (p));

	    sym->set_type (dbl_type);
	    sym->set_value_bytes (dbl_valu);
	    sym->set_aclass_index (LOC_CONST_BYTES);
	  }
	  break;
	case 'i':
	  {
	    /* Defining integer constants this way is kind of silly,
	       since 'e' constants allows the compiler to give not
	       only the value, but the type as well.  C has at least
	       int, long, unsigned int, and long long as constant
	       types; other languages probably should have at least
	       unsigned as well as signed constants.  */

	    sym->set_type (builtin_type (objfile)->builtin_long);
	    sym->set_value_longest (atoi (p));
	    sym->set_aclass_index (LOC_CONST);
	  }
	  break;

	case 'c':
	  {
	    sym->set_type (builtin_type (objfile)->builtin_char);
	    sym->set_value_longest (atoi (p));
	    sym->set_aclass_index (LOC_CONST);
	  }
	  break;

	case 's':
	  {
	    struct type *range_type;
	    int ind = 0;
	    char quote = *p++;
	    gdb_byte *string_local = (gdb_byte *) alloca (strlen (p));
	    gdb_byte *string_value;

	    if (quote != '\'' && quote != '"')
	      {
		sym->set_aclass_index (LOC_CONST);
		sym->set_type (error_type (&p, objfile));
		sym->set_domain (VAR_DOMAIN);
		add_symbol_to_list (sym, get_file_symbols ());
		return sym;
	      }

	    /* Find matching quote, rejecting escaped quotes.  */
	    while (*p && *p != quote)
	      {
		if (*p == '\\' && p[1] == quote)
		  {
		    string_local[ind] = (gdb_byte) quote;
		    ind++;
		    p += 2;
		  }
		else if (*p) 
		  {
		    string_local[ind] = (gdb_byte) (*p);
		    ind++;
		    p++;
		  }
	      }
	    if (*p != quote)
	      {
		sym->set_aclass_index (LOC_CONST);
		sym->set_type (error_type (&p, objfile));
		sym->set_domain (VAR_DOMAIN);
		add_symbol_to_list (sym, get_file_symbols ());
		return sym;
	      }

	    /* NULL terminate the string.  */
	    string_local[ind] = 0;
	    type_allocator alloc (objfile, get_current_subfile ()->language);
	    range_type
	      = create_static_range_type (alloc,
					  builtin_type (objfile)->builtin_int,
					  0, ind);
	    sym->set_type
	      (create_array_type (alloc, builtin_type (objfile)->builtin_char,
				  range_type));
	    string_value
	      = (gdb_byte *) obstack_alloc (&objfile->objfile_obstack, ind + 1);
	    memcpy (string_value, string_local, ind + 1);
	    p++;

	    sym->set_value_bytes (string_value);
	    sym->set_aclass_index (LOC_CONST_BYTES);
	  }
	  break;

	case 'e':
	  /* SYMBOL:c=eTYPE,INTVALUE for a constant symbol whose value
	     can be represented as integral.
	     e.g. "b:c=e6,0" for "const b = blob1"
	     (where type 6 is defined by "blobs:t6=eblob1:0,blob2:1,;").  */
	  {
	    sym->set_aclass_index (LOC_CONST);
	    sym->set_type (read_type (&p, objfile));

	    if (*p != ',')
	      {
		sym->set_type (error_type (&p, objfile));
		break;
	      }
	    ++p;

	    /* If the value is too big to fit in an int (perhaps because
	       it is unsigned), or something like that, we silently get
	       a bogus value.  The type and everything else about it is
	       correct.  Ideally, we should be using whatever we have
	       available for parsing unsigned and long long values,
	       however.  */
	    sym->set_value_longest (atoi (p));
	  }
	  break;
	default:
	  {
	    sym->set_aclass_index (LOC_CONST);
	    sym->set_type (error_type (&p, objfile));
	  }
	}
      sym->set_domain (VAR_DOMAIN);
      add_symbol_to_list (sym, get_file_symbols ());
      return sym;

    case 'C':
      /* The name of a caught exception.  */
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (LOC_LABEL);
      sym->set_domain (VAR_DOMAIN);
      sym->set_value_address (valu);
      add_symbol_to_list (sym, get_local_symbols ());
      break;

    case 'f':
      /* A static function definition.  */
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (LOC_BLOCK);
      sym->set_domain (VAR_DOMAIN);
      add_symbol_to_list (sym, get_file_symbols ());
      /* fall into process_function_types.  */

    process_function_types:
      /* Function result types are described as the result type in stabs.
	 We need to convert this to the function-returning-type-X type
	 in GDB.  E.g. "int" is converted to "function returning int".  */
      if (sym->type ()->code () != TYPE_CODE_FUNC)
	sym->set_type (lookup_function_type (sym->type ()));

      /* All functions in C++ have prototypes.  Stabs does not offer an
	 explicit way to identify prototyped or unprototyped functions,
	 but both GCC and Sun CC emit stabs for the "call-as" type rather
	 than the "declared-as" type for unprototyped functions, so
	 we treat all functions as if they were prototyped.  This is used
	 primarily for promotion when calling the function from GDB.  */
      sym->type ()->set_is_prototyped (true);

      /* fall into process_prototype_types.  */

    process_prototype_types:
      /* Sun acc puts declared types of arguments here.  */
      if (*p == ';')
	{
	  struct type *ftype = sym->type ();
	  int nsemi = 0;
	  int nparams = 0;
	  const char *p1 = p;

	  /* Obtain a worst case guess for the number of arguments
	     by counting the semicolons.  */
	  while (*p1)
	    {
	      if (*p1++ == ';')
		nsemi++;
	    }

	  /* Allocate parameter information fields and fill them in.  */
	  ftype->alloc_fields (nsemi);
	  while (*p++ == ';')
	    {
	      struct type *ptype;

	      /* A type number of zero indicates the start of varargs.
		 FIXME: GDB currently ignores vararg functions.  */
	      if (p[0] == '0' && p[1] == '\0')
		break;
	      ptype = read_type (&p, objfile);

	      /* The Sun compilers mark integer arguments, which should
		 be promoted to the width of the calling conventions, with
		 a type which references itself.  This type is turned into
		 a TYPE_CODE_VOID type by read_type, and we have to turn
		 it back into builtin_int here.
		 FIXME: Do we need a new builtin_promoted_int_arg ?  */
	      if (ptype->code () == TYPE_CODE_VOID)
		ptype = builtin_type (objfile)->builtin_int;
	      ftype->field (nparams).set_type (ptype);
	      ftype->field (nparams).set_is_artificial (false);
	      nparams++;
	    }
	  ftype->set_num_fields (nparams);
	  ftype->set_is_prototyped (true);
	}
      break;

    case 'F':
      /* A global function definition.  */
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (LOC_BLOCK);
      sym->set_domain (VAR_DOMAIN);
      add_symbol_to_list (sym, get_global_symbols ());
      goto process_function_types;

    case 'G':
      /* For a class G (global) symbol, it appears that the
	 value is not correct.  It is necessary to search for the
	 corresponding linker definition to find the value.
	 These definitions appear at the end of the namelist.  */
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (LOC_STATIC);
      sym->set_domain (VAR_DOMAIN);
      /* Don't add symbol references to global_sym_chain.
	 Symbol references don't have valid names and wont't match up with
	 minimal symbols when the global_sym_chain is relocated.
	 We'll fixup symbol references when we fixup the defining symbol.  */
      if (sym->linkage_name () && sym->linkage_name ()[0] != '#')
	{
	  i = hashname (sym->linkage_name ());
	  sym->set_value_chain (global_sym_chain[i]);
	  global_sym_chain[i] = sym;
	}
      add_symbol_to_list (sym, get_global_symbols ());
      break;

      /* This case is faked by a conditional above,
	 when there is no code letter in the dbx data.
	 Dbx data never actually contains 'l'.  */
    case 's':
    case 'l':
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (LOC_LOCAL);
      sym->set_value_longest (valu);
      sym->set_domain (VAR_DOMAIN);
      add_symbol_to_list (sym, get_local_symbols ());
      break;

    case 'p':
      if (*p == 'F')
	/* pF is a two-letter code that means a function parameter in Fortran.
	   The type-number specifies the type of the return value.
	   Translate it into a pointer-to-function type.  */
	{
	  p++;
	  sym->set_type
	    (lookup_pointer_type
	       (lookup_function_type (read_type (&p, objfile))));
	}
      else
	sym->set_type (read_type (&p, objfile));

      sym->set_aclass_index (LOC_ARG);
      sym->set_value_longest (valu);
      sym->set_domain (VAR_DOMAIN);
      sym->set_is_argument (1);
      add_symbol_to_list (sym, get_local_symbols ());

      if (gdbarch_byte_order (gdbarch) != BFD_ENDIAN_BIG)
	{
	  /* On little-endian machines, this crud is never necessary,
	     and, if the extra bytes contain garbage, is harmful.  */
	  break;
	}

      /* If it's gcc-compiled, if it says `short', believe it.  */
      if (processing_gcc_compilation
	  || gdbarch_believe_pcc_promotion (gdbarch))
	break;

      if (!gdbarch_believe_pcc_promotion (gdbarch))
	{
	  /* If PCC says a parameter is a short or a char, it is
	     really an int.  */
	  if (sym->type ()->length ()
	      < gdbarch_int_bit (gdbarch) / TARGET_CHAR_BIT
	      && sym->type ()->code () == TYPE_CODE_INT)
	    {
	      sym->set_type
		(sym->type ()->is_unsigned ()
		 ? builtin_type (objfile)->builtin_unsigned_int
		 : builtin_type (objfile)->builtin_int);
	    }
	  break;
	}
      [[fallthrough]];

    case 'P':
      /* acc seems to use P to declare the prototypes of functions that
	 are referenced by this file.  gdb is not prepared to deal
	 with this extra information.  FIXME, it ought to.  */
      if (type == N_FUN)
	{
	  sym->set_type (read_type (&p, objfile));
	  goto process_prototype_types;
	}
      [[fallthrough]];

    case 'R':
      /* Parameter which is in a register.  */
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (stab_register_index);
      sym->set_is_argument (1);
      sym->set_value_longest (valu);
      sym->set_domain (VAR_DOMAIN);
      add_symbol_to_list (sym, get_local_symbols ());
      break;

    case 'r':
      /* Register variable (either global or local).  */
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (stab_register_index);
      sym->set_value_longest (valu);
      sym->set_domain (VAR_DOMAIN);
      if (within_function)
	{
	  /* Sun cc uses a pair of symbols, one 'p' and one 'r', with
	     the same name to represent an argument passed in a
	     register.  GCC uses 'P' for the same case.  So if we find
	     such a symbol pair we combine it into one 'P' symbol.
	     For Sun cc we need to do this regardless of stabs_argument_has_addr, because the compiler puts out
	     the 'p' symbol even if it never saves the argument onto
	     the stack.

	     On most machines, we want to preserve both symbols, so
	     that we can still get information about what is going on
	     with the stack (VAX for computing args_printed, using
	     stack slots instead of saved registers in backtraces,
	     etc.).

	     Note that this code illegally combines
	     main(argc) struct foo argc; { register struct foo argc; }
	     but this case is considered pathological and causes a warning
	     from a decent compiler.  */

	  struct pending *local_symbols = *get_local_symbols ();
	  if (local_symbols
	      && local_symbols->nsyms > 0
	      && gdbarch_stabs_argument_has_addr (gdbarch, sym->type ()))
	    {
	      struct symbol *prev_sym;

	      prev_sym = local_symbols->symbol[local_symbols->nsyms - 1];
	      if ((prev_sym->aclass () == LOC_REF_ARG
		   || prev_sym->aclass () == LOC_ARG)
		  && strcmp (prev_sym->linkage_name (),
			     sym->linkage_name ()) == 0)
		{
		  prev_sym->set_aclass_index (stab_register_index);
		  /* Use the type from the LOC_REGISTER; that is the type
		     that is actually in that register.  */
		  prev_sym->set_type (sym->type ());
		  prev_sym->set_value_longest (sym->value_longest ());
		  sym = prev_sym;
		  break;
		}
	    }
	  add_symbol_to_list (sym, get_local_symbols ());
	}
      else
	add_symbol_to_list (sym, get_file_symbols ());
      break;

    case 'S':
      /* Static symbol at top level of file.  */
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (LOC_STATIC);
      sym->set_value_address (valu);
      sym->set_domain (VAR_DOMAIN);
      add_symbol_to_list (sym, get_file_symbols ());
      break;

    case 't':
      /* In Ada, there is no distinction between typedef and non-typedef;
	 any type declaration implicitly has the equivalent of a typedef,
	 and thus 't' is in fact equivalent to 'Tt'.

	 Therefore, for Ada units, we check the character immediately
	 before the 't', and if we do not find a 'T', then make sure to
	 create the associated symbol in the STRUCT_DOMAIN ('t' definitions
	 will be stored in the VAR_DOMAIN).  If the symbol was indeed
	 defined as 'Tt' then the STRUCT_DOMAIN symbol will be created
	 elsewhere, so we don't need to take care of that.
	 
	 This is important to do, because of forward references:
	 The cleanup of undefined types stored in undef_types only uses
	 STRUCT_DOMAIN symbols to perform the replacement.  */
      synonym = (sym->language () == language_ada && p[-2] != 'T');

      /* Typedef */
      sym->set_type (read_type (&p, objfile));

      /* For a nameless type, we don't want a create a symbol, thus we
	 did not use `sym'.  Return without further processing.  */
      if (nameless)
	return NULL;

      sym->set_aclass_index (LOC_TYPEDEF);
      sym->set_value_longest (valu);
      sym->set_domain (VAR_DOMAIN);
      /* C++ vagaries: we may have a type which is derived from
	 a base type which did not have its name defined when the
	 derived class was output.  We fill in the derived class's
	 base part member's name here in that case.  */
      if (sym->type ()->name () != NULL)
	if ((sym->type ()->code () == TYPE_CODE_STRUCT
	     || sym->type ()->code () == TYPE_CODE_UNION)
	    && TYPE_N_BASECLASSES (sym->type ()))
	  {
	    int j;

	    for (j = TYPE_N_BASECLASSES (sym->type ()) - 1; j >= 0; j--)
	      if (TYPE_BASECLASS_NAME (sym->type (), j) == 0)
		sym->type ()->field (j).set_name
		  (TYPE_BASECLASS (sym->type (), j)->name ());
	  }

      if (sym->type ()->name () == NULL)
	{
	  if ((sym->type ()->code () == TYPE_CODE_PTR
	       && strcmp (sym->linkage_name (), vtbl_ptr_name))
	      || sym->type ()->code () == TYPE_CODE_FUNC)
	    {
	      /* If we are giving a name to a type such as "pointer to
		 foo" or "function returning foo", we better not set
		 the TYPE_NAME.  If the program contains "typedef char
		 *caddr_t;", we don't want all variables of type char
		 * to print as caddr_t.  This is not just a
		 consequence of GDB's type management; PCC and GCC (at
		 least through version 2.4) both output variables of
		 either type char * or caddr_t with the type number
		 defined in the 't' symbol for caddr_t.  If a future
		 compiler cleans this up it GDB is not ready for it
		 yet, but if it becomes ready we somehow need to
		 disable this check (without breaking the PCC/GCC2.4
		 case).

		 Sigh.

		 Fortunately, this check seems not to be necessary
		 for anything except pointers or functions.  */
	      /* ezannoni: 2000-10-26.  This seems to apply for
		 versions of gcc older than 2.8.  This was the original
		 problem: with the following code gdb would tell that
		 the type for name1 is caddr_t, and func is char().

		 typedef char *caddr_t;
		 char *name2;
		 struct x
		 {
		   char *name1;
		 } xx;
		 char *func()
		 {
		 }
		 main () {}
		 */

	      /* Pascal accepts names for pointer types.  */
	      if (get_current_subfile ()->language == language_pascal)
		sym->type ()->set_name (sym->linkage_name ());
	    }
	  else
	    sym->type ()->set_name (sym->linkage_name ());
	}

      add_symbol_to_list (sym, get_file_symbols ());

      if (synonym)
	{
	  /* Create the STRUCT_DOMAIN clone.  */
	  struct symbol *struct_sym = new (&objfile->objfile_obstack) symbol;

	  *struct_sym = *sym;
	  struct_sym->set_aclass_index (LOC_TYPEDEF);
	  struct_sym->set_value_longest (valu);
	  struct_sym->set_domain (STRUCT_DOMAIN);
	  if (sym->type ()->name () == 0)
	    sym->type ()->set_name
	      (obconcat (&objfile->objfile_obstack, sym->linkage_name (),
			 (char *) NULL));
	  add_symbol_to_list (struct_sym, get_file_symbols ());
	}

      break;

    case 'T':
      /* Struct, union, or enum tag.  For GNU C++, this can be be followed
	 by 't' which means we are typedef'ing it as well.  */
      synonym = *p == 't';

      if (synonym)
	p++;

      sym->set_type (read_type (&p, objfile));
 
      /* For a nameless type, we don't want a create a symbol, thus we
	 did not use `sym'.  Return without further processing.  */
      if (nameless)
	return NULL;

      sym->set_aclass_index (LOC_TYPEDEF);
      sym->set_value_longest (valu);
      sym->set_domain (STRUCT_DOMAIN);
      if (sym->type ()->name () == 0)
	sym->type ()->set_name
	  (obconcat (&objfile->objfile_obstack, sym->linkage_name (),
		     (char *) NULL));
      add_symbol_to_list (sym, get_file_symbols ());

      if (synonym)
	{
	  /* Clone the sym and then modify it.  */
	  struct symbol *typedef_sym = new (&objfile->objfile_obstack) symbol;

	  *typedef_sym = *sym;
	  typedef_sym->set_aclass_index (LOC_TYPEDEF);
	  typedef_sym->set_value_longest (valu);
	  typedef_sym->set_domain (VAR_DOMAIN);
	  if (sym->type ()->name () == 0)
	    sym->type ()->set_name
	      (obconcat (&objfile->objfile_obstack, sym->linkage_name (),
			 (char *) NULL));
	  add_symbol_to_list (typedef_sym, get_file_symbols ());
	}
      break;

    case 'V':
      /* Static symbol of local scope.  */
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (LOC_STATIC);
      sym->set_value_address (valu);
      sym->set_domain (VAR_DOMAIN);
      add_symbol_to_list (sym, get_local_symbols ());
      break;

    case 'v':
      /* Reference parameter */
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (LOC_REF_ARG);
      sym->set_is_argument (1);
      sym->set_value_longest (valu);
      sym->set_domain (VAR_DOMAIN);
      add_symbol_to_list (sym, get_local_symbols ());
      break;

    case 'a':
      /* Reference parameter which is in a register.  */
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (stab_regparm_index);
      sym->set_is_argument (1);
      sym->set_value_longest (valu);
      sym->set_domain (VAR_DOMAIN);
      add_symbol_to_list (sym, get_local_symbols ());
      break;

    case 'X':
      /* This is used by Sun FORTRAN for "function result value".
	 Sun claims ("dbx and dbxtool interfaces", 2nd ed)
	 that Pascal uses it too, but when I tried it Pascal used
	 "x:3" (local symbol) instead.  */
      sym->set_type (read_type (&p, objfile));
      sym->set_aclass_index (LOC_LOCAL);
      sym->set_value_longest (valu);
      sym->set_domain (VAR_DOMAIN);
      add_symbol_to_list (sym, get_local_symbols ());
      break;

    default:
      sym->set_type (error_type (&p, objfile));
      sym->set_aclass_index (LOC_CONST);
      sym->set_value_longest (0);
      sym->set_domain (VAR_DOMAIN);
      add_symbol_to_list (sym, get_file_symbols ());
      break;
    }

  /* Some systems pass variables of certain types by reference instead
     of by value, i.e. they will pass the address of a structure (in a
     register or on the stack) instead of the structure itself.  */

  if (gdbarch_stabs_argument_has_addr (gdbarch, sym->type ())
      && sym->is_argument ())
    {
      /* We have to convert LOC_REGISTER to LOC_REGPARM_ADDR (for
	 variables passed in a register).  */
      if (sym->aclass () == LOC_REGISTER)
	sym->set_aclass_index (LOC_REGPARM_ADDR);
      /* Likewise for converting LOC_ARG to LOC_REF_ARG (for the 7th
	 and subsequent arguments on SPARC, for example).  */
      else if (sym->aclass () == LOC_ARG)
	sym->set_aclass_index (LOC_REF_ARG);
    }

  return sym;
}

/* Skip rest of this symbol and return an error type.

   General notes on error recovery:  error_type always skips to the
   end of the symbol (modulo cretinous dbx symbol name continuation).
   Thus code like this:

   if (*(*pp)++ != ';')
   return error_type (pp, objfile);

   is wrong because if *pp starts out pointing at '\0' (typically as the
   result of an earlier error), it will be incremented to point to the
   start of the next symbol, which might produce strange results, at least
   if you run off the end of the string table.  Instead use

   if (**pp != ';')
   return error_type (pp, objfile);
   ++*pp;

   or

   if (**pp != ';')
   foo = error_type (pp, objfile);
   else
   ++*pp;

   And in case it isn't obvious, the point of all this hair is so the compiler
   can define new types and new syntaxes, and old versions of the
   debugger will be able to read the new symbol tables.  */

static struct type *
error_type (const char **pp, struct objfile *objfile)
{
  complaint (_("couldn't parse type; debugger out of date?"));
  while (1)
    {
      /* Skip to end of symbol.  */
      while (**pp != '\0')
	{
	  (*pp)++;
	}

      /* Check for and handle cretinous dbx symbol name continuation!  */
      if ((*pp)[-1] == '\\' || (*pp)[-1] == '?')
	{
	  *pp = next_symbol_text (objfile);
	}
      else
	{
	  break;
	}
    }
  return builtin_type (objfile)->builtin_error;
}


/* Allocate a stub method whose return type is TYPE.  This apparently
   happens for speed of symbol reading, since parsing out the
   arguments to the method is cpu-intensive, the way we are doing it.
   So, we will fill in arguments later.  This always returns a fresh
   type.  */

static struct type *
allocate_stub_method (struct type *type)
{
  struct type *mtype;

  mtype = type_allocator (type).new_type ();
  mtype->set_code (TYPE_CODE_METHOD);
  mtype->set_length (1);
  mtype->set_is_stub (true);
  mtype->set_target_type (type);
  /* TYPE_SELF_TYPE (mtype) = unknown yet */
  return mtype;
}

/* Read type information or a type definition; return the type.  Even
   though this routine accepts either type information or a type
   definition, the distinction is relevant--some parts of stabsread.c
   assume that type information starts with a digit, '-', or '(' in
   deciding whether to call read_type.  */

static struct type *
read_type (const char **pp, struct objfile *objfile)
{
  struct type *type = 0;
  struct type *type1;
  int typenums[2];
  char type_descriptor;

  /* Size in bits of type if specified by a type attribute, or -1 if
     there is no size attribute.  */
  int type_size = -1;

  /* Used to distinguish string and bitstring from char-array and set.  */
  int is_string = 0;

  /* Used to distinguish vector from array.  */
  int is_vector = 0;

  /* Read type number if present.  The type number may be omitted.
     for instance in a two-dimensional array declared with type
     "ar1;1;10;ar1;1;10;4".  */
  if ((**pp >= '0' && **pp <= '9')
      || **pp == '('
      || **pp == '-')
    {
      if (read_type_number (pp, typenums) != 0)
	return error_type (pp, objfile);

      if (**pp != '=')
	{
	  /* Type is not being defined here.  Either it already
	     exists, or this is a forward reference to it.
	     dbx_alloc_type handles both cases.  */
	  type = dbx_alloc_type (typenums, objfile);

	  /* If this is a forward reference, arrange to complain if it
	     doesn't get patched up by the time we're done
	     reading.  */
	  if (type->code () == TYPE_CODE_UNDEF)
	    add_undefined_type (type, typenums);

	  return type;
	}

      /* Type is being defined here.  */
      /* Skip the '='.
	 Also skip the type descriptor - we get it below with (*pp)[-1].  */
      (*pp) += 2;
    }
  else
    {
      /* 'typenums=' not present, type is anonymous.  Read and return
	 the definition, but don't put it in the type vector.  */
      typenums[0] = typenums[1] = -1;
      (*pp)++;
    }

again:
  type_descriptor = (*pp)[-1];
  switch (type_descriptor)
    {
    case 'x':
      {
	enum type_code code;

	/* Used to index through file_symbols.  */
	struct pending *ppt;
	int i;

	/* Name including "struct", etc.  */
	char *type_name;

	{
	  const char *from, *p, *q1, *q2;

	  /* Set the type code according to the following letter.  */
	  switch ((*pp)[0])
	    {
	    case 's':
	      code = TYPE_CODE_STRUCT;
	      break;
	    case 'u':
	      code = TYPE_CODE_UNION;
	      break;
	    case 'e':
	      code = TYPE_CODE_ENUM;
	      break;
	    default:
	      {
		/* Complain and keep going, so compilers can invent new
		   cross-reference types.  */
		complaint (_("Unrecognized cross-reference type `%c'"),
			   (*pp)[0]);
		code = TYPE_CODE_STRUCT;
		break;
	      }
	    }

	  q1 = strchr (*pp, '<');
	  p = strchr (*pp, ':');
	  if (p == NULL)
	    return error_type (pp, objfile);
	  if (q1 && p > q1 && p[1] == ':')
	    {
	      int nesting_level = 0;

	      for (q2 = q1; *q2; q2++)
		{
		  if (*q2 == '<')
		    nesting_level++;
		  else if (*q2 == '>')
		    nesting_level--;
		  else if (*q2 == ':' && nesting_level == 0)
		    break;
		}
	      p = q2;
	      if (*p != ':')
		return error_type (pp, objfile);
	    }
	  type_name = NULL;
	  if (get_current_subfile ()->language == language_cplus)
	    {
	      std::string name (*pp, p - *pp);
	      gdb::unique_xmalloc_ptr<char> new_name
		= cp_canonicalize_string (name.c_str ());
	      if (new_name != nullptr)
		type_name = obstack_strdup (&objfile->objfile_obstack,
					    new_name.get ());
	    }
	  else if (get_current_subfile ()->language == language_c)
	    {
	      std::string name (*pp, p - *pp);
	      gdb::unique_xmalloc_ptr<char> new_name
		= c_canonicalize_name (name.c_str ());
	      if (new_name != nullptr)
		type_name = obstack_strdup (&objfile->objfile_obstack,
					    new_name.get ());
	    }
	  if (type_name == NULL)
	    {
	      char *to = type_name = (char *)
		obstack_alloc (&objfile->objfile_obstack, p - *pp + 1);

	      /* Copy the name.  */
	      from = *pp + 1;
	      while (from < p)
		*to++ = *from++;
	      *to = '\0';
	    }

	  /* Set the pointer ahead of the name which we just read, and
	     the colon.  */
	  *pp = p + 1;
	}

	/* If this type has already been declared, then reuse the same
	   type, rather than allocating a new one.  This saves some
	   memory.  */

	for (ppt = *get_file_symbols (); ppt; ppt = ppt->next)
	  for (i = 0; i < ppt->nsyms; i++)
	    {
	      struct symbol *sym = ppt->symbol[i];

	      if (sym->aclass () == LOC_TYPEDEF
		  && sym->domain () == STRUCT_DOMAIN
		  && (sym->type ()->code () == code)
		  && strcmp (sym->linkage_name (), type_name) == 0)
		{
		  obstack_free (&objfile->objfile_obstack, type_name);
		  type = sym->type ();
		  if (typenums[0] != -1)
		    *dbx_lookup_type (typenums, objfile) = type;
		  return type;
		}
	    }

	/* Didn't find the type to which this refers, so we must
	   be dealing with a forward reference.  Allocate a type
	   structure for it, and keep track of it so we can
	   fill in the rest of the fields when we get the full
	   type.  */
	type = dbx_alloc_type (typenums, objfile);
	type->set_code (code);
	type->set_name (type_name);
	INIT_CPLUS_SPECIFIC (type);
	type->set_is_stub (true);

	add_undefined_type (type, typenums);
	return type;
      }

    case '-':			/* RS/6000 built-in type */
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
    case '(':
      (*pp)--;

      /* We deal with something like t(1,2)=(3,4)=... which
	 the Lucid compiler and recent gcc versions (post 2.7.3) use.  */

      /* Allocate and enter the typedef type first.
	 This handles recursive types.  */
      type = dbx_alloc_type (typenums, objfile);
      type->set_code (TYPE_CODE_TYPEDEF);
      {
	struct type *xtype = read_type (pp, objfile);

	if (type == xtype)
	  {
	    /* It's being defined as itself.  That means it is "void".  */
	    type->set_code (TYPE_CODE_VOID);
	    type->set_length (1);
	  }
	else if (type_size >= 0 || is_string)
	  {
	    /* This is the absolute wrong way to construct types.  Every
	       other debug format has found a way around this problem and
	       the related problems with unnecessarily stubbed types;
	       someone motivated should attempt to clean up the issue
	       here as well.  Once a type pointed to has been created it
	       should not be modified.

	       Well, it's not *absolutely* wrong.  Constructing recursive
	       types (trees, linked lists) necessarily entails modifying
	       types after creating them.  Constructing any loop structure
	       entails side effects.  The Dwarf 2 reader does handle this
	       more gracefully (it never constructs more than once
	       instance of a type object, so it doesn't have to copy type
	       objects wholesale), but it still mutates type objects after
	       other folks have references to them.

	       Keep in mind that this circularity/mutation issue shows up
	       at the source language level, too: C's "incomplete types",
	       for example.  So the proper cleanup, I think, would be to
	       limit GDB's type smashing to match exactly those required
	       by the source language.  So GDB could have a
	       "complete_this_type" function, but never create unnecessary
	       copies of a type otherwise.  */
	    replace_type (type, xtype);
	    type->set_name (NULL);
	  }
	else
	  {
	    type->set_target_is_stub (true);
	    type->set_target_type (xtype);
	  }
      }
      break;

      /* In the following types, we must be sure to overwrite any existing
	 type that the typenums refer to, rather than allocating a new one
	 and making the typenums point to the new one.  This is because there
	 may already be pointers to the existing type (if it had been
	 forward-referenced), and we must change it to a pointer, function,
	 reference, or whatever, *in-place*.  */

    case '*':			/* Pointer to another type */
      type1 = read_type (pp, objfile);
      type = make_pointer_type (type1, dbx_lookup_type (typenums, objfile));
      break;

    case '&':			/* Reference to another type */
      type1 = read_type (pp, objfile);
      type = make_reference_type (type1, dbx_lookup_type (typenums, objfile),
				  TYPE_CODE_REF);
      break;

    case 'f':			/* Function returning another type */
      type1 = read_type (pp, objfile);
      type = make_function_type (type1, dbx_lookup_type (typenums, objfile));
      break;

    case 'g':                   /* Prototyped function.  (Sun)  */
      {
	/* Unresolved questions:

	   - According to Sun's ``STABS Interface Manual'', for 'f'
	   and 'F' symbol descriptors, a `0' in the argument type list
	   indicates a varargs function.  But it doesn't say how 'g'
	   type descriptors represent that info.  Someone with access
	   to Sun's toolchain should try it out.

	   - According to the comment in define_symbol (search for
	   `process_prototype_types:'), Sun emits integer arguments as
	   types which ref themselves --- like `void' types.  Do we
	   have to deal with that here, too?  Again, someone with
	   access to Sun's toolchain should try it out and let us
	   know.  */

	const char *type_start = (*pp) - 1;
	struct type *return_type = read_type (pp, objfile);
	struct type *func_type
	  = make_function_type (return_type,
				dbx_lookup_type (typenums, objfile));
	struct type_list {
	  struct type *type;
	  struct type_list *next;
	} *arg_types = 0;
	int num_args = 0;

	while (**pp && **pp != '#')
	  {
	    struct type *arg_type = read_type (pp, objfile);
	    struct type_list *newobj = XALLOCA (struct type_list);
	    newobj->type = arg_type;
	    newobj->next = arg_types;
	    arg_types = newobj;
	    num_args++;
	  }
	if (**pp == '#')
	  ++*pp;
	else
	  {
	    complaint (_("Prototyped function type didn't "
			 "end arguments with `#':\n%s"),
		       type_start);
	  }

	/* If there is just one argument whose type is `void', then
	   that's just an empty argument list.  */
	if (arg_types
	    && ! arg_types->next
	    && arg_types->type->code () == TYPE_CODE_VOID)
	  num_args = 0;

	func_type->alloc_fields (num_args);
	{
	  int i;
	  struct type_list *t;

	  /* We stuck each argument type onto the front of the list
	     when we read it, so the list is reversed.  Build the
	     fields array right-to-left.  */
	  for (t = arg_types, i = num_args - 1; t; t = t->next, i--)
	    func_type->field (i).set_type (t->type);
	}
	func_type->set_num_fields (num_args);
	func_type->set_is_prototyped (true);

	type = func_type;
	break;
      }

    case 'k':			/* Const qualifier on some type (Sun) */
      type = read_type (pp, objfile);
      type = make_cv_type (1, TYPE_VOLATILE (type), type,
			   dbx_lookup_type (typenums, objfile));
      break;

    case 'B':			/* Volatile qual on some type (Sun) */
      type = read_type (pp, objfile);
      type = make_cv_type (TYPE_CONST (type), 1, type,
			   dbx_lookup_type (typenums, objfile));
      break;

    case '@':
      if (isdigit (**pp) || **pp == '(' || **pp == '-')
	{			/* Member (class & variable) type */
	  /* FIXME -- we should be doing smash_to_XXX types here.  */

	  struct type *domain = read_type (pp, objfile);
	  struct type *memtype;

	  if (**pp != ',')
	    /* Invalid member type data format.  */
	    return error_type (pp, objfile);
	  ++*pp;

	  memtype = read_type (pp, objfile);
	  type = dbx_alloc_type (typenums, objfile);
	  smash_to_memberptr_type (type, domain, memtype);
	}
      else
	/* type attribute */
	{
	  const char *attr = *pp;

	  /* Skip to the semicolon.  */
	  while (**pp != ';' && **pp != '\0')
	    ++(*pp);
	  if (**pp == '\0')
	    return error_type (pp, objfile);
	  else
	    ++ * pp;		/* Skip the semicolon.  */

	  switch (*attr)
	    {
	    case 's':		/* Size attribute */
	      type_size = atoi (attr + 1);
	      if (type_size <= 0)
		type_size = -1;
	      break;

	    case 'S':		/* String attribute */
	      /* FIXME: check to see if following type is array?  */
	      is_string = 1;
	      break;

	    case 'V':		/* Vector attribute */
	      /* FIXME: check to see if following type is array?  */
	      is_vector = 1;
	      break;

	    default:
	      /* Ignore unrecognized type attributes, so future compilers
		 can invent new ones.  */
	      break;
	    }
	  ++*pp;
	  goto again;
	}
      break;

    case '#':			/* Method (class & fn) type */
      if ((*pp)[0] == '#')
	{
	  /* We'll get the parameter types from the name.  */
	  struct type *return_type;

	  (*pp)++;
	  return_type = read_type (pp, objfile);
	  if (*(*pp)++ != ';')
	    complaint (_("invalid (minimal) member type "
			 "data format at symtab pos %d."),
		       symnum);
	  type = allocate_stub_method (return_type);
	  if (typenums[0] != -1)
	    *dbx_lookup_type (typenums, objfile) = type;
	}
      else
	{
	  struct type *domain = read_type (pp, objfile);
	  struct type *return_type;
	  struct field *args;
	  int nargs, varargs;

	  if (**pp != ',')
	    /* Invalid member type data format.  */
	    return error_type (pp, objfile);
	  else
	    ++(*pp);

	  return_type = read_type (pp, objfile);
	  args = read_args (pp, ';', objfile, &nargs, &varargs);
	  if (args == NULL)
	    return error_type (pp, objfile);
	  type = dbx_alloc_type (typenums, objfile);
	  smash_to_method_type (type, domain, return_type, args,
				nargs, varargs);
	}
      break;

    case 'r':			/* Range type */
      type = read_range_type (pp, typenums, type_size, objfile);
      if (typenums[0] != -1)
	*dbx_lookup_type (typenums, objfile) = type;
      break;

    case 'b':
	{
	  /* Sun ACC builtin int type */
	  type = read_sun_builtin_type (pp, typenums, objfile);
	  if (typenums[0] != -1)
	    *dbx_lookup_type (typenums, objfile) = type;
	}
      break;

    case 'R':			/* Sun ACC builtin float type */
      type = read_sun_floating_type (pp, typenums, objfile);
      if (typenums[0] != -1)
	*dbx_lookup_type (typenums, objfile) = type;
      break;

    case 'e':			/* Enumeration type */
      type = dbx_alloc_type (typenums, objfile);
      type = read_enum_type (pp, type, objfile);
      if (typenums[0] != -1)
	*dbx_lookup_type (typenums, objfile) = type;
      break;

    case 's':			/* Struct type */
    case 'u':			/* Union type */
      {
	enum type_code type_code = TYPE_CODE_UNDEF;
	type = dbx_alloc_type (typenums, objfile);
	switch (type_descriptor)
	  {
	  case 's':
	    type_code = TYPE_CODE_STRUCT;
	    break;
	  case 'u':
	    type_code = TYPE_CODE_UNION;
	    break;
	  }
	type = read_struct_type (pp, type, type_code, objfile);
	break;
      }

    case 'a':			/* Array type */
      if (**pp != 'r')
	return error_type (pp, objfile);
      ++*pp;

      type = dbx_alloc_type (typenums, objfile);
      type = read_array_type (pp, type, objfile);
      if (is_string)
	type->set_code (TYPE_CODE_STRING);
      if (is_vector)
	make_vector_type (type);
      break;

    case 'S':			/* Set type */
      {
	type1 = read_type (pp, objfile);
	type_allocator alloc (objfile, get_current_subfile ()->language);
	type = create_set_type (alloc, type1);
	if (typenums[0] != -1)
	  *dbx_lookup_type (typenums, objfile) = type;
      }
      break;

    default:
      --*pp;			/* Go back to the symbol in error.  */
      /* Particularly important if it was \0!  */
      return error_type (pp, objfile);
    }

  if (type == 0)
    {
      warning (_("GDB internal error, type is NULL in stabsread.c."));
      return error_type (pp, objfile);
    }

  /* Size specified in a type attribute overrides any other size.  */
  if (type_size != -1)
    type->set_length ((type_size + TARGET_CHAR_BIT - 1) / TARGET_CHAR_BIT);

  return type;
}

/* RS/6000 xlc/dbx combination uses a set of builtin types, starting from -1.
   Return the proper type node for a given builtin type number.  */

static const registry<objfile>::key<struct type *,
				    gdb::noop_deleter<struct type *>>
  rs6000_builtin_type_data;

static struct type *
rs6000_builtin_type (int typenum, struct objfile *objfile)
{
  struct type **negative_types = rs6000_builtin_type_data.get (objfile);

  /* We recognize types numbered from -NUMBER_RECOGNIZED to -1.  */
#define NUMBER_RECOGNIZED 34
  struct type *rettype = NULL;

  if (typenum >= 0 || typenum < -NUMBER_RECOGNIZED)
    {
      complaint (_("Unknown builtin type %d"), typenum);
      return builtin_type (objfile)->builtin_error;
    }

  if (!negative_types)
    {
      /* This includes an empty slot for type number -0.  */
      negative_types = OBSTACK_CALLOC (&objfile->objfile_obstack,
				       NUMBER_RECOGNIZED + 1, struct type *);
      rs6000_builtin_type_data.set (objfile, negative_types);
    }

  if (negative_types[-typenum] != NULL)
    return negative_types[-typenum];

#if TARGET_CHAR_BIT != 8
#error This code wrong for TARGET_CHAR_BIT not 8
  /* These definitions all assume that TARGET_CHAR_BIT is 8.  I think
     that if that ever becomes not true, the correct fix will be to
     make the size in the struct type to be in bits, not in units of
     TARGET_CHAR_BIT.  */
#endif

  type_allocator alloc (objfile, get_current_subfile ()->language);
  switch (-typenum)
    {
    case 1:
      /* The size of this and all the other types are fixed, defined
	 by the debugging format.  If there is a type called "int" which
	 is other than 32 bits, then it should use a new negative type
	 number (or avoid negative type numbers for that case).
	 See stabs.texinfo.  */
      rettype = init_integer_type (alloc, 32, 0, "int");
      break;
    case 2:
      rettype = init_integer_type (alloc, 8, 0, "char");
      rettype->set_has_no_signedness (true);
      break;
    case 3:
      rettype = init_integer_type (alloc, 16, 0, "short");
      break;
    case 4:
      rettype = init_integer_type (alloc, 32, 0, "long");
      break;
    case 5:
      rettype = init_integer_type (alloc, 8, 1, "unsigned char");
      break;
    case 6:
      rettype = init_integer_type (alloc, 8, 0, "signed char");
      break;
    case 7:
      rettype = init_integer_type (alloc, 16, 1, "unsigned short");
      break;
    case 8:
      rettype = init_integer_type (alloc, 32, 1, "unsigned int");
      break;
    case 9:
      rettype = init_integer_type (alloc, 32, 1, "unsigned");
      break;
    case 10:
      rettype = init_integer_type (alloc, 32, 1, "unsigned long");
      break;
    case 11:
      rettype = alloc.new_type (TYPE_CODE_VOID, TARGET_CHAR_BIT, "void");
      break;
    case 12:
      /* IEEE single precision (32 bit).  */
      rettype = init_float_type (alloc, 32, "float",
				 floatformats_ieee_single);
      break;
    case 13:
      /* IEEE double precision (64 bit).  */
      rettype = init_float_type (alloc, 64, "double",
				 floatformats_ieee_double);
      break;
    case 14:
      /* This is an IEEE double on the RS/6000, and different machines with
	 different sizes for "long double" should use different negative
	 type numbers.  See stabs.texinfo.  */
      rettype = init_float_type (alloc, 64, "long double",
				 floatformats_ieee_double);
      break;
    case 15:
      rettype = init_integer_type (alloc, 32, 0, "integer");
      break;
    case 16:
      rettype = init_boolean_type (alloc, 32, 1, "boolean");
      break;
    case 17:
      rettype = init_float_type (alloc, 32, "short real",
				 floatformats_ieee_single);
      break;
    case 18:
      rettype = init_float_type (alloc, 64, "real",
				 floatformats_ieee_double);
      break;
    case 19:
      rettype = alloc.new_type (TYPE_CODE_ERROR, 0, "stringptr");
      break;
    case 20:
      rettype = init_character_type (alloc, 8, 1, "character");
      break;
    case 21:
      rettype = init_boolean_type (alloc, 8, 1, "logical*1");
      break;
    case 22:
      rettype = init_boolean_type (alloc, 16, 1, "logical*2");
      break;
    case 23:
      rettype = init_boolean_type (alloc, 32, 1, "logical*4");
      break;
    case 24:
      rettype = init_boolean_type (alloc, 32, 1, "logical");
      break;
    case 25:
      /* Complex type consisting of two IEEE single precision values.  */
      rettype = init_complex_type ("complex",
				   rs6000_builtin_type (12, objfile));
      break;
    case 26:
      /* Complex type consisting of two IEEE double precision values.  */
      rettype = init_complex_type ("double complex",
				   rs6000_builtin_type (13, objfile));
      break;
    case 27:
      rettype = init_integer_type (alloc, 8, 0, "integer*1");
      break;
    case 28:
      rettype = init_integer_type (alloc, 16, 0, "integer*2");
      break;
    case 29:
      rettype = init_integer_type (alloc, 32, 0, "integer*4");
      break;
    case 30:
      rettype = init_character_type (alloc, 16, 0, "wchar");
      break;
    case 31:
      rettype = init_integer_type (alloc, 64, 0, "long long");
      break;
    case 32:
      rettype = init_integer_type (alloc, 64, 1, "unsigned long long");
      break;
    case 33:
      rettype = init_integer_type (alloc, 64, 1, "logical*8");
      break;
    case 34:
      rettype = init_integer_type (alloc, 64, 0, "integer*8");
      break;
    }
  negative_types[-typenum] = rettype;
  return rettype;
}

/* This page contains subroutines of read_type.  */

/* Wrapper around method_name_from_physname to flag a complaint
   if there is an error.  */

static char *
stabs_method_name_from_physname (const char *physname)
{
  char *method_name;

  method_name = method_name_from_physname (physname);

  if (method_name == NULL)
    {
      complaint (_("Method has bad physname %s\n"), physname);
      return NULL;
    }

  return method_name;
}

/* Read member function stabs info for C++ classes.  The form of each member
   function data is:

   NAME :: TYPENUM[=type definition] ARGS : PHYSNAME ;

   An example with two member functions is:

   afunc1::20=##15;:i;2A.;afunc2::20:i;2A.;

   For the case of overloaded operators, the format is op$::*.funcs, where
   $ is the CPLUS_MARKER (usually '$'), `*' holds the place for an operator
   name (such as `+=') and `.' marks the end of the operator name.

   Returns 1 for success, 0 for failure.  */

static int
read_member_functions (struct stab_field_info *fip, const char **pp,
		       struct type *type, struct objfile *objfile)
{
  int nfn_fields = 0;
  int length = 0;
  int i;
  struct next_fnfield
    {
      struct next_fnfield *next;
      struct fn_field fn_field;
    }
   *sublist;
  struct type *look_ahead_type;
  struct next_fnfieldlist *new_fnlist;
  struct next_fnfield *new_sublist;
  char *main_fn_name;
  const char *p;

  /* Process each list until we find something that is not a member function
     or find the end of the functions.  */

  while (**pp != ';')
    {
      /* We should be positioned at the start of the function name.
	 Scan forward to find the first ':' and if it is not the
	 first of a "::" delimiter, then this is not a member function.  */
      p = *pp;
      while (*p != ':')
	{
	  p++;
	}
      if (p[1] != ':')
	{
	  break;
	}

      sublist = NULL;
      look_ahead_type = NULL;
      length = 0;

      new_fnlist = OBSTACK_ZALLOC (&fip->obstack, struct next_fnfieldlist);

      if ((*pp)[0] == 'o' && (*pp)[1] == 'p' && is_cplus_marker ((*pp)[2]))
	{
	  /* This is a completely wierd case.  In order to stuff in the
	     names that might contain colons (the usual name delimiter),
	     Mike Tiemann defined a different name format which is
	     signalled if the identifier is "op$".  In that case, the
	     format is "op$::XXXX." where XXXX is the name.  This is
	     used for names like "+" or "=".  YUUUUUUUK!  FIXME!  */
	  /* This lets the user type "break operator+".
	     We could just put in "+" as the name, but that wouldn't
	     work for "*".  */
	  static char opname[32] = "op$";
	  char *o = opname + 3;

	  /* Skip past '::'.  */
	  *pp = p + 2;

	  STABS_CONTINUE (pp, objfile);
	  p = *pp;
	  while (*p != '.')
	    {
	      *o++ = *p++;
	    }
	  main_fn_name = savestring (opname, o - opname);
	  /* Skip past '.'  */
	  *pp = p + 1;
	}
      else
	{
	  main_fn_name = savestring (*pp, p - *pp);
	  /* Skip past '::'.  */
	  *pp = p + 2;
	}
      new_fnlist->fn_fieldlist.name = main_fn_name;

      do
	{
	  new_sublist = OBSTACK_ZALLOC (&fip->obstack, struct next_fnfield);

	  /* Check for and handle cretinous dbx symbol name continuation!  */
	  if (look_ahead_type == NULL)
	    {
	      /* Normal case.  */
	      STABS_CONTINUE (pp, objfile);

	      new_sublist->fn_field.type = read_type (pp, objfile);
	      if (**pp != ':')
		{
		  /* Invalid symtab info for member function.  */
		  return 0;
		}
	    }
	  else
	    {
	      /* g++ version 1 kludge */
	      new_sublist->fn_field.type = look_ahead_type;
	      look_ahead_type = NULL;
	    }

	  (*pp)++;
	  p = *pp;
	  while (*p != ';')
	    {
	      p++;
	    }

	  /* These are methods, not functions.  */
	  if (new_sublist->fn_field.type->code () == TYPE_CODE_FUNC)
	    new_sublist->fn_field.type->set_code (TYPE_CODE_METHOD);

	  /* If this is just a stub, then we don't have the real name here.  */
	  if (new_sublist->fn_field.type->is_stub ())
	    {
	      if (!TYPE_SELF_TYPE (new_sublist->fn_field.type))
		set_type_self_type (new_sublist->fn_field.type, type);
	      new_sublist->fn_field.is_stub = 1;
	    }

	  new_sublist->fn_field.physname = savestring (*pp, p - *pp);
	  *pp = p + 1;

	  /* Set this member function's visibility fields.  */
	  switch (*(*pp)++)
	    {
	    case VISIBILITY_PRIVATE:
	      new_sublist->fn_field.accessibility = accessibility::PRIVATE;
	      break;
	    case VISIBILITY_PROTECTED:
	      new_sublist->fn_field.accessibility = accessibility::PROTECTED;
	      break;
	    }

	  STABS_CONTINUE (pp, objfile);
	  switch (**pp)
	    {
	    case 'A':		/* Normal functions.  */
	      new_sublist->fn_field.is_const = 0;
	      new_sublist->fn_field.is_volatile = 0;
	      (*pp)++;
	      break;
	    case 'B':		/* `const' member functions.  */
	      new_sublist->fn_field.is_const = 1;
	      new_sublist->fn_field.is_volatile = 0;
	      (*pp)++;
	      break;
	    case 'C':		/* `volatile' member function.  */
	      new_sublist->fn_field.is_const = 0;
	      new_sublist->fn_field.is_volatile = 1;
	      (*pp)++;
	      break;
	    case 'D':		/* `const volatile' member function.  */
	      new_sublist->fn_field.is_const = 1;
	      new_sublist->fn_field.is_volatile = 1;
	      (*pp)++;
	      break;
	    case '*':		/* File compiled with g++ version 1 --
				   no info.  */
	    case '?':
	    case '.':
	      break;
	    default:
	      complaint (_("const/volatile indicator missing, got '%c'"),
			 **pp);
	      break;
	    }

	  switch (*(*pp)++)
	    {
	    case '*':
	      {
		int nbits;
		/* virtual member function, followed by index.
		   The sign bit is set to distinguish pointers-to-methods
		   from virtual function indicies.  Since the array is
		   in words, the quantity must be shifted left by 1
		   on 16 bit machine, and by 2 on 32 bit machine, forcing
		   the sign bit out, and usable as a valid index into
		   the array.  Remove the sign bit here.  */
		new_sublist->fn_field.voffset =
		  (0x7fffffff & read_huge_number (pp, ';', &nbits, 0)) + 2;
		if (nbits != 0)
		  return 0;

		STABS_CONTINUE (pp, objfile);
		if (**pp == ';' || **pp == '\0')
		  {
		    /* Must be g++ version 1.  */
		    new_sublist->fn_field.fcontext = 0;
		  }
		else
		  {
		    /* Figure out from whence this virtual function came.
		       It may belong to virtual function table of
		       one of its baseclasses.  */
		    look_ahead_type = read_type (pp, objfile);
		    if (**pp == ':')
		      {
			/* g++ version 1 overloaded methods.  */
		      }
		    else
		      {
			new_sublist->fn_field.fcontext = look_ahead_type;
			if (**pp != ';')
			  {
			    return 0;
			  }
			else
			  {
			    ++*pp;
			  }
			look_ahead_type = NULL;
		      }
		  }
		break;
	      }
	    case '?':
	      /* static member function.  */
	      {
		int slen = strlen (main_fn_name);

		new_sublist->fn_field.voffset = VOFFSET_STATIC;

		/* For static member functions, we can't tell if they
		   are stubbed, as they are put out as functions, and not as
		   methods.
		   GCC v2 emits the fully mangled name if
		   dbxout.c:flag_minimal_debug is not set, so we have to
		   detect a fully mangled physname here and set is_stub
		   accordingly.  Fully mangled physnames in v2 start with
		   the member function name, followed by two underscores.
		   GCC v3 currently always emits stubbed member functions,
		   but with fully mangled physnames, which start with _Z.  */
		if (!(strncmp (new_sublist->fn_field.physname,
			       main_fn_name, slen) == 0
		      && new_sublist->fn_field.physname[slen] == '_'
		      && new_sublist->fn_field.physname[slen + 1] == '_'))
		  {
		    new_sublist->fn_field.is_stub = 1;
		  }
		break;
	      }

	    default:
	      /* error */
	      complaint (_("member function type missing, got '%c'"),
			 (*pp)[-1]);
	      /* Normal member function.  */
	      [[fallthrough]];

	    case '.':
	      /* normal member function.  */
	      new_sublist->fn_field.voffset = 0;
	      new_sublist->fn_field.fcontext = 0;
	      break;
	    }

	  new_sublist->next = sublist;
	  sublist = new_sublist;
	  length++;
	  STABS_CONTINUE (pp, objfile);
	}
      while (**pp != ';' && **pp != '\0');

      (*pp)++;
      STABS_CONTINUE (pp, objfile);

      /* Skip GCC 3.X member functions which are duplicates of the callable
	 constructor/destructor.  */
      if (strcmp_iw (main_fn_name, "__base_ctor ") == 0
	  || strcmp_iw (main_fn_name, "__base_dtor ") == 0
	  || strcmp (main_fn_name, "__deleting_dtor") == 0)
	{
	  xfree (main_fn_name);
	}
      else
	{
	  int has_destructor = 0, has_other = 0;
	  int is_v3 = 0;
	  struct next_fnfield *tmp_sublist;

	  /* Various versions of GCC emit various mostly-useless
	     strings in the name field for special member functions.

	     For stub methods, we need to defer correcting the name
	     until we are ready to unstub the method, because the current
	     name string is used by gdb_mangle_name.  The only stub methods
	     of concern here are GNU v2 operators; other methods have their
	     names correct (see caveat below).

	     For non-stub methods, in GNU v3, we have a complete physname.
	     Therefore we can safely correct the name now.  This primarily
	     affects constructors and destructors, whose name will be
	     __comp_ctor or __comp_dtor instead of Foo or ~Foo.  Cast
	     operators will also have incorrect names; for instance,
	     "operator int" will be named "operator i" (i.e. the type is
	     mangled).

	     For non-stub methods in GNU v2, we have no easy way to
	     know if we have a complete physname or not.  For most
	     methods the result depends on the platform (if CPLUS_MARKER
	     can be `$' or `.', it will use minimal debug information, or
	     otherwise the full physname will be included).

	     Rather than dealing with this, we take a different approach.
	     For v3 mangled names, we can use the full physname; for v2,
	     we use cplus_demangle_opname (which is actually v2 specific),
	     because the only interesting names are all operators - once again
	     barring the caveat below.  Skip this process if any method in the
	     group is a stub, to prevent our fouling up the workings of
	     gdb_mangle_name.

	     The caveat: GCC 2.95.x (and earlier?) put constructors and
	     destructors in the same method group.  We need to split this
	     into two groups, because they should have different names.
	     So for each method group we check whether it contains both
	     routines whose physname appears to be a destructor (the physnames
	     for and destructors are always provided, due to quirks in v2
	     mangling) and routines whose physname does not appear to be a
	     destructor.  If so then we break up the list into two halves.
	     Even if the constructors and destructors aren't in the same group
	     the destructor will still lack the leading tilde, so that also
	     needs to be fixed.

	     So, to summarize what we expect and handle here:

		Given         Given          Real         Real       Action
	     method name     physname      physname   method name

	     __opi            [none]     __opi__3Foo  operator int    opname
								 [now or later]
	     Foo              _._3Foo       _._3Foo      ~Foo      separate and
								       rename
	     operator i     _ZN3FoocviEv _ZN3FoocviEv operator int    demangle
	     __comp_ctor  _ZN3FooC1ERKS_ _ZN3FooC1ERKS_   Foo         demangle
	  */

	  tmp_sublist = sublist;
	  while (tmp_sublist != NULL)
	    {
	      if (tmp_sublist->fn_field.physname[0] == '_'
		  && tmp_sublist->fn_field.physname[1] == 'Z')
		is_v3 = 1;

	      if (is_destructor_name (tmp_sublist->fn_field.physname))
		has_destructor++;
	      else
		has_other++;

	      tmp_sublist = tmp_sublist->next;
	    }

	  if (has_destructor && has_other)
	    {
	      struct next_fnfieldlist *destr_fnlist;
	      struct next_fnfield *last_sublist;

	      /* Create a new fn_fieldlist for the destructors.  */

	      destr_fnlist = OBSTACK_ZALLOC (&fip->obstack,
					     struct next_fnfieldlist);

	      destr_fnlist->fn_fieldlist.name
		= obconcat (&objfile->objfile_obstack, "~",
			    new_fnlist->fn_fieldlist.name, (char *) NULL);

	      destr_fnlist->fn_fieldlist.fn_fields =
		XOBNEWVEC (&objfile->objfile_obstack,
			   struct fn_field, has_destructor);
	      memset (destr_fnlist->fn_fieldlist.fn_fields, 0,
		  sizeof (struct fn_field) * has_destructor);
	      tmp_sublist = sublist;
	      last_sublist = NULL;
	      i = 0;
	      while (tmp_sublist != NULL)
		{
		  if (!is_destructor_name (tmp_sublist->fn_field.physname))
		    {
		      tmp_sublist = tmp_sublist->next;
		      continue;
		    }
		  
		  destr_fnlist->fn_fieldlist.fn_fields[i++]
		    = tmp_sublist->fn_field;
		  if (last_sublist)
		    last_sublist->next = tmp_sublist->next;
		  else
		    sublist = tmp_sublist->next;
		  last_sublist = tmp_sublist;
		  tmp_sublist = tmp_sublist->next;
		}

	      destr_fnlist->fn_fieldlist.length = has_destructor;
	      destr_fnlist->next = fip->fnlist;
	      fip->fnlist = destr_fnlist;
	      nfn_fields++;
	      length -= has_destructor;
	    }
	  else if (is_v3)
	    {
	      /* v3 mangling prevents the use of abbreviated physnames,
		 so we can do this here.  There are stubbed methods in v3
		 only:
		 - in -gstabs instead of -gstabs+
		 - or for static methods, which are output as a function type
		   instead of a method type.  */
	      char *new_method_name =
		stabs_method_name_from_physname (sublist->fn_field.physname);

	      if (new_method_name != NULL
		  && strcmp (new_method_name,
			     new_fnlist->fn_fieldlist.name) != 0)
		{
		  new_fnlist->fn_fieldlist.name = new_method_name;
		  xfree (main_fn_name);
		}
	      else
		xfree (new_method_name);
	    }
	  else if (has_destructor && new_fnlist->fn_fieldlist.name[0] != '~')
	    {
	      new_fnlist->fn_fieldlist.name =
		obconcat (&objfile->objfile_obstack,
			  "~", main_fn_name, (char *)NULL);
	      xfree (main_fn_name);
	    }

	  new_fnlist->fn_fieldlist.fn_fields
	    = OBSTACK_CALLOC (&objfile->objfile_obstack, length, fn_field);
	  for (i = length; (i--, sublist); sublist = sublist->next)
	    {
	      new_fnlist->fn_fieldlist.fn_fields[i] = sublist->fn_field;
	    }

	  new_fnlist->fn_fieldlist.length = length;
	  new_fnlist->next = fip->fnlist;
	  fip->fnlist = new_fnlist;
	  nfn_fields++;
	}
    }

  if (nfn_fields)
    {
      ALLOCATE_CPLUS_STRUCT_TYPE (type);
      TYPE_FN_FIELDLISTS (type) = (struct fn_fieldlist *)
	TYPE_ZALLOC (type, sizeof (struct fn_fieldlist) * nfn_fields);
      TYPE_NFN_FIELDS (type) = nfn_fields;
    }

  return 1;
}

/* Special GNU C++ name.

   Returns 1 for success, 0 for failure.  "failure" means that we can't
   keep parsing and it's time for error_type().  */

static int
read_cpp_abbrev (struct stab_field_info *fip, const char **pp,
		 struct type *type, struct objfile *objfile)
{
  const char *p;
  const char *name;
  char cpp_abbrev;
  struct type *context;

  p = *pp;
  if (*++p == 'v')
    {
      name = NULL;
      cpp_abbrev = *++p;

      *pp = p + 1;

      /* At this point, *pp points to something like "22:23=*22...",
	 where the type number before the ':' is the "context" and
	 everything after is a regular type definition.  Lookup the
	 type, find it's name, and construct the field name.  */

      context = read_type (pp, objfile);

      switch (cpp_abbrev)
	{
	case 'f':		/* $vf -- a virtual function table pointer */
	  name = context->name ();
	  if (name == NULL)
	    {
	      name = "";
	    }
	  fip->list->field.set_name (obconcat (&objfile->objfile_obstack,
					       vptr_name, name, (char *) NULL));
	  break;

	case 'b':		/* $vb -- a virtual bsomethingorother */
	  name = context->name ();
	  if (name == NULL)
	    {
	      complaint (_("C++ abbreviated type name "
			   "unknown at symtab pos %d"),
			 symnum);
	      name = "FOO";
	    }
	  fip->list->field.set_name (obconcat (&objfile->objfile_obstack,
					       vb_name, name, (char *) NULL));
	  break;

	default:
	  invalid_cpp_abbrev_complaint (*pp);
	  fip->list->field.set_name (obconcat (&objfile->objfile_obstack,
					       "INVALID_CPLUSPLUS_ABBREV",
					       (char *) NULL));
	  break;
	}

      /* At this point, *pp points to the ':'.  Skip it and read the
	 field type.  */

      p = ++(*pp);
      if (p[-1] != ':')
	{
	  invalid_cpp_abbrev_complaint (*pp);
	  return 0;
	}
      fip->list->field.set_type (read_type (pp, objfile));
      if (**pp == ',')
	(*pp)++;		/* Skip the comma.  */
      else
	return 0;

      {
	int nbits;

	fip->list->field.set_loc_bitpos (read_huge_number (pp, ';', &nbits, 0));
	if (nbits != 0)
	  return 0;
      }
      /* This field is unpacked.  */
      fip->list->field.set_bitsize (0);
      fip->list->field.set_accessibility (accessibility::PRIVATE);
    }
  else
    {
      invalid_cpp_abbrev_complaint (*pp);
      /* We have no idea what syntax an unrecognized abbrev would have, so
	 better return 0.  If we returned 1, we would need to at least advance
	 *pp to avoid an infinite loop.  */
      return 0;
    }
  return 1;
}

static void
read_one_struct_field (struct stab_field_info *fip, const char **pp,
		       const char *p, struct type *type,
		       struct objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();

  fip->list->field.set_name
    (obstack_strndup (&objfile->objfile_obstack, *pp, p - *pp));
  *pp = p + 1;

  /* This means we have a visibility for a field coming.  */
  int visibility;
  if (**pp == '/')
    {
      (*pp)++;
      visibility = *(*pp)++;
    }
  else
    {
      /* normal dbx-style format, no explicit visibility */
      visibility = VISIBILITY_PUBLIC;
    }

  switch (visibility)
    {
    case VISIBILITY_PRIVATE:
      fip->list->field.set_accessibility (accessibility::PRIVATE);
      break;

    case VISIBILITY_PROTECTED:
      fip->list->field.set_accessibility (accessibility::PROTECTED);
      break;

    case VISIBILITY_IGNORE:
      fip->list->field.set_ignored ();
      break;

    case VISIBILITY_PUBLIC:
      break;

    default:
      /* Unknown visibility.  Complain and treat it as public.  */
      {
	complaint (_("Unknown visibility `%c' for field"),
		   visibility);
      }
      break;
    }

  fip->list->field.set_type (read_type (pp, objfile));
  if (**pp == ':')
    {
      p = ++(*pp);
#if 0
      /* Possible future hook for nested types.  */
      if (**pp == '!')
	{
	  fip->list->field.bitpos = (long) -2;	/* nested type */
	  p = ++(*pp);
	}
      else
	...;
#endif
      while (*p != ';')
	{
	  p++;
	}
      /* Static class member.  */
      fip->list->field.set_loc_physname (savestring (*pp, p - *pp));
      *pp = p + 1;
      return;
    }
  else if (**pp != ',')
    {
      /* Bad structure-type format.  */
      stabs_general_complaint ("bad structure-type format");
      return;
    }

  (*pp)++;			/* Skip the comma.  */

  {
    int nbits;

    fip->list->field.set_loc_bitpos (read_huge_number (pp, ',', &nbits, 0));
    if (nbits != 0)
      {
	stabs_general_complaint ("bad structure-type format");
	return;
      }
    fip->list->field.set_bitsize (read_huge_number (pp, ';', &nbits, 0));
    if (nbits != 0)
      {
	stabs_general_complaint ("bad structure-type format");
	return;
      }
  }

  if (fip->list->field.loc_bitpos () == 0
      && fip->list->field.bitsize () == 0)
    {
      /* This can happen in two cases: (1) at least for gcc 2.4.5 or so,
	 it is a field which has been optimized out.  The correct stab for
	 this case is to use VISIBILITY_IGNORE, but that is a recent
	 invention.  (2) It is a 0-size array.  For example
	 union { int num; char str[0]; } foo.  Printing _("<no value>" for
	 str in "p foo" is OK, since foo.str (and thus foo.str[3])
	 will continue to work, and a 0-size array as a whole doesn't
	 have any contents to print.

	 I suspect this probably could also happen with gcc -gstabs (not
	 -gstabs+) for static fields, and perhaps other C++ extensions.
	 Hopefully few people use -gstabs with gdb, since it is intended
	 for dbx compatibility.  */

      /* Ignore this field.  */
      fip->list->field.set_ignored ();
    }
  else
    {
      /* Detect an unpacked field and mark it as such.
	 dbx gives a bit size for all fields.
	 Note that forward refs cannot be packed,
	 and treat enums as if they had the width of ints.  */

      struct type *field_type = check_typedef (fip->list->field.type ());

      if (field_type->code () != TYPE_CODE_INT
	  && field_type->code () != TYPE_CODE_RANGE
	  && field_type->code () != TYPE_CODE_BOOL
	  && field_type->code () != TYPE_CODE_ENUM)
	{
	  fip->list->field.set_bitsize (0);
	}
      if ((fip->list->field.bitsize ()
	   == TARGET_CHAR_BIT * field_type->length ()
	   || (field_type->code () == TYPE_CODE_ENUM
	       && (fip->list->field.bitsize ()
		   == gdbarch_int_bit (gdbarch)))
	  )
	  &&
	  fip->list->field.loc_bitpos () % 8 == 0)
	{
	  fip->list->field.set_bitsize (0);
	}
    }
}


/* Read struct or class data fields.  They have the form:

   NAME : [VISIBILITY] TYPENUM , BITPOS , BITSIZE ;

   At the end, we see a semicolon instead of a field.

   In C++, this may wind up being NAME:?TYPENUM:PHYSNAME; for
   a static field.

   The optional VISIBILITY is one of:

   '/0' (VISIBILITY_PRIVATE)
   '/1' (VISIBILITY_PROTECTED)
   '/2' (VISIBILITY_PUBLIC)
   '/9' (VISIBILITY_IGNORE)

   or nothing, for C style fields with public visibility.

   Returns 1 for success, 0 for failure.  */

static int
read_struct_fields (struct stab_field_info *fip, const char **pp,
		    struct type *type, struct objfile *objfile)
{
  const char *p;
  struct stabs_nextfield *newobj;

  /* We better set p right now, in case there are no fields at all...    */

  p = *pp;

  /* Read each data member type until we find the terminating ';' at the end of
     the data member list, or break for some other reason such as finding the
     start of the member function list.  */
  /* Stab string for structure/union does not end with two ';' in
     SUN C compiler 5.3 i.e. F6U2, hence check for end of string.  */

  while (**pp != ';' && **pp != '\0')
    {
      STABS_CONTINUE (pp, objfile);
      /* Get space to record the next field's data.  */
      newobj = OBSTACK_ZALLOC (&fip->obstack, struct stabs_nextfield);

      newobj->next = fip->list;
      fip->list = newobj;

      /* Get the field name.  */
      p = *pp;

      /* If is starts with CPLUS_MARKER it is a special abbreviation,
	 unless the CPLUS_MARKER is followed by an underscore, in
	 which case it is just the name of an anonymous type, which we
	 should handle like any other type name.  */

      if (is_cplus_marker (p[0]) && p[1] != '_')
	{
	  if (!read_cpp_abbrev (fip, pp, type, objfile))
	    return 0;
	  continue;
	}

      /* Look for the ':' that separates the field name from the field
	 values.  Data members are delimited by a single ':', while member
	 functions are delimited by a pair of ':'s.  When we hit the member
	 functions (if any), terminate scan loop and return.  */

      while (*p != ':' && *p != '\0')
	{
	  p++;
	}
      if (*p == '\0')
	return 0;

      /* Check to see if we have hit the member functions yet.  */
      if (p[1] == ':')
	{
	  break;
	}
      read_one_struct_field (fip, pp, p, type, objfile);
    }
  if (p[0] == ':' && p[1] == ':')
    {
      /* (the deleted) chill the list of fields: the last entry (at
	 the head) is a partially constructed entry which we now
	 scrub.  */
      fip->list = fip->list->next;
    }
  return 1;
}
/* The stabs for C++ derived classes contain baseclass information which
   is marked by a '!' character after the total size.  This function is
   called when we encounter the baseclass marker, and slurps up all the
   baseclass information.

   Immediately following the '!' marker is the number of base classes that
   the class is derived from, followed by information for each base class.
   For each base class, there are two visibility specifiers, a bit offset
   to the base class information within the derived class, a reference to
   the type for the base class, and a terminating semicolon.

   A typical example, with two base classes, would be "!2,020,19;0264,21;".
						       ^^ ^ ^ ^  ^ ^  ^
	Baseclass information marker __________________|| | | |  | |  |
	Number of baseclasses __________________________| | | |  | |  |
	Visibility specifiers (2) ________________________| | |  | |  |
	Offset in bits from start of class _________________| |  | |  |
	Type number for base class ___________________________|  | |  |
	Visibility specifiers (2) _______________________________| |  |
	Offset in bits from start of class ________________________|  |
	Type number of base class ____________________________________|

  Return 1 for success, 0 for (error-type-inducing) failure.  */



static int
read_baseclasses (struct stab_field_info *fip, const char **pp,
		  struct type *type, struct objfile *objfile)
{
  int i;
  struct stabs_nextfield *newobj;

  if (**pp != '!')
    {
      return 1;
    }
  else
    {
      /* Skip the '!' baseclass information marker.  */
      (*pp)++;
    }

  ALLOCATE_CPLUS_STRUCT_TYPE (type);
  {
    int nbits;

    TYPE_N_BASECLASSES (type) = read_huge_number (pp, ',', &nbits, 0);
    if (nbits != 0)
      return 0;
  }

  for (i = 0; i < TYPE_N_BASECLASSES (type); i++)
    {
      newobj = OBSTACK_ZALLOC (&fip->obstack, struct stabs_nextfield);

      newobj->next = fip->list;
      fip->list = newobj;
      newobj->field.set_bitsize (0);	/* This should be an unpacked
					   field!  */

      STABS_CONTINUE (pp, objfile);
      switch (**pp)
	{
	case '0':
	  /* Nothing to do.  */
	  break;
	case '1':
	  newobj->field.set_virtual ();
	  break;
	default:
	  /* Unknown character.  Complain and treat it as non-virtual.  */
	  {
	    complaint (_("Unknown virtual character `%c' for baseclass"),
		       **pp);
	  }
	}
      ++(*pp);

      int visibility = *(*pp)++;
      switch (visibility)
	{
	case VISIBILITY_PRIVATE:
	  newobj->field.set_accessibility (accessibility::PRIVATE);
	  break;
	case VISIBILITY_PROTECTED:
	  newobj->field.set_accessibility (accessibility::PROTECTED);
	  break;
	case VISIBILITY_PUBLIC:
	  break;
	default:
	  /* Bad visibility format.  Complain and treat it as
	     public.  */
	  {
	    complaint (_("Unknown visibility `%c' for baseclass"),
		       visibility);
	  }
	}

      {
	int nbits;

	/* The remaining value is the bit offset of the portion of the object
	   corresponding to this baseclass.  Always zero in the absence of
	   multiple inheritance.  */

	newobj->field.set_loc_bitpos (read_huge_number (pp, ',', &nbits, 0));
	if (nbits != 0)
	  return 0;
      }

      /* The last piece of baseclass information is the type of the
	 base class.  Read it, and remember it's type name as this
	 field's name.  */

      newobj->field.set_type (read_type (pp, objfile));
      newobj->field.set_name (newobj->field.type ()->name ());

      /* Skip trailing ';' and bump count of number of fields seen.  */
      if (**pp == ';')
	(*pp)++;
      else
	return 0;
    }
  return 1;
}

/* The tail end of stabs for C++ classes that contain a virtual function
   pointer contains a tilde, a %, and a type number.
   The type number refers to the base class (possibly this class itself) which
   contains the vtable pointer for the current class.

   This function is called when we have parsed all the method declarations,
   so we can look for the vptr base class info.  */

static int
read_tilde_fields (struct stab_field_info *fip, const char **pp,
		   struct type *type, struct objfile *objfile)
{
  const char *p;

  STABS_CONTINUE (pp, objfile);

  /* If we are positioned at a ';', then skip it.  */
  if (**pp == ';')
    {
      (*pp)++;
    }

  if (**pp == '~')
    {
      (*pp)++;

      if (**pp == '=' || **pp == '+' || **pp == '-')
	{
	  /* Obsolete flags that used to indicate the presence
	     of constructors and/or destructors.  */
	  (*pp)++;
	}

      /* Read either a '%' or the final ';'.  */
      if (*(*pp)++ == '%')
	{
	  /* The next number is the type number of the base class
	     (possibly our own class) which supplies the vtable for
	     this class.  Parse it out, and search that class to find
	     its vtable pointer, and install those into TYPE_VPTR_BASETYPE
	     and TYPE_VPTR_FIELDNO.  */

	  struct type *t;
	  int i;

	  t = read_type (pp, objfile);
	  p = (*pp)++;
	  while (*p != '\0' && *p != ';')
	    {
	      p++;
	    }
	  if (*p == '\0')
	    {
	      /* Premature end of symbol.  */
	      return 0;
	    }

	  set_type_vptr_basetype (type, t);
	  if (type == t)	/* Our own class provides vtbl ptr.  */
	    {
	      for (i = t->num_fields () - 1;
		   i >= TYPE_N_BASECLASSES (t);
		   --i)
		{
		  const char *name = t->field (i).name ();

		  if (!strncmp (name, vptr_name, sizeof (vptr_name) - 2)
		      && is_cplus_marker (name[sizeof (vptr_name) - 2]))
		    {
		      set_type_vptr_fieldno (type, i);
		      goto gotit;
		    }
		}
	      /* Virtual function table field not found.  */
	      complaint (_("virtual function table pointer "
			   "not found when defining class `%s'"),
			 type->name ());
	      return 0;
	    }
	  else
	    {
	      set_type_vptr_fieldno (type, TYPE_VPTR_FIELDNO (t));
	    }

	gotit:
	  *pp = p + 1;
	}
    }
  return 1;
}

static int
attach_fn_fields_to_type (struct stab_field_info *fip, struct type *type)
{
  int n;

  for (n = TYPE_NFN_FIELDS (type);
       fip->fnlist != NULL;
       fip->fnlist = fip->fnlist->next)
    {
      --n;			/* Circumvent Sun3 compiler bug.  */
      TYPE_FN_FIELDLISTS (type)[n] = fip->fnlist->fn_fieldlist;
    }
  return 1;
}

/* Create the vector of fields, and record how big it is.
   We need this info to record proper virtual function table information
   for this class's virtual functions.  */

static int
attach_fields_to_type (struct stab_field_info *fip, struct type *type,
		       struct objfile *objfile)
{
  int nfields = 0;
  struct stabs_nextfield *scan;

  /* Count up the number of fields that we have.  */

  for (scan = fip->list; scan != NULL; scan = scan->next)
    nfields++;

  /* Now we know how many fields there are, and whether or not there are any
     non-public fields.  Record the field count, allocate space for the
     array of fields.  */

  type->alloc_fields (nfields);

  /* Copy the saved-up fields into the field vector.  Start from the
     head of the list, adding to the tail of the field array, so that
     they end up in the same order in the array in which they were
     added to the list.  */

  while (nfields-- > 0)
    {
      type->field (nfields) = fip->list->field;
      fip->list = fip->list->next;
    }
  return 1;
}


/* Complain that the compiler has emitted more than one definition for the
   structure type TYPE.  */
static void 
complain_about_struct_wipeout (struct type *type)
{
  const char *name = "";
  const char *kind = "";

  if (type->name ())
    {
      name = type->name ();
      switch (type->code ())
	{
	case TYPE_CODE_STRUCT: kind = "struct "; break;
	case TYPE_CODE_UNION:  kind = "union ";  break;
	case TYPE_CODE_ENUM:   kind = "enum ";   break;
	default: kind = "";
	}
    }
  else
    {
      name = "<unknown>";
      kind = "";
    }

  complaint (_("struct/union type gets multiply defined: %s%s"), kind, name);
}

/* Set the length for all variants of a same main_type, which are
   connected in the closed chain.
   
   This is something that needs to be done when a type is defined *after*
   some cross references to this type have already been read.  Consider
   for instance the following scenario where we have the following two
   stabs entries:

	.stabs  "t:p(0,21)=*(0,22)=k(0,23)=xsdummy:",160,0,28,-24
	.stabs  "dummy:T(0,23)=s16x:(0,1),0,3[...]"

   A stubbed version of type dummy is created while processing the first
   stabs entry.  The length of that type is initially set to zero, since
   it is unknown at this point.  Also, a "constant" variation of type
   "dummy" is created as well (this is the "(0,22)=k(0,23)" section of
   the stabs line).

   The second stabs entry allows us to replace the stubbed definition
   with the real definition.  However, we still need to adjust the length
   of the "constant" variation of that type, as its length was left
   untouched during the main type replacement...  */

static void
set_length_in_type_chain (struct type *type)
{
  struct type *ntype = TYPE_CHAIN (type);

  while (ntype != type)
    {
      if (ntype->length () == 0)
	ntype->set_length (type->length ());
      else
	complain_about_struct_wipeout (ntype);
      ntype = TYPE_CHAIN (ntype);
    }
}

/* Read the description of a structure (or union type) and return an object
   describing the type.

   PP points to a character pointer that points to the next unconsumed token
   in the stabs string.  For example, given stabs "A:T4=s4a:1,0,32;;",
   *PP will point to "4a:1,0,32;;".

   TYPE points to an incomplete type that needs to be filled in.

   OBJFILE points to the current objfile from which the stabs information is
   being read.  (Note that it is redundant in that TYPE also contains a pointer
   to this same objfile, so it might be a good idea to eliminate it.  FIXME). 
 */

static struct type *
read_struct_type (const char **pp, struct type *type, enum type_code type_code,
		  struct objfile *objfile)
{
  struct stab_field_info fi;

  /* When describing struct/union/class types in stabs, G++ always drops
     all qualifications from the name.  So if you've got:
       struct A { ... struct B { ... }; ... };
     then G++ will emit stabs for `struct A::B' that call it simply
     `struct B'.  Obviously, if you've got a real top-level definition for
     `struct B', or other nested definitions, this is going to cause
     problems.

     Obviously, GDB can't fix this by itself, but it can at least avoid
     scribbling on existing structure type objects when new definitions
     appear.  */
  if (! (type->code () == TYPE_CODE_UNDEF
	 || type->is_stub ()))
    {
      complain_about_struct_wipeout (type);

      /* It's probably best to return the type unchanged.  */
      return type;
    }

  INIT_CPLUS_SPECIFIC (type);
  type->set_code (type_code);
  type->set_is_stub (false);

  /* First comes the total size in bytes.  */

  {
    int nbits;

    type->set_length (read_huge_number (pp, 0, &nbits, 0));
    if (nbits != 0)
      return error_type (pp, objfile);
    set_length_in_type_chain (type);
  }

  /* Now read the baseclasses, if any, read the regular C struct or C++
     class member fields, attach the fields to the type, read the C++
     member functions, attach them to the type, and then read any tilde
     field (baseclass specifier for the class holding the main vtable).  */

  if (!read_baseclasses (&fi, pp, type, objfile)
      || !read_struct_fields (&fi, pp, type, objfile)
      || !attach_fields_to_type (&fi, type, objfile)
      || !read_member_functions (&fi, pp, type, objfile)
      || !attach_fn_fields_to_type (&fi, type)
      || !read_tilde_fields (&fi, pp, type, objfile))
    {
      type = error_type (pp, objfile);
    }

  return (type);
}

/* Read a definition of an array type,
   and create and return a suitable type object.
   Also creates a range type which represents the bounds of that
   array.  */

static struct type *
read_array_type (const char **pp, struct type *type,
		 struct objfile *objfile)
{
  struct type *index_type, *element_type, *range_type;
  int lower, upper;
  int adjustable = 0;
  int nbits;

  /* Format of an array type:
     "ar<index type>;lower;upper;<array_contents_type>".
     OS9000: "arlower,upper;<array_contents_type>".

     Fortran adjustable arrays use Adigits or Tdigits for lower or upper;
     for these, produce a type like float[][].  */

    {
      index_type = read_type (pp, objfile);
      if (**pp != ';')
	/* Improper format of array type decl.  */
	return error_type (pp, objfile);
      ++*pp;
    }

  if (!(**pp >= '0' && **pp <= '9') && **pp != '-')
    {
      (*pp)++;
      adjustable = 1;
    }
  lower = read_huge_number (pp, ';', &nbits, 0);

  if (nbits != 0)
    return error_type (pp, objfile);

  if (!(**pp >= '0' && **pp <= '9') && **pp != '-')
    {
      (*pp)++;
      adjustable = 1;
    }
  upper = read_huge_number (pp, ';', &nbits, 0);
  if (nbits != 0)
    return error_type (pp, objfile);

  element_type = read_type (pp, objfile);

  if (adjustable)
    {
      lower = 0;
      upper = -1;
    }

  type_allocator alloc (objfile, get_current_subfile ()->language);
  range_type =
    create_static_range_type (alloc, index_type, lower, upper);
  type_allocator smash_alloc (type, type_allocator::SMASH);
  type = create_array_type (smash_alloc, element_type, range_type);

  return type;
}


/* Read a definition of an enumeration type,
   and create and return a suitable type object.
   Also defines the symbols that represent the values of the type.  */

static struct type *
read_enum_type (const char **pp, struct type *type,
		struct objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  const char *p;
  char *name;
  long n;
  struct symbol *sym;
  int nsyms = 0;
  struct pending **symlist;
  struct pending *osyms, *syms;
  int o_nsyms;
  int nbits;
  int unsigned_enum = 1;

#if 0
  /* FIXME!  The stabs produced by Sun CC merrily define things that ought
     to be file-scope, between N_FN entries, using N_LSYM.  What's a mother
     to do?  For now, force all enum values to file scope.  */
  if (within_function)
    symlist = get_local_symbols ();
  else
#endif
    symlist = get_file_symbols ();
  osyms = *symlist;
  o_nsyms = osyms ? osyms->nsyms : 0;

  /* The aix4 compiler emits an extra field before the enum members;
     my guess is it's a type of some sort.  Just ignore it.  */
  if (**pp == '-')
    {
      /* Skip over the type.  */
      while (**pp != ':')
	(*pp)++;

      /* Skip over the colon.  */
      (*pp)++;
    }

  /* Read the value-names and their values.
     The input syntax is NAME:VALUE,NAME:VALUE, and so on.
     A semicolon or comma instead of a NAME means the end.  */
  while (**pp && **pp != ';' && **pp != ',')
    {
      STABS_CONTINUE (pp, objfile);
      p = *pp;
      while (*p != ':')
	p++;
      name = obstack_strndup (&objfile->objfile_obstack, *pp, p - *pp);
      *pp = p + 1;
      n = read_huge_number (pp, ',', &nbits, 0);
      if (nbits != 0)
	return error_type (pp, objfile);

      sym = new (&objfile->objfile_obstack) symbol;
      sym->set_linkage_name (name);
      sym->set_language (get_current_subfile ()->language,
			 &objfile->objfile_obstack);
      sym->set_aclass_index (LOC_CONST);
      sym->set_domain (VAR_DOMAIN);
      sym->set_value_longest (n);
      if (n < 0)
	unsigned_enum = 0;
      add_symbol_to_list (sym, symlist);
      nsyms++;
    }

  if (**pp == ';')
    (*pp)++;			/* Skip the semicolon.  */

  /* Now fill in the fields of the type-structure.  */

  type->set_length (gdbarch_int_bit (gdbarch) / HOST_CHAR_BIT);
  set_length_in_type_chain (type);
  type->set_code (TYPE_CODE_ENUM);
  type->set_is_stub (false);
  if (unsigned_enum)
    type->set_is_unsigned (true);
  type->alloc_fields (nsyms);

  /* Find the symbols for the values and put them into the type.
     The symbols can be found in the symlist that we put them on
     to cause them to be defined.  osyms contains the old value
     of that symlist; everything up to there was defined by us.  */
  /* Note that we preserve the order of the enum constants, so
     that in something like "enum {FOO, LAST_THING=FOO}" we print
     FOO, not LAST_THING.  */

  for (syms = *symlist, n = nsyms - 1; syms; syms = syms->next)
    {
      int last = syms == osyms ? o_nsyms : 0;
      int j = syms->nsyms;

      for (; --j >= last; --n)
	{
	  struct symbol *xsym = syms->symbol[j];

	  xsym->set_type (type);
	  type->field (n).set_name (xsym->linkage_name ());
	  type->field (n).set_loc_enumval (xsym->value_longest ());
	  type->field (n).set_bitsize (0);
	}
      if (syms == osyms)
	break;
    }

  return type;
}

/* Sun's ACC uses a somewhat saner method for specifying the builtin
   typedefs in every file (for int, long, etc):

   type = b <signed> <width> <format type>; <offset>; <nbits>
   signed = u or s.
   optional format type = c or b for char or boolean.
   offset = offset from high order bit to start bit of type.
   width is # bytes in object of this type, nbits is # bits in type.

   The width/offset stuff appears to be for small objects stored in
   larger ones (e.g. `shorts' in `int' registers).  We ignore it for now,
   FIXME.  */

static struct type *
read_sun_builtin_type (const char **pp, int typenums[2], struct objfile *objfile)
{
  int type_bits;
  int nbits;
  int unsigned_type;
  int boolean_type = 0;

  switch (**pp)
    {
    case 's':
      unsigned_type = 0;
      break;
    case 'u':
      unsigned_type = 1;
      break;
    default:
      return error_type (pp, objfile);
    }
  (*pp)++;

  /* For some odd reason, all forms of char put a c here.  This is strange
     because no other type has this honor.  We can safely ignore this because
     we actually determine 'char'acterness by the number of bits specified in
     the descriptor.
     Boolean forms, e.g Fortran logical*X, put a b here.  */

  if (**pp == 'c')
    (*pp)++;
  else if (**pp == 'b')
    {
      boolean_type = 1;
      (*pp)++;
    }

  /* The first number appears to be the number of bytes occupied
     by this type, except that unsigned short is 4 instead of 2.
     Since this information is redundant with the third number,
     we will ignore it.  */
  read_huge_number (pp, ';', &nbits, 0);
  if (nbits != 0)
    return error_type (pp, objfile);

  /* The second number is always 0, so ignore it too.  */
  read_huge_number (pp, ';', &nbits, 0);
  if (nbits != 0)
    return error_type (pp, objfile);

  /* The third number is the number of bits for this type.  */
  type_bits = read_huge_number (pp, 0, &nbits, 0);
  if (nbits != 0)
    return error_type (pp, objfile);
  /* The type *should* end with a semicolon.  If it are embedded
     in a larger type the semicolon may be the only way to know where
     the type ends.  If this type is at the end of the stabstring we
     can deal with the omitted semicolon (but we don't have to like
     it).  Don't bother to complain(), Sun's compiler omits the semicolon
     for "void".  */
  if (**pp == ';')
    ++(*pp);

  type_allocator alloc (objfile, get_current_subfile ()->language);
  if (type_bits == 0)
    {
      struct type *type = alloc.new_type (TYPE_CODE_VOID,
					  TARGET_CHAR_BIT, nullptr);
      if (unsigned_type)
	type->set_is_unsigned (true);

      return type;
    }

  if (boolean_type)
    return init_boolean_type (alloc, type_bits, unsigned_type, NULL);
  else
    return init_integer_type (alloc, type_bits, unsigned_type, NULL);
}

static struct type *
read_sun_floating_type (const char **pp, int typenums[2],
			struct objfile *objfile)
{
  int nbits;
  int details;
  int nbytes;
  struct type *rettype;

  /* The first number has more details about the type, for example
     FN_COMPLEX.  */
  details = read_huge_number (pp, ';', &nbits, 0);
  if (nbits != 0)
    return error_type (pp, objfile);

  /* The second number is the number of bytes occupied by this type.  */
  nbytes = read_huge_number (pp, ';', &nbits, 0);
  if (nbits != 0)
    return error_type (pp, objfile);

  nbits = nbytes * TARGET_CHAR_BIT;

  if (details == NF_COMPLEX || details == NF_COMPLEX16
      || details == NF_COMPLEX32)
    {
      rettype = dbx_init_float_type (objfile, nbits / 2);
      return init_complex_type (NULL, rettype);
    }

  return dbx_init_float_type (objfile, nbits);
}

/* Read a number from the string pointed to by *PP.
   The value of *PP is advanced over the number.
   If END is nonzero, the character that ends the
   number must match END, or an error happens;
   and that character is skipped if it does match.
   If END is zero, *PP is left pointing to that character.

   If TWOS_COMPLEMENT_BITS is set to a strictly positive value and if
   the number is represented in an octal representation, assume that
   it is represented in a 2's complement representation with a size of
   TWOS_COMPLEMENT_BITS.

   If the number fits in a long, set *BITS to 0 and return the value.
   If not, set *BITS to be the number of bits in the number and return 0.

   If encounter garbage, set *BITS to -1 and return 0.  */

static long
read_huge_number (const char **pp, int end, int *bits,
		  int twos_complement_bits)
{
  const char *p = *pp;
  int sign = 1;
  int sign_bit = 0;
  long n = 0;
  int radix = 10;
  char overflow = 0;
  int nbits = 0;
  int c;
  long upper_limit;
  int twos_complement_representation = 0;

  if (*p == '-')
    {
      sign = -1;
      p++;
    }

  /* Leading zero means octal.  GCC uses this to output values larger
     than an int (because that would be hard in decimal).  */
  if (*p == '0')
    {
      radix = 8;
      p++;
    }

  /* Skip extra zeros.  */
  while (*p == '0')
    p++;

  if (sign > 0 && radix == 8 && twos_complement_bits > 0)
    {
      /* Octal, possibly signed.  Check if we have enough chars for a
	 negative number.  */

      size_t len;
      const char *p1 = p;

      while ((c = *p1) >= '0' && c < '8')
	p1++;

      len = p1 - p;
      if (len > twos_complement_bits / 3
	  || (twos_complement_bits % 3 == 0
	      && len == twos_complement_bits / 3))
	{
	  /* Ok, we have enough characters for a signed value, check
	     for signedness by testing if the sign bit is set.  */
	  sign_bit = (twos_complement_bits % 3 + 2) % 3;
	  c = *p - '0';
	  if (c & (1 << sign_bit))
	    {
	      /* Definitely signed.  */
	      twos_complement_representation = 1;
	      sign = -1;
	    }
	}
    }

  upper_limit = LONG_MAX / radix;

  while ((c = *p++) >= '0' && c < ('0' + radix))
    {
      if (n <= upper_limit)
	{
	  if (twos_complement_representation)
	    {
	      /* Octal, signed, twos complement representation.  In
		 this case, n is the corresponding absolute value.  */
	      if (n == 0)
		{
		  long sn = c - '0' - ((2 * (c - '0')) | (2 << sign_bit));

		  n = -sn;
		}
	      else
		{
		  n *= radix;
		  n -= c - '0';
		}
	    }
	  else
	    {
	      /* unsigned representation */
	      n *= radix;
	      n += c - '0';		/* FIXME this overflows anyway.  */
	    }
	}
      else
	overflow = 1;

      /* This depends on large values being output in octal, which is
	 what GCC does.  */
      if (radix == 8)
	{
	  if (nbits == 0)
	    {
	      if (c == '0')
		/* Ignore leading zeroes.  */
		;
	      else if (c == '1')
		nbits = 1;
	      else if (c == '2' || c == '3')
		nbits = 2;
	      else
		nbits = 3;
	    }
	  else
	    nbits += 3;
	}
    }
  if (end)
    {
      if (c && c != end)
	{
	  if (bits != NULL)
	    *bits = -1;
	  return 0;
	}
    }
  else
    --p;

  if (radix == 8 && twos_complement_bits > 0 && nbits > twos_complement_bits)
    {
      /* We were supposed to parse a number with maximum
	 TWOS_COMPLEMENT_BITS bits, but something went wrong.  */
      if (bits != NULL)
	*bits = -1;
      return 0;
    }

  *pp = p;
  if (overflow)
    {
      if (nbits == 0)
	{
	  /* Large decimal constants are an error (because it is hard to
	     count how many bits are in them).  */
	  if (bits != NULL)
	    *bits = -1;
	  return 0;
	}

      /* -0x7f is the same as 0x80.  So deal with it by adding one to
	 the number of bits.  Two's complement represention octals
	 can't have a '-' in front.  */
      if (sign == -1 && !twos_complement_representation)
	++nbits;
      if (bits)
	*bits = nbits;
    }
  else
    {
      if (bits)
	*bits = 0;
      return n * sign;
    }
  /* It's *BITS which has the interesting information.  */
  return 0;
}

static struct type *
read_range_type (const char **pp, int typenums[2], int type_size,
		 struct objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  const char *orig_pp = *pp;
  int rangenums[2];
  long n2, n3;
  int n2bits, n3bits;
  int self_subrange;
  struct type *result_type;
  struct type *index_type = NULL;

  /* First comes a type we are a subrange of.
     In C it is usually 0, 1 or the type being defined.  */
  if (read_type_number (pp, rangenums) != 0)
    return error_type (pp, objfile);
  self_subrange = (rangenums[0] == typenums[0] &&
		   rangenums[1] == typenums[1]);

  if (**pp == '=')
    {
      *pp = orig_pp;
      index_type = read_type (pp, objfile);
    }

  /* A semicolon should now follow; skip it.  */
  if (**pp == ';')
    (*pp)++;

  /* The remaining two operands are usually lower and upper bounds
     of the range.  But in some special cases they mean something else.  */
  n2 = read_huge_number (pp, ';', &n2bits, type_size);
  n3 = read_huge_number (pp, ';', &n3bits, type_size);

  if (n2bits == -1 || n3bits == -1)
    return error_type (pp, objfile);

  type_allocator alloc (objfile, get_current_subfile ()->language);

  if (index_type)
    goto handle_true_range;

  /* If limits are huge, must be large integral type.  */
  if (n2bits != 0 || n3bits != 0)
    {
      char got_signed = 0;
      char got_unsigned = 0;
      /* Number of bits in the type.  */
      int nbits = 0;

      /* If a type size attribute has been specified, the bounds of
	 the range should fit in this size.  If the lower bounds needs
	 more bits than the upper bound, then the type is signed.  */
      if (n2bits <= type_size && n3bits <= type_size)
	{
	  if (n2bits == type_size && n2bits > n3bits)
	    got_signed = 1;
	  else
	    got_unsigned = 1;
	  nbits = type_size;
	}
      /* Range from 0 to <large number> is an unsigned large integral type.  */
      else if ((n2bits == 0 && n2 == 0) && n3bits != 0)
	{
	  got_unsigned = 1;
	  nbits = n3bits;
	}
      /* Range from <large number> to <large number>-1 is a large signed
	 integral type.  Take care of the case where <large number> doesn't
	 fit in a long but <large number>-1 does.  */
      else if ((n2bits != 0 && n3bits != 0 && n2bits == n3bits + 1)
	       || (n2bits != 0 && n3bits == 0
		   && (n2bits == sizeof (long) * HOST_CHAR_BIT)
		   && n3 == LONG_MAX))
	{
	  got_signed = 1;
	  nbits = n2bits;
	}

      if (got_signed || got_unsigned)
	return init_integer_type (alloc, nbits, got_unsigned, NULL);
      else
	return error_type (pp, objfile);
    }

  /* A type defined as a subrange of itself, with bounds both 0, is void.  */
  if (self_subrange && n2 == 0 && n3 == 0)
    return alloc.new_type (TYPE_CODE_VOID, TARGET_CHAR_BIT, nullptr);

  /* If n3 is zero and n2 is positive, we want a floating type, and n2
     is the width in bytes.

     Fortran programs appear to use this for complex types also.  To
     distinguish between floats and complex, g77 (and others?)  seem
     to use self-subranges for the complexes, and subranges of int for
     the floats.

     Also note that for complexes, g77 sets n2 to the size of one of
     the member floats, not the whole complex beast.  My guess is that
     this was to work well with pre-COMPLEX versions of gdb.  */

  if (n3 == 0 && n2 > 0)
    {
      struct type *float_type
	= dbx_init_float_type (objfile, n2 * TARGET_CHAR_BIT);

      if (self_subrange)
	return init_complex_type (NULL, float_type);
      else
	return float_type;
    }

  /* If the upper bound is -1, it must really be an unsigned integral.  */

  else if (n2 == 0 && n3 == -1)
    {
      int bits = type_size;

      if (bits <= 0)
	{
	  /* We don't know its size.  It is unsigned int or unsigned
	     long.  GCC 2.3.3 uses this for long long too, but that is
	     just a GDB 3.5 compatibility hack.  */
	  bits = gdbarch_int_bit (gdbarch);
	}

      return init_integer_type (alloc, bits, 1, NULL);
    }

  /* Special case: char is defined (Who knows why) as a subrange of
     itself with range 0-127.  */
  else if (self_subrange && n2 == 0 && n3 == 127)
    {
      struct type *type = init_integer_type (alloc, TARGET_CHAR_BIT,
					     0, NULL);
      type->set_has_no_signedness (true);
      return type;
    }
  /* We used to do this only for subrange of self or subrange of int.  */
  else if (n2 == 0)
    {
      /* -1 is used for the upper bound of (4 byte) "unsigned int" and
	 "unsigned long", and we already checked for that,
	 so don't need to test for it here.  */

      if (n3 < 0)
	/* n3 actually gives the size.  */
	return init_integer_type (alloc, -n3 * TARGET_CHAR_BIT, 1, NULL);

      /* Is n3 == 2**(8n)-1 for some integer n?  Then it's an
	 unsigned n-byte integer.  But do require n to be a power of
	 two; we don't want 3- and 5-byte integers flying around.  */
      {
	int bytes;
	unsigned long bits;

	bits = n3;
	for (bytes = 0; (bits & 0xff) == 0xff; bytes++)
	  bits >>= 8;
	if (bits == 0
	    && ((bytes - 1) & bytes) == 0) /* "bytes is a power of two" */
	  return init_integer_type (alloc, bytes * TARGET_CHAR_BIT, 1, NULL);
      }
    }
  /* I think this is for Convex "long long".  Since I don't know whether
     Convex sets self_subrange, I also accept that particular size regardless
     of self_subrange.  */
  else if (n3 == 0 && n2 < 0
	   && (self_subrange
	       || n2 == -gdbarch_long_long_bit
			  (gdbarch) / TARGET_CHAR_BIT))
    return init_integer_type (alloc, -n2 * TARGET_CHAR_BIT, 0, NULL);
  else if (n2 == -n3 - 1)
    {
      if (n3 == 0x7f)
	return init_integer_type (alloc, 8, 0, NULL);
      if (n3 == 0x7fff)
	return init_integer_type (alloc, 16, 0, NULL);
      if (n3 == 0x7fffffff)
	return init_integer_type (alloc, 32, 0, NULL);
    }

  /* We have a real range type on our hands.  Allocate space and
     return a real pointer.  */
handle_true_range:

  if (self_subrange)
    index_type = builtin_type (objfile)->builtin_int;
  else
    index_type = *dbx_lookup_type (rangenums, objfile);
  if (index_type == NULL)
    {
      /* Does this actually ever happen?  Is that why we are worrying
	 about dealing with it rather than just calling error_type?  */

      complaint (_("base type %d of range type is not defined"), rangenums[1]);

      index_type = builtin_type (objfile)->builtin_int;
    }

  result_type
    = create_static_range_type (alloc, index_type, n2, n3);
  return (result_type);
}

/* Read in an argument list.  This is a list of types, separated by commas
   and terminated with END.  Return the list of types read in, or NULL
   if there is an error.  */

static struct field *
read_args (const char **pp, int end, struct objfile *objfile, int *nargsp,
	   int *varargsp)
{
  /* FIXME!  Remove this arbitrary limit!  */
  struct type *types[1024];	/* Allow for fns of 1023 parameters.  */
  int n = 0, i;
  struct field *rval;

  while (**pp != end)
    {
      if (**pp != ',')
	/* Invalid argument list: no ','.  */
	return NULL;
      (*pp)++;
      STABS_CONTINUE (pp, objfile);
      types[n++] = read_type (pp, objfile);
    }
  (*pp)++;			/* get past `end' (the ':' character).  */

  if (n == 0)
    {
      /* We should read at least the THIS parameter here.  Some broken stabs
	 output contained `(0,41),(0,42)=@s8;-16;,(0,43),(0,1);' where should
	 have been present ";-16,(0,43)" reference instead.  This way the
	 excessive ";" marker prematurely stops the parameters parsing.  */

      complaint (_("Invalid (empty) method arguments"));
      *varargsp = 0;
    }
  else if (types[n - 1]->code () != TYPE_CODE_VOID)
    *varargsp = 1;
  else
    {
      n--;
      *varargsp = 0;
    }

  rval = XCNEWVEC (struct field, n);
  for (i = 0; i < n; i++)
    rval[i].set_type (types[i]);
  *nargsp = n;
  return rval;
}

/* Common block handling.  */

/* List of symbols declared since the last BCOMM.  This list is a tail
   of local_symbols.  When ECOMM is seen, the symbols on the list
   are noted so their proper addresses can be filled in later,
   using the common block base address gotten from the assembler
   stabs.  */

static struct pending *common_block;
static int common_block_i;

/* Name of the current common block.  We get it from the BCOMM instead of the
   ECOMM to match IBM documentation (even though IBM puts the name both places
   like everyone else).  */
static char *common_block_name;

/* Process a N_BCOMM symbol.  The storage for NAME is not guaranteed
   to remain after this function returns.  */

void
common_block_start (const char *name, struct objfile *objfile)
{
  if (common_block_name != NULL)
    {
      complaint (_("Invalid symbol data: common block within common block"));
    }
  common_block = *get_local_symbols ();
  common_block_i = common_block ? common_block->nsyms : 0;
  common_block_name = obstack_strdup (&objfile->objfile_obstack, name);
}

/* Process a N_ECOMM symbol.  */

void
common_block_end (struct objfile *objfile)
{
  /* Symbols declared since the BCOMM are to have the common block
     start address added in when we know it.  common_block and
     common_block_i point to the first symbol after the BCOMM in
     the local_symbols list; copy the list and hang it off the
     symbol for the common block name for later fixup.  */
  int i;
  struct symbol *sym;
  struct pending *newobj = 0;
  struct pending *next;
  int j;

  if (common_block_name == NULL)
    {
      complaint (_("ECOMM symbol unmatched by BCOMM"));
      return;
    }

  sym = new (&objfile->objfile_obstack) symbol;
  /* Note: common_block_name already saved on objfile_obstack.  */
  sym->set_linkage_name (common_block_name);
  sym->set_aclass_index (LOC_BLOCK);

  /* Now we copy all the symbols which have been defined since the BCOMM.  */

  /* Copy all the struct pendings before common_block.  */
  for (next = *get_local_symbols ();
       next != NULL && next != common_block;
       next = next->next)
    {
      for (j = 0; j < next->nsyms; j++)
	add_symbol_to_list (next->symbol[j], &newobj);
    }

  /* Copy however much of COMMON_BLOCK we need.  If COMMON_BLOCK is
     NULL, it means copy all the local symbols (which we already did
     above).  */

  if (common_block != NULL)
    for (j = common_block_i; j < common_block->nsyms; j++)
      add_symbol_to_list (common_block->symbol[j], &newobj);

  sym->set_type ((struct type *) newobj);

  /* Should we be putting local_symbols back to what it was?
     Does it matter?  */

  i = hashname (sym->linkage_name ());
  sym->set_value_chain (global_sym_chain[i]);
  global_sym_chain[i] = sym;
  common_block_name = NULL;
}

/* Add a common block's start address to the offset of each symbol
   declared to be in it (by being between a BCOMM/ECOMM pair that uses
   the common block name).  */

static void
fix_common_block (struct symbol *sym, CORE_ADDR valu, int section_index)
{
  struct pending *next = (struct pending *) sym->type ();

  for (; next; next = next->next)
    {
      int j;

      for (j = next->nsyms - 1; j >= 0; j--)
	{
	  next->symbol[j]->set_value_address
	    (next->symbol[j]->value_address () + valu);
	  next->symbol[j]->set_section_index (section_index);
	}
    }
}



/* Add {TYPE, TYPENUMS} to the NONAME_UNDEFS vector.
   See add_undefined_type for more details.  */

static void
add_undefined_type_noname (struct type *type, int typenums[2])
{
  struct nat nat;

  nat.typenums[0] = typenums [0];
  nat.typenums[1] = typenums [1];
  nat.type = type;

  if (noname_undefs_length == noname_undefs_allocated)
    {
      noname_undefs_allocated *= 2;
      noname_undefs = (struct nat *)
	xrealloc ((char *) noname_undefs,
		  noname_undefs_allocated * sizeof (struct nat));
    }
  noname_undefs[noname_undefs_length++] = nat;
}

/* Add TYPE to the UNDEF_TYPES vector.
   See add_undefined_type for more details.  */

static void
add_undefined_type_1 (struct type *type)
{
  if (undef_types_length == undef_types_allocated)
    {
      undef_types_allocated *= 2;
      undef_types = (struct type **)
	xrealloc ((char *) undef_types,
		  undef_types_allocated * sizeof (struct type *));
    }
  undef_types[undef_types_length++] = type;
}

/* What about types defined as forward references inside of a small lexical
   scope?  */
/* Add a type to the list of undefined types to be checked through
   once this file has been read in.
   
   In practice, we actually maintain two such lists: The first list
   (UNDEF_TYPES) is used for types whose name has been provided, and
   concerns forward references (eg 'xs' or 'xu' forward references);
   the second list (NONAME_UNDEFS) is used for types whose name is
   unknown at creation time, because they were referenced through
   their type number before the actual type was declared.
   This function actually adds the given type to the proper list.  */

static void
add_undefined_type (struct type *type, int typenums[2])
{
  if (type->name () == NULL)
    add_undefined_type_noname (type, typenums);
  else
    add_undefined_type_1 (type);
}

/* Try to fix all undefined types pushed on the UNDEF_TYPES vector.  */

static void
cleanup_undefined_types_noname (struct objfile *objfile)
{
  int i;

  for (i = 0; i < noname_undefs_length; i++)
    {
      struct nat nat = noname_undefs[i];
      struct type **type;

      type = dbx_lookup_type (nat.typenums, objfile);
      if (nat.type != *type && (*type)->code () != TYPE_CODE_UNDEF)
	{
	  /* The instance flags of the undefined type are still unset,
	     and needs to be copied over from the reference type.
	     Since replace_type expects them to be identical, we need
	     to set these flags manually before hand.  */
	  nat.type->set_instance_flags ((*type)->instance_flags ());
	  replace_type (nat.type, *type);
	}
    }

  noname_undefs_length = 0;
}

/* Go through each undefined type, see if it's still undefined, and fix it
   up if possible.  We have two kinds of undefined types:

   TYPE_CODE_ARRAY:  Array whose target type wasn't defined yet.
   Fix:  update array length using the element bounds
   and the target type's length.
   TYPE_CODE_STRUCT, TYPE_CODE_UNION:  Structure whose fields were not
   yet defined at the time a pointer to it was made.
   Fix:  Do a full lookup on the struct/union tag.  */

static void
cleanup_undefined_types_1 (void)
{
  struct type **type;

  /* Iterate over every undefined type, and look for a symbol whose type
     matches our undefined type.  The symbol matches if:
       1. It is a typedef in the STRUCT domain;
       2. It has the same name, and same type code;
       3. The instance flags are identical.
     
     It is important to check the instance flags, because we have seen
     examples where the debug info contained definitions such as:

	 "foo_t:t30=B31=xefoo_t:"

     In this case, we have created an undefined type named "foo_t" whose
     instance flags is null (when processing "xefoo_t"), and then created
     another type with the same name, but with different instance flags
     ('B' means volatile).  I think that the definition above is wrong,
     since the same type cannot be volatile and non-volatile at the same
     time, but we need to be able to cope with it when it happens.  The
     approach taken here is to treat these two types as different.  */

  for (type = undef_types; type < undef_types + undef_types_length; type++)
    {
      switch ((*type)->code ())
	{

	case TYPE_CODE_STRUCT:
	case TYPE_CODE_UNION:
	case TYPE_CODE_ENUM:
	  {
	    /* Check if it has been defined since.  Need to do this here
	       as well as in check_typedef to deal with the (legitimate in
	       C though not C++) case of several types with the same name
	       in different source files.  */
	    if ((*type)->is_stub ())
	      {
		struct pending *ppt;
		int i;
		/* Name of the type, without "struct" or "union".  */
		const char *type_name = (*type)->name ();

		if (type_name == NULL)
		  {
		    complaint (_("need a type name"));
		    break;
		  }
		for (ppt = *get_file_symbols (); ppt; ppt = ppt->next)
		  {
		    for (i = 0; i < ppt->nsyms; i++)
		      {
			struct symbol *sym = ppt->symbol[i];

			if (sym->aclass () == LOC_TYPEDEF
			    && sym->domain () == STRUCT_DOMAIN
			    && (sym->type ()->code () == (*type)->code ())
			    && ((*type)->instance_flags ()
				== sym->type ()->instance_flags ())
			    && strcmp (sym->linkage_name (), type_name) == 0)
			  replace_type (*type, sym->type ());
		      }
		  }
	      }
	  }
	  break;

	default:
	  {
	    complaint (_("forward-referenced types left unresolved, "
		       "type code %d."),
		       (*type)->code ());
	  }
	  break;
	}
    }

  undef_types_length = 0;
}

/* Try to fix all the undefined types we encountered while processing
   this unit.  */

void
cleanup_undefined_stabs_types (struct objfile *objfile)
{
  cleanup_undefined_types_1 ();
  cleanup_undefined_types_noname (objfile);
}

/* See stabsread.h.  */

void
scan_file_globals (struct objfile *objfile)
{
  int hash;
  struct symbol *sym, *prev;
  struct objfile *resolve_objfile;

  /* SVR4 based linkers copy referenced global symbols from shared
     libraries to the main executable.
     If we are scanning the symbols for a shared library, try to resolve
     them from the minimal symbols of the main executable first.  */

  if (current_program_space->symfile_object_file
      && objfile != current_program_space->symfile_object_file)
    resolve_objfile = current_program_space->symfile_object_file;
  else
    resolve_objfile = objfile;

  while (1)
    {
      /* Avoid expensive loop through all minimal symbols if there are
	 no unresolved symbols.  */
      for (hash = 0; hash < HASHSIZE; hash++)
	{
	  if (global_sym_chain[hash])
	    break;
	}
      if (hash >= HASHSIZE)
	return;

      for (minimal_symbol *msymbol : resolve_objfile->msymbols ())
	{
	  QUIT;

	  /* Skip static symbols.  */
	  switch (msymbol->type ())
	    {
	    case mst_file_text:
	    case mst_file_data:
	    case mst_file_bss:
	      continue;
	    default:
	      break;
	    }

	  prev = NULL;

	  /* Get the hash index and check all the symbols
	     under that hash index.  */

	  hash = hashname (msymbol->linkage_name ());

	  for (sym = global_sym_chain[hash]; sym;)
	    {
	      if (strcmp (msymbol->linkage_name (), sym->linkage_name ()) == 0)
		{
		  /* Splice this symbol out of the hash chain and
		     assign the value we have to it.  */
		  if (prev)
		    {
		      prev->set_value_chain (sym->value_chain ());
		    }
		  else
		    {
		      global_sym_chain[hash] = sym->value_chain ();
		    }

		  /* Check to see whether we need to fix up a common block.  */
		  /* Note: this code might be executed several times for
		     the same symbol if there are multiple references.  */
		  if (sym)
		    {
		      if (sym->aclass () == LOC_BLOCK)
			fix_common_block
			  (sym, msymbol->value_address (resolve_objfile),
			   msymbol->section_index ());
		      else
			sym->set_value_address
			  (msymbol->value_address (resolve_objfile));
		      sym->set_section_index (msymbol->section_index ());
		    }

		  if (prev)
		    {
		      sym = prev->value_chain ();
		    }
		  else
		    {
		      sym = global_sym_chain[hash];
		    }
		}
	      else
		{
		  prev = sym;
		  sym = sym->value_chain ();
		}
	    }
	}
      if (resolve_objfile == objfile)
	break;
      resolve_objfile = objfile;
    }

  /* Change the storage class of any remaining unresolved globals to
     LOC_UNRESOLVED and remove them from the chain.  */
  for (hash = 0; hash < HASHSIZE; hash++)
    {
      sym = global_sym_chain[hash];
      while (sym)
	{
	  prev = sym;
	  sym = sym->value_chain ();

	  /* Change the symbol address from the misleading chain value
	     to address zero.  */
	  prev->set_value_address (0);

	  /* Complain about unresolved common block symbols.  */
	  if (prev->aclass () == LOC_STATIC)
	    prev->set_aclass_index (LOC_UNRESOLVED);
	  else
	    complaint (_("%s: common block `%s' from "
			 "global_sym_chain unresolved"),
		       objfile_name (objfile), prev->print_name ());
	}
    }
  memset (global_sym_chain, 0, sizeof (global_sym_chain));
}

/* Initialize anything that needs initializing when starting to read
   a fresh piece of a symbol file, e.g. reading in the stuff corresponding
   to a psymtab.  */

void
stabsread_init (void)
{
}

/* Initialize anything that needs initializing when a completely new
   symbol file is specified (not just adding some symbols from another
   file, e.g. a shared library).  */

void
stabsread_new_init (void)
{
  /* Empty the hash table of global syms looking for values.  */
  memset (global_sym_chain, 0, sizeof (global_sym_chain));
}

/* Initialize anything that needs initializing at the same time as
   start_compunit_symtab() is called.  */

void
start_stabs (void)
{
  global_stabs = NULL;		/* AIX COFF */
  /* Leave FILENUM of 0 free for builtin types and this file's types.  */
  n_this_object_header_files = 1;
  type_vector_length = 0;
  type_vector = (struct type **) 0;
  within_function = 0;

  /* FIXME: If common_block_name is not already NULL, we should complain().  */
  common_block_name = NULL;
}

/* Call after end_compunit_symtab().  */

void
end_stabs (void)
{
  if (type_vector)
    {
      xfree (type_vector);
    }
  type_vector = 0;
  type_vector_length = 0;
  previous_stab_code = 0;
}

void
finish_global_stabs (struct objfile *objfile)
{
  if (global_stabs)
    {
      patch_block_stabs (*get_global_symbols (), global_stabs, objfile);
      xfree (global_stabs);
      global_stabs = NULL;
    }
}

/* Find the end of the name, delimited by a ':', but don't match
   ObjC symbols which look like -[Foo bar::]:bla.  */
static const char *
find_name_end (const char *name)
{
  const char *s = name;

  if (s[0] == '-' || *s == '+')
    {
      /* Must be an ObjC method symbol.  */
      if (s[1] != '[')
	{
	  error (_("invalid symbol name \"%s\""), name);
	}
      s = strchr (s, ']');
      if (s == NULL)
	{
	  error (_("invalid symbol name \"%s\""), name);
	}
      return strchr (s, ':');
    }
  else
    {
      return strchr (s, ':');
    }
}

/* See stabsread.h.  */

int
hashname (const char *name)
{
  return fast_hash (name, strlen (name)) % HASHSIZE;
}

/* Initializer for this module.  */

void _initialize_stabsread ();
void
_initialize_stabsread ()
{
  undef_types_allocated = 20;
  undef_types_length = 0;
  undef_types = XNEWVEC (struct type *, undef_types_allocated);

  noname_undefs_allocated = 20;
  noname_undefs_length = 0;
  noname_undefs = XNEWVEC (struct nat, noname_undefs_allocated);

  stab_register_index = register_symbol_register_impl (LOC_REGISTER,
						       &stab_register_funcs);
  stab_regparm_index = register_symbol_register_impl (LOC_REGPARM_ADDR,
						      &stab_register_funcs);
}
