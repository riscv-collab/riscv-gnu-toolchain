/* Load module for 'compile' command.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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
#include "compile-object-load.h"
#include "compile-internal.h"
#include "command.h"
#include "objfiles.h"
#include "gdbcore.h"
#include "readline/tilde.h"
#include "bfdlink.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "inferior.h"
#include "gdbthread.h"
#include "compile.h"
#include "block.h"
#include "arch-utils.h"
#include <algorithm>

/* Add inferior mmap memory range ADDR..ADDR+SIZE (exclusive) to the
   list.  */

void
munmap_list::add (CORE_ADDR addr, CORE_ADDR size)
{
  struct munmap_item item = { addr, size };
  items.push_back (item);
}

/* Destroy an munmap_list.  */

munmap_list::~munmap_list ()
{
  for (auto &item : items)
    {
      try
	{
	  gdbarch_infcall_munmap (current_inferior ()->arch (),
				  item.addr, item.size);
	}
      catch (const gdb_exception_error &ex)
	{
	  /* There's not much the user can do, so just ignore
	     this.  */
	}
    }
}

/* A data structure that is used to lay out sections of our objfile in
   inferior memory.  */

struct setup_sections_data
{
  explicit setup_sections_data (bfd *abfd)
    : m_bfd (abfd),
      m_last_section_first (abfd->sections)
  {
  }

  /* Place all ABFD sections next to each other obeying all
     constraints.  */
  void setup_one_section (asection *sect);

  /* List of inferior mmap ranges where setup_sections should add its
     next range.  */
  struct munmap_list munmap_list;

private:

  /* The BFD.  */
  bfd *m_bfd;

  /* Size of all recent sections with matching LAST_PROT.  */
  CORE_ADDR m_last_size = 0;

  /* First section matching LAST_PROT.  */
  asection *m_last_section_first;

  /* Memory protection like the prot parameter of gdbarch_infcall_mmap. */
  unsigned m_last_prot = -1;

  /* Maximum of alignments of all sections matching LAST_PROT.
     This value is always at least 1.  This value is always a power of 2.  */
  CORE_ADDR m_last_max_alignment = -1;

};

/* See setup_sections_data.  */

void
setup_sections_data::setup_one_section (asection *sect)
{
  CORE_ADDR alignment;
  unsigned prot;

  if (sect != NULL)
    {
      /* It is required by later bfd_get_relocated_section_contents.  */
      if (sect->output_section == NULL)
	sect->output_section = sect;

      if ((bfd_section_flags (sect) & SEC_ALLOC) == 0)
	return;

      /* Make the memory always readable.  */
      prot = GDB_MMAP_PROT_READ;
      if ((bfd_section_flags (sect) & SEC_READONLY) == 0)
	prot |= GDB_MMAP_PROT_WRITE;
      if ((bfd_section_flags (sect) & SEC_CODE) != 0)
	prot |= GDB_MMAP_PROT_EXEC;

      if (compile_debug)
	gdb_printf (gdb_stdlog,
		    "module \"%s\" section \"%s\" size %s prot %u\n",
		    bfd_get_filename (m_bfd),
		    bfd_section_name (sect),
		    paddress (current_inferior ()->arch (),
			      bfd_section_size (sect)),
		    prot);
    }
  else
    prot = -1;

  if (sect == NULL
      || (m_last_prot != prot && bfd_section_size (sect) != 0))
    {
      CORE_ADDR addr;
      asection *sect_iter;

      if (m_last_size != 0)
	{
	  addr = gdbarch_infcall_mmap (current_inferior ()->arch (),
				       m_last_size, m_last_prot);
	  munmap_list.add (addr, m_last_size);
	  if (compile_debug)
	    gdb_printf (gdb_stdlog,
			"allocated %s bytes at %s prot %u\n",
			paddress (current_inferior ()->arch (), m_last_size),
			paddress (current_inferior ()->arch (), addr),
			m_last_prot);
	}
      else
	addr = 0;

      if ((addr & (m_last_max_alignment - 1)) != 0)
	error (_("Inferior compiled module address %s "
		 "is not aligned to BFD required %s."),
	       paddress (current_inferior ()->arch (), addr),
	       paddress (current_inferior ()->arch (), m_last_max_alignment));

      for (sect_iter = m_last_section_first; sect_iter != sect;
	   sect_iter = sect_iter->next)
	if ((bfd_section_flags (sect_iter) & SEC_ALLOC) != 0)
	  bfd_set_section_vma (sect_iter, addr + bfd_section_vma (sect_iter));

      m_last_size = 0;
      m_last_section_first = sect;
      m_last_prot = prot;
      m_last_max_alignment = 1;
    }

  if (sect == NULL)
    return;

  alignment = ((CORE_ADDR) 1) << bfd_section_alignment (sect);
  m_last_max_alignment = std::max (m_last_max_alignment, alignment);

  m_last_size = (m_last_size + alignment - 1) & -alignment;

  bfd_set_section_vma (sect, m_last_size);

  m_last_size += bfd_section_size (sect);
  m_last_size = (m_last_size + alignment - 1) & -alignment;
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_multiple_definition (struct bfd_link_info *link_info,
				    struct bfd_link_hash_entry *h, bfd *nbfd,
				    asection *nsec, bfd_vma nval)
{
  bfd *abfd = link_info->input_bfds;

  if (link_info->allow_multiple_definition)
    return;
  warning (_("Compiled module \"%s\": multiple symbol definitions: %s"),
	   bfd_get_filename (abfd), h->root.string);
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_warning (struct bfd_link_info *link_info, const char *xwarning,
			const char *symbol, bfd *abfd, asection *section,
			bfd_vma address)
{
  warning (_("Compiled module \"%s\" section \"%s\": warning: %s"),
	   bfd_get_filename (abfd), bfd_section_name (section),
	   xwarning);
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_undefined_symbol (struct bfd_link_info *link_info,
				 const char *name, bfd *abfd, asection *section,
				 bfd_vma address, bfd_boolean is_fatal)
{
  warning (_("Cannot resolve relocation to \"%s\" "
	     "from compiled module \"%s\" section \"%s\"."),
	   name, bfd_get_filename (abfd), bfd_section_name (section));
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_reloc_overflow (struct bfd_link_info *link_info,
			       struct bfd_link_hash_entry *entry,
			       const char *name, const char *reloc_name,
			       bfd_vma addend, bfd *abfd, asection *section,
			       bfd_vma address)
{
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_reloc_dangerous (struct bfd_link_info *link_info,
				const char *message, bfd *abfd,
				asection *section, bfd_vma address)
{
  warning (_("Compiled module \"%s\" section \"%s\": dangerous "
	     "relocation: %s\n"),
	   bfd_get_filename (abfd), bfd_section_name (section),
	   message);
}

/* Helper for link_callbacks callbacks vector.  */

static void
link_callbacks_unattached_reloc (struct bfd_link_info *link_info,
				 const char *name, bfd *abfd, asection *section,
				 bfd_vma address)
{
  warning (_("Compiled module \"%s\" section \"%s\": unattached "
	     "relocation: %s\n"),
	   bfd_get_filename (abfd), bfd_section_name (section),
	   name);
}

/* Helper for link_callbacks callbacks vector.  */

static void link_callbacks_einfo (const char *fmt, ...)
  ATTRIBUTE_PRINTF (1, 2);

static void
link_callbacks_einfo (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  std::string str = string_vprintf (fmt, ap);
  va_end (ap);

  warning (_("Compile module: warning: %s"), str.c_str ());
}

/* Helper for bfd_get_relocated_section_contents.
   Only these symbols are set by bfd_simple_get_relocated_section_contents
   but bfd/ seems to use even the NULL ones without checking them first.  */

static const struct bfd_link_callbacks link_callbacks =
{
  NULL, /* add_archive_element */
  link_callbacks_multiple_definition, /* multiple_definition */
  NULL, /* multiple_common */
  NULL, /* add_to_set */
  NULL, /* constructor */
  link_callbacks_warning, /* warning */
  link_callbacks_undefined_symbol, /* undefined_symbol */
  link_callbacks_reloc_overflow, /* reloc_overflow */
  link_callbacks_reloc_dangerous, /* reloc_dangerous */
  link_callbacks_unattached_reloc, /* unattached_reloc */
  NULL, /* notice */
  link_callbacks_einfo, /* einfo */
  NULL, /* info */
  NULL, /* minfo */
  NULL, /* override_segment_assignment */
};

struct link_hash_table_cleanup_data
{
  explicit link_hash_table_cleanup_data (bfd *abfd_)
    : abfd (abfd_),
      link_next (abfd->link.next)
  {
  }

  ~link_hash_table_cleanup_data ()
  {
    if (abfd->is_linker_output)
      (*abfd->link.hash->hash_table_free) (abfd);
    abfd->link.next = link_next;
  }

  DISABLE_COPY_AND_ASSIGN (link_hash_table_cleanup_data);

private:

  bfd *abfd;
  bfd *link_next;
};

/* Relocate and store into inferior memory each section SECT of ABFD.  */

static void
copy_sections (bfd *abfd, asection *sect, void *data)
{
  asymbol **symbol_table = (asymbol **) data;
  bfd_byte *sect_data_got;
  struct bfd_link_info link_info;
  struct bfd_link_order link_order;
  CORE_ADDR inferior_addr;

  if ((bfd_section_flags (sect) & (SEC_ALLOC | SEC_LOAD))
      != (SEC_ALLOC | SEC_LOAD))
    return;

  if (bfd_section_size (sect) == 0)
    return;

  /* Mostly a copy of bfd_simple_get_relocated_section_contents which GDB
     cannot use as it does not report relocations to undefined symbols.  */
  memset (&link_info, 0, sizeof (link_info));
  link_info.output_bfd = abfd;
  link_info.input_bfds = abfd;
  link_info.input_bfds_tail = &abfd->link.next;

  struct link_hash_table_cleanup_data cleanup_data (abfd);

  abfd->link.next = NULL;
  link_info.hash = bfd_link_hash_table_create (abfd);

  link_info.callbacks = &link_callbacks;

  memset (&link_order, 0, sizeof (link_order));
  link_order.next = NULL;
  link_order.type = bfd_indirect_link_order;
  link_order.offset = 0;
  link_order.size = bfd_section_size (sect);
  link_order.u.indirect.section = sect;

  gdb::unique_xmalloc_ptr<gdb_byte> sect_data
    ((bfd_byte *) xmalloc (bfd_section_size (sect)));

  sect_data_got = bfd_get_relocated_section_contents (abfd, &link_info,
						      &link_order,
						      sect_data.get (),
						      FALSE, symbol_table);

  if (sect_data_got == NULL)
    error (_("Cannot map compiled module \"%s\" section \"%s\": %s"),
	   bfd_get_filename (abfd), bfd_section_name (sect),
	   bfd_errmsg (bfd_get_error ()));
  gdb_assert (sect_data_got == sect_data.get ());

  inferior_addr = bfd_section_vma (sect);
  if (0 != target_write_memory (inferior_addr, sect_data.get (),
				bfd_section_size (sect)))
    error (_("Cannot write compiled module \"%s\" section \"%s\" "
	     "to inferior memory range %s-%s."),
	   bfd_get_filename (abfd), bfd_section_name (sect),
	   paddress (current_inferior ()->arch (), inferior_addr),
	   paddress (current_inferior ()->arch (),
		     inferior_addr + bfd_section_size (sect)));
}

/* Fetch the type of COMPILE_I_EXPR_PTR_TYPE and COMPILE_I_EXPR_VAL
   symbols in OBJFILE so we can calculate how much memory to allocate
   for the out parameter.  This avoids needing a malloc in the generated
   code.  Throw an error if anything fails.
   GDB first tries to compile the code with COMPILE_I_PRINT_ADDRESS_SCOPE.
   If it finds user tries to print an array type this function returns
   NULL.  Caller will then regenerate the code with
   COMPILE_I_PRINT_VALUE_SCOPE, recompiles it again and finally runs it.
   This is because __auto_type array-to-pointer type conversion of
   COMPILE_I_EXPR_VAL which gets detected by COMPILE_I_EXPR_PTR_TYPE
   preserving the array type.  */

static struct type *
get_out_value_type (struct symbol *func_sym, struct objfile *objfile,
		    enum compile_i_scope_types scope)
{
  struct symbol *gdb_ptr_type_sym;
  /* Initialize it just to avoid a GCC false warning.  */
  struct symbol *gdb_val_sym = NULL;
  struct type *gdb_ptr_type, *gdb_type_from_ptr, *gdb_type, *retval;
  /* Initialize it just to avoid a GCC false warning.  */
  const struct block *block = NULL;
  const struct blockvector *bv;
  int nblocks = 0;
  int block_loop = 0;

  lookup_name_info func_matcher (GCC_FE_WRAPPER_FUNCTION,
				 symbol_name_match_type::SEARCH_NAME);

  bv = func_sym->symtab ()->compunit ()->blockvector ();
  nblocks = bv->num_blocks ();

  gdb_ptr_type_sym = NULL;
  for (block_loop = 0; block_loop < nblocks; block_loop++)
    {
      struct symbol *function = NULL;
      const struct block *function_block;

      block = bv->block (block_loop);
      if (block->function () != NULL)
	continue;
      gdb_val_sym = block_lookup_symbol (block,
					 COMPILE_I_EXPR_VAL,
					 symbol_name_match_type::SEARCH_NAME,
					 VAR_DOMAIN);
      if (gdb_val_sym == NULL)
	continue;

      function_block = block;
      while (function_block != bv->static_block ()
	     && function_block != bv->global_block ())
	{
	  function_block = function_block->superblock ();
	  function = function_block->function ();
	  if (function != NULL)
	    break;
	}
      if (function != NULL
	  && function_block->superblock () == bv->static_block ()
	  && symbol_matches_search_name (function, func_matcher))
	break;
    }
  if (block_loop == nblocks)
    error (_("No \"%s\" symbol found"), COMPILE_I_EXPR_VAL);

  gdb_type = gdb_val_sym->type ();
  gdb_type = check_typedef (gdb_type);

  gdb_ptr_type_sym = block_lookup_symbol (block, COMPILE_I_EXPR_PTR_TYPE,
					  symbol_name_match_type::SEARCH_NAME,
					  VAR_DOMAIN);
  if (gdb_ptr_type_sym == NULL)
    error (_("No \"%s\" symbol found"), COMPILE_I_EXPR_PTR_TYPE);
  gdb_ptr_type = gdb_ptr_type_sym->type ();
  gdb_ptr_type = check_typedef (gdb_ptr_type);
  if (gdb_ptr_type->code () != TYPE_CODE_PTR)
    error (_("Type of \"%s\" is not a pointer"), COMPILE_I_EXPR_PTR_TYPE);
  gdb_type_from_ptr = check_typedef (gdb_ptr_type->target_type ());

  if (types_deeply_equal (gdb_type, gdb_type_from_ptr))
    {
      if (scope != COMPILE_I_PRINT_ADDRESS_SCOPE)
	error (_("Expected address scope in compiled module \"%s\"."),
	       objfile_name (objfile));
      return gdb_type;
    }

  if (gdb_type->code () != TYPE_CODE_PTR)
    error (_("Invalid type code %d of symbol \"%s\" "
	     "in compiled module \"%s\"."),
	   gdb_type_from_ptr->code (), COMPILE_I_EXPR_VAL,
	   objfile_name (objfile));
  
  retval = gdb_type_from_ptr;
  switch (gdb_type_from_ptr->code ())
    {
    case TYPE_CODE_ARRAY:
      gdb_type_from_ptr = gdb_type_from_ptr->target_type ();
      break;
    case TYPE_CODE_FUNC:
      break;
    default:
      error (_("Invalid type code %d of symbol \"%s\" "
	       "in compiled module \"%s\"."),
	     gdb_type_from_ptr->code (), COMPILE_I_EXPR_PTR_TYPE,
	     objfile_name (objfile));
    }
  if (!types_deeply_equal (gdb_type_from_ptr,
			   gdb_type->target_type ()))
    error (_("Referenced types do not match for symbols \"%s\" and \"%s\" "
	     "in compiled module \"%s\"."),
	   COMPILE_I_EXPR_PTR_TYPE, COMPILE_I_EXPR_VAL,
	   objfile_name (objfile));
  if (scope == COMPILE_I_PRINT_ADDRESS_SCOPE)
    return NULL;
  return retval;
}

/* Fetch the type of first parameter of FUNC_SYM.
   Return NULL if FUNC_SYM has no parameters.  Throw an error otherwise.  */

static struct type *
get_regs_type (struct symbol *func_sym, struct objfile *objfile)
{
  struct type *func_type = func_sym->type ();
  struct type *regsp_type, *regs_type;

  /* No register parameter present.  */
  if (func_type->num_fields () == 0)
    return NULL;

  regsp_type = check_typedef (func_type->field (0).type ());
  if (regsp_type->code () != TYPE_CODE_PTR)
    error (_("Invalid type code %d of first parameter of function \"%s\" "
	     "in compiled module \"%s\"."),
	   regsp_type->code (), GCC_FE_WRAPPER_FUNCTION,
	   objfile_name (objfile));

  regs_type = check_typedef (regsp_type->target_type ());
  if (regs_type->code () != TYPE_CODE_STRUCT)
    error (_("Invalid type code %d of dereferenced first parameter "
	     "of function \"%s\" in compiled module \"%s\"."),
	   regs_type->code (), GCC_FE_WRAPPER_FUNCTION,
	   objfile_name (objfile));

  return regs_type;
}

/* Store all inferior registers required by REGS_TYPE to inferior memory
   starting at inferior address REGS_BASE.  */

static void
store_regs (struct type *regs_type, CORE_ADDR regs_base)
{
  gdbarch *gdbarch = current_inferior ()->arch ();
  int fieldno;

  for (fieldno = 0; fieldno < regs_type->num_fields (); fieldno++)
    {
      const char *reg_name = regs_type->field (fieldno).name ();
      ULONGEST reg_bitpos = regs_type->field (fieldno).loc_bitpos ();
      ULONGEST reg_bitsize = regs_type->field (fieldno).bitsize ();
      ULONGEST reg_offset;
      struct type *reg_type
	= check_typedef (regs_type->field (fieldno).type ());
      ULONGEST reg_size = reg_type->length ();
      int regnum;
      struct value *regval;
      CORE_ADDR inferior_addr;

      if (strcmp (reg_name, COMPILE_I_SIMPLE_REGISTER_DUMMY) == 0)
	continue;

      if ((reg_bitpos % 8) != 0 || reg_bitsize != 0)
	error (_("Invalid register \"%s\" position %s bits or size %s bits"),
	       reg_name, pulongest (reg_bitpos), pulongest (reg_bitsize));
      reg_offset = reg_bitpos / 8;

      if (reg_type->code () != TYPE_CODE_INT
	  && reg_type->code () != TYPE_CODE_PTR)
	error (_("Invalid register \"%s\" type code %d"), reg_name,
	       reg_type->code ());

      regnum = compile_register_name_demangle (gdbarch, reg_name);

      regval = value_from_register (reg_type, regnum, get_current_frame ());
      if (regval->optimized_out ())
	error (_("Register \"%s\" is optimized out."), reg_name);
      if (!regval->entirely_available ())
	error (_("Register \"%s\" is not available."), reg_name);

      inferior_addr = regs_base + reg_offset;
      if (0 != target_write_memory (inferior_addr,
				    regval->contents ().data (),
				    reg_size))
	error (_("Cannot write register \"%s\" to inferior memory at %s."),
	       reg_name, paddress (gdbarch, inferior_addr));
    }
}

/* Load the object file specified in FILE_NAMES into inferior memory.
   Throw an error otherwise.  Caller must fully dispose the return
   value by calling compile_object_run.  Returns NULL only for
   COMPILE_I_PRINT_ADDRESS_SCOPE when COMPILE_I_PRINT_VALUE_SCOPE
   should have been used instead.  */

compile_module_up
compile_object_load (const compile_file_names &file_names,
		     enum compile_i_scope_types scope, void *scope_data)
{
  CORE_ADDR regs_addr, out_value_addr = 0;
  struct symbol *func_sym;
  struct type *func_type;
  struct bound_minimal_symbol bmsym;
  long storage_needed;
  asymbol **symbol_table, **symp;
  long number_of_symbols, missing_symbols;
  struct type *regs_type, *out_value_type = NULL;
  char **matching;
  struct objfile *objfile;
  int expect_parameters;
  struct type *expect_return_type;

  gdb::unique_xmalloc_ptr<char> filename
    (tilde_expand (file_names.object_file ()));

  gdb_bfd_ref_ptr abfd (gdb_bfd_open (filename.get (), gnutarget));
  if (abfd == NULL)
    error (_("\"%s\": could not open as compiled module: %s"),
	  filename.get (), bfd_errmsg (bfd_get_error ()));

  if (!bfd_check_format_matches (abfd.get (), bfd_object, &matching))
    error (_("\"%s\": not in loadable format: %s"),
	   filename.get (),
	   gdb_bfd_errmsg (bfd_get_error (), matching).c_str ());

  if ((bfd_get_file_flags (abfd.get ()) & (EXEC_P | DYNAMIC)) != 0)
    error (_("\"%s\": not in object format."), filename.get ());

  struct setup_sections_data setup_sections_data (abfd.get ());
  for (asection *sect = abfd->sections; sect != nullptr; sect = sect->next)
    setup_sections_data.setup_one_section (sect);
  setup_sections_data.setup_one_section (nullptr);

  storage_needed = bfd_get_symtab_upper_bound (abfd.get ());
  if (storage_needed < 0)
    error (_("Cannot read symbols of compiled module \"%s\": %s"),
	   filename.get (), bfd_errmsg (bfd_get_error ()));

  /* SYMFILE_VERBOSE is not passed even if FROM_TTY, user is not interested in
     "Reading symbols from ..." message for automatically generated file.  */
  objfile_up objfile_holder (symbol_file_add_from_bfd (abfd,
						       filename.get (),
						       0, NULL, 0, NULL));
  objfile = objfile_holder.get ();

  func_sym = lookup_global_symbol_from_objfile (objfile,
						GLOBAL_BLOCK,
						GCC_FE_WRAPPER_FUNCTION,
						VAR_DOMAIN).symbol;
  if (func_sym == NULL)
    error (_("Cannot find function \"%s\" in compiled module \"%s\"."),
	   GCC_FE_WRAPPER_FUNCTION, objfile_name (objfile));
  func_type = func_sym->type ();
  if (func_type->code () != TYPE_CODE_FUNC)
    error (_("Invalid type code %d of function \"%s\" in compiled "
	     "module \"%s\"."),
	   func_type->code (), GCC_FE_WRAPPER_FUNCTION,
	   objfile_name (objfile));

  switch (scope)
    {
    case COMPILE_I_SIMPLE_SCOPE:
      expect_parameters = 1;
      expect_return_type
	= builtin_type (current_inferior ()->arch ())->builtin_void;
      break;
    case COMPILE_I_RAW_SCOPE:
      expect_parameters = 0;
      expect_return_type
	= builtin_type (current_inferior ()->arch ())->builtin_void;
      break;
    case COMPILE_I_PRINT_ADDRESS_SCOPE:
    case COMPILE_I_PRINT_VALUE_SCOPE:
      expect_parameters = 2;
      expect_return_type
	= builtin_type (current_inferior ()->arch ())->builtin_void;
      break;
    default:
      internal_error (_("invalid scope %d"), scope);
    }
  if (func_type->num_fields () != expect_parameters)
    error (_("Invalid %d parameters of function \"%s\" in compiled "
	     "module \"%s\"."),
	   func_type->num_fields (), GCC_FE_WRAPPER_FUNCTION,
	   objfile_name (objfile));
  if (!types_deeply_equal (expect_return_type, func_type->target_type ()))
    error (_("Invalid return type of function \"%s\" in compiled "
	     "module \"%s\"."),
	   GCC_FE_WRAPPER_FUNCTION, objfile_name (objfile));

  /* The memory may be later needed
     by bfd_generic_get_relocated_section_contents
     called from default_symfile_relocate.  */
  symbol_table = (asymbol **) obstack_alloc (&objfile->objfile_obstack,
					     storage_needed);
  number_of_symbols = bfd_canonicalize_symtab (abfd.get (), symbol_table);
  if (number_of_symbols < 0)
    error (_("Cannot parse symbols of compiled module \"%s\": %s"),
	   filename.get (), bfd_errmsg (bfd_get_error ()));

  missing_symbols = 0;
  for (symp = symbol_table; symp < symbol_table + number_of_symbols; symp++)
    {
      asymbol *sym = *symp;

      if (sym->flags != 0)
	continue;
      sym->flags = BSF_GLOBAL;
      sym->section = bfd_abs_section_ptr;
      if (strcmp (sym->name, "_GLOBAL_OFFSET_TABLE_") == 0)
	{
	  if (compile_debug)
	    gdb_printf (gdb_stdlog,
			"ELF symbol \"%s\" relocated to zero\n",
			sym->name);

	  /* It seems to be a GCC bug, with -mcmodel=large there should be no
	     need for _GLOBAL_OFFSET_TABLE_.  Together with -fPIE the data
	     remain PC-relative even with _GLOBAL_OFFSET_TABLE_ as zero.  */
	  sym->value = 0;
	  continue;
	}
      if (strcmp (sym->name, ".TOC.") == 0)
	{
	  /* Handle the .TOC. symbol as the linker would do.  Set the .TOC.
	     sections value to 0x8000 (see bfd/elf64-ppc.c TOC_BASE_OFF);
	     point the symbol section at the ".toc" section;
	     and pass the toc->vma value into bfd_set_gp_value().
	     If the .toc section is not found, use the first section
	     with the SEC_ALLOC flag set.  In the unlikely case that
	     we still do not have a section identified, fall back to using
	     the "*ABS*" section.  */
	  asection *toc_fallback = bfd_get_section_by_name(abfd.get(), ".toc");
	  if (toc_fallback == NULL)
	    {
	      for (asection *tsect = abfd->sections; tsect != nullptr;
		   tsect = tsect->next)
		 {
		    if (bfd_section_flags (tsect) & SEC_ALLOC)
		       {
			  toc_fallback = tsect;
			  break;
		       }
		 }
	    }

	  if (toc_fallback == NULL)
	    /*  If we've gotten here, we have not found a section usable
		as a backup for the .toc section.  In this case, use the
		absolute (*ABS*) section.  */
	     toc_fallback = bfd_abs_section_ptr;

	  sym->section = toc_fallback;
	  sym->value = 0x8000;
	  bfd_set_gp_value(abfd.get(), toc_fallback->vma);
	  if (compile_debug)
	    gdb_printf (gdb_stdlog,
			"Connecting ELF symbol \"%s\" to the .toc section (%s)\n",
			sym->name,
			paddress (current_inferior ()->arch (), sym->value));
	  continue;
	}

      bmsym = lookup_minimal_symbol (sym->name, NULL, NULL);
      switch (bmsym.minsym == NULL
	      ? mst_unknown : bmsym.minsym->type ())
	{
	case mst_text:
	case mst_bss:
	case mst_data:
	  sym->value = bmsym.value_address ();
	  if (compile_debug)
	    gdb_printf (gdb_stdlog,
			"ELF mst_text symbol \"%s\" relocated to %s\n",
			sym->name,
			paddress (current_inferior ()->arch (), sym->value));
	  break;
	case mst_text_gnu_ifunc:
	  sym->value = gnu_ifunc_resolve_addr (current_inferior ()->arch (),
					       bmsym.value_address ());
	  if (compile_debug)
	    gdb_printf (gdb_stdlog,
			"ELF mst_text_gnu_ifunc symbol \"%s\" "
			"relocated to %s\n",
			sym->name,
			paddress (current_inferior ()->arch (), sym->value));
	  break;
	default:
	  warning (_("Could not find symbol \"%s\" "
		     "for compiled module \"%s\"."),
		   sym->name, filename.get ());
	  missing_symbols++;
	}
    }
  if (missing_symbols)
    error (_("%ld symbols were missing, cannot continue."), missing_symbols);

  bfd_map_over_sections (abfd.get (), copy_sections, symbol_table);

  regs_type = get_regs_type (func_sym, objfile);
  if (regs_type == NULL)
    regs_addr = 0;
  else
    {
      /* Use read-only non-executable memory protection.  */
      regs_addr = gdbarch_infcall_mmap (current_inferior ()->arch (),
					regs_type->length (),
					GDB_MMAP_PROT_READ);
      gdb_assert (regs_addr != 0);
      setup_sections_data.munmap_list.add (regs_addr, regs_type->length ());
      if (compile_debug)
	gdb_printf (gdb_stdlog,
		    "allocated %s bytes at %s for registers\n",
		    paddress (current_inferior ()->arch (),
			      regs_type->length ()),
		    paddress (current_inferior ()->arch (), regs_addr));
      store_regs (regs_type, regs_addr);
    }

  if (scope == COMPILE_I_PRINT_ADDRESS_SCOPE
      || scope == COMPILE_I_PRINT_VALUE_SCOPE)
    {
      out_value_type = get_out_value_type (func_sym, objfile, scope);
      if (out_value_type == NULL)
	return NULL;
      check_typedef (out_value_type);
      out_value_addr = gdbarch_infcall_mmap (current_inferior ()->arch (),
					     out_value_type->length (),
					     (GDB_MMAP_PROT_READ
					      | GDB_MMAP_PROT_WRITE));
      gdb_assert (out_value_addr != 0);
      setup_sections_data.munmap_list.add (out_value_addr,
					   out_value_type->length ());
      if (compile_debug)
	gdb_printf (gdb_stdlog,
		    "allocated %s bytes at %s for printed value\n",
		    paddress (current_inferior ()->arch (),
			      out_value_type->length ()),
		    paddress (current_inferior ()->arch (), out_value_addr));
    }

  compile_module_up retval (new struct compile_module);
  retval->objfile = objfile_holder.release ();
  retval->source_file = file_names.source_file ();
  retval->func_sym = func_sym;
  retval->regs_addr = regs_addr;
  retval->scope = scope;
  retval->scope_data = scope_data;
  retval->out_value_type = out_value_type;
  retval->out_value_addr = out_value_addr;
  retval->munmap_list = std::move (setup_sections_data.munmap_list);

  return retval;
}
