/* Read ELF (Executable and Linking Format) object files for GDB.

   Copyright (C) 1991-2024 Free Software Foundation, Inc.

   Written by Fred Fish at Cygnus Support.

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
#include "elf-bfd.h"
#include "elf/common.h"
#include "elf/internal.h"
#include "elf/mips.h"
#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "stabsread.h"
#include "demangle.h"
#include "filenames.h"
#include "probe.h"
#include "arch-utils.h"
#include "gdbtypes.h"
#include "value.h"
#include "infcall.h"
#include "gdbthread.h"
#include "inferior.h"
#include "regcache.h"
#include "bcache.h"
#include "gdb_bfd.h"
#include "location.h"
#include "auxv.h"
#include "mdebugread.h"
#include "ctfread.h"
#include <string_view>
#include "gdbsupport/scoped_fd.h"
#include "dwarf2/public.h"
#include "cli/cli-cmds.h"

/* Whether ctf should always be read, or only if no dwarf is present.  */
static bool always_read_ctf;

/* The struct elfinfo is available only during ELF symbol table and
   psymtab reading.  It is destroyed at the completion of psymtab-reading.
   It's local to elf_symfile_read.  */

struct elfinfo
  {
    asection *stabsect;		/* Section pointer for .stab section */
    asection *mdebugsect;	/* Section pointer for .mdebug section */
    asection *ctfsect;		/* Section pointer for .ctf section */
  };

/* Type for per-BFD data.  */

typedef std::vector<std::unique_ptr<probe>> elfread_data;

/* Per-BFD data for probe info.  */

static const registry<bfd>::key<elfread_data> probe_key;

/* Minimal symbols located at the GOT entries for .plt - that is the real
   pointer where the given entry will jump to.  It gets updated by the real
   function address during lazy ld.so resolving in the inferior.  These
   minimal symbols are indexed for <tab>-completion.  */

#define SYMBOL_GOT_PLT_SUFFIX "@got.plt"

/* Locate the segments in ABFD.  */

static symfile_segment_data_up
elf_symfile_segments (bfd *abfd)
{
  Elf_Internal_Phdr *phdrs, **segments;
  long phdrs_size;
  int num_phdrs, num_segments, num_sections, i;
  asection *sect;

  phdrs_size = bfd_get_elf_phdr_upper_bound (abfd);
  if (phdrs_size == -1)
    return NULL;

  phdrs = (Elf_Internal_Phdr *) alloca (phdrs_size);
  num_phdrs = bfd_get_elf_phdrs (abfd, phdrs);
  if (num_phdrs == -1)
    return NULL;

  num_segments = 0;
  segments = XALLOCAVEC (Elf_Internal_Phdr *, num_phdrs);
  for (i = 0; i < num_phdrs; i++)
    if (phdrs[i].p_type == PT_LOAD)
      segments[num_segments++] = &phdrs[i];

  if (num_segments == 0)
    return NULL;

  symfile_segment_data_up data (new symfile_segment_data);
  data->segments.reserve (num_segments);

  for (i = 0; i < num_segments; i++)
    data->segments.emplace_back (segments[i]->p_vaddr, segments[i]->p_memsz);

  num_sections = bfd_count_sections (abfd);

  /* All elements are initialized to 0 (map to no segment).  */
  data->segment_info.resize (num_sections);

  for (i = 0, sect = abfd->sections; sect != NULL; i++, sect = sect->next)
    {
      int j;

      if ((bfd_section_flags (sect) & SEC_ALLOC) == 0)
	continue;

      Elf_Internal_Shdr *this_hdr = &elf_section_data (sect)->this_hdr;

      for (j = 0; j < num_segments; j++)
	if (ELF_SECTION_IN_SEGMENT (this_hdr, segments[j]))
	  {
	    data->segment_info[i] = j + 1;
	    break;
	  }

      /* We should have found a segment for every non-empty section.
	 If we haven't, we will not relocate this section by any
	 offsets we apply to the segments.  As an exception, do not
	 warn about SHT_NOBITS sections; in normal ELF execution
	 environments, SHT_NOBITS means zero-initialized and belongs
	 in a segment, but in no-OS environments some tools (e.g. ARM
	 RealView) use SHT_NOBITS for uninitialized data.  Since it is
	 uninitialized, it doesn't need a program header.  Such
	 binaries are not relocatable.  */

      /* Exclude debuginfo files from this warning, too, since those
	 are often not strictly compliant with the standard. See, e.g.,
	 ld/24717 for more discussion.  */
      if (!is_debuginfo_file (abfd)
	  && bfd_section_size (sect) > 0 && j == num_segments
	  && (bfd_section_flags (sect) & SEC_LOAD) != 0)
	warning (_("Loadable section \"%s\" outside of ELF segments\n  in %s"),
		 bfd_section_name (sect), bfd_get_filename (abfd));
    }

  return data;
}

/* We are called once per section from elf_symfile_read.  We
   need to examine each section we are passed, check to see
   if it is something we are interested in processing, and
   if so, stash away some access information for the section.

   For now we recognize the dwarf debug information sections and
   line number sections from matching their section names.  The
   ELF definition is no real help here since it has no direct
   knowledge of DWARF (by design, so any debugging format can be
   used).

   We also recognize the ".stab" sections used by the Sun compilers
   released with Solaris 2.

   FIXME: The section names should not be hardwired strings (what
   should they be?  I don't think most object file formats have enough
   section flags to specify what kind of debug section it is.
   -kingdon).  */

static void
elf_locate_sections (asection *sectp, struct elfinfo *ei)
{
  if (strcmp (sectp->name, ".stab") == 0)
    {
      ei->stabsect = sectp;
    }
  else if (strcmp (sectp->name, ".mdebug") == 0)
    {
      ei->mdebugsect = sectp;
    }
  else if (strcmp (sectp->name, ".ctf") == 0)
    {
      ei->ctfsect = sectp;
    }
}

static struct minimal_symbol *
record_minimal_symbol (minimal_symbol_reader &reader,
		       std::string_view name, bool copy_name,
		       unrelocated_addr address,
		       enum minimal_symbol_type ms_type,
		       asection *bfd_section, struct objfile *objfile)
{
  struct gdbarch *gdbarch = objfile->arch ();

  if (ms_type == mst_text || ms_type == mst_file_text
      || ms_type == mst_text_gnu_ifunc)
    address
      = unrelocated_addr (gdbarch_addr_bits_remove (gdbarch,
						    CORE_ADDR (address)));

  /* We only setup section information for allocatable sections.  Usually
     we'd only expect to find msymbols for allocatable sections, but if the
     ELF is malformed then this might not be the case.  In that case don't
     create an msymbol that references an uninitialised section object.  */
  int section_index = 0;
  if ((bfd_section_flags (bfd_section) & SEC_ALLOC) == SEC_ALLOC
      || bfd_section == bfd_abs_section_ptr)
    section_index = gdb_bfd_section_index (objfile->obfd.get (), bfd_section);

  return reader.record_full (name, copy_name, address, ms_type, section_index);
}

/* Read the symbol table of an ELF file.

   Given an objfile, a symbol table, and a flag indicating whether the
   symbol table contains regular, dynamic, or synthetic symbols, add all
   the global function and data symbols to the minimal symbol table.

   In stabs-in-ELF, as implemented by Sun, there are some local symbols
   defined in the ELF symbol table, which can be used to locate
   the beginnings of sections from each ".o" file that was linked to
   form the executable objfile.  We gather any such info and record it
   in data structures hung off the objfile's private data.  */

#define ST_REGULAR 0
#define ST_DYNAMIC 1
#define ST_SYNTHETIC 2

static void
elf_symtab_read (minimal_symbol_reader &reader,
		 struct objfile *objfile, int type,
		 long number_of_symbols, asymbol **symbol_table,
		 bool copy_names)
{
  struct gdbarch *gdbarch = objfile->arch ();
  asymbol *sym;
  long i;
  CORE_ADDR symaddr;
  enum minimal_symbol_type ms_type;
  /* Name of the last file symbol.  This is either a constant string or is
     saved on the objfile's filename cache.  */
  const char *filesymname = "";
  int stripped = (bfd_get_symcount (objfile->obfd.get ()) == 0);
  int elf_make_msymbol_special_p
    = gdbarch_elf_make_msymbol_special_p (gdbarch);

  for (i = 0; i < number_of_symbols; i++)
    {
      sym = symbol_table[i];
      if (sym->name == NULL || *sym->name == '\0')
	{
	  /* Skip names that don't exist (shouldn't happen), or names
	     that are null strings (may happen).  */
	  continue;
	}

      elf_symbol_type *elf_sym = (elf_symbol_type *) sym;

      /* Skip "special" symbols, e.g. ARM mapping symbols.  These are
	 symbols which do not correspond to objects in the symbol table,
	 but have some other target-specific meaning.  */
      if (bfd_is_target_special_symbol (objfile->obfd.get (), sym))
	{
	  if (gdbarch_record_special_symbol_p (gdbarch))
	    gdbarch_record_special_symbol (gdbarch, objfile, sym);
	  continue;
	}

      if (type == ST_DYNAMIC
	  && sym->section == bfd_und_section_ptr
	  && (sym->flags & BSF_FUNCTION))
	{
	  struct minimal_symbol *msym;
	  bfd *abfd = objfile->obfd.get ();
	  asection *sect;

	  /* Symbol is a reference to a function defined in
	     a shared library.
	     If its value is non zero then it is usually the address
	     of the corresponding entry in the procedure linkage table,
	     plus the desired section offset.
	     If its value is zero then the dynamic linker has to resolve
	     the symbol.  We are unable to find any meaningful address
	     for this symbol in the executable file, so we skip it.  */
	  symaddr = sym->value;
	  if (symaddr == 0)
	    continue;

	  /* sym->section is the undefined section.  However, we want to
	     record the section where the PLT stub resides with the
	     minimal symbol.  Search the section table for the one that
	     covers the stub's address.  */
	  for (sect = abfd->sections; sect != NULL; sect = sect->next)
	    {
	      if ((bfd_section_flags (sect) & SEC_ALLOC) == 0)
		continue;

	      if (symaddr >= bfd_section_vma (sect)
		  && symaddr < bfd_section_vma (sect)
			       + bfd_section_size (sect))
		break;
	    }
	  if (!sect)
	    continue;

	  /* On ia64-hpux, we have discovered that the system linker
	     adds undefined symbols with nonzero addresses that cannot
	     be right (their address points inside the code of another
	     function in the .text section).  This creates problems
	     when trying to determine which symbol corresponds to
	     a given address.

	     We try to detect those buggy symbols by checking which
	     section we think they correspond to.  Normally, PLT symbols
	     are stored inside their own section, and the typical name
	     for that section is ".plt".  So, if there is a ".plt"
	     section, and yet the section name of our symbol does not
	     start with ".plt", we ignore that symbol.  */
	  if (!startswith (sect->name, ".plt")
	      && bfd_get_section_by_name (abfd, ".plt") != NULL)
	    continue;

	  msym = record_minimal_symbol
	    (reader, sym->name, copy_names,
	     unrelocated_addr (symaddr),
	     mst_solib_trampoline, sect, objfile);
	  if (msym != NULL)
	    {
	      msym->filename = filesymname;
	      if (elf_make_msymbol_special_p)
		gdbarch_elf_make_msymbol_special (gdbarch, sym, msym);
	    }
	  continue;
	}

      /* If it is a nonstripped executable, do not enter dynamic
	 symbols, as the dynamic symbol table is usually a subset
	 of the main symbol table.  */
      if (type == ST_DYNAMIC && !stripped)
	continue;
      if (sym->flags & BSF_FILE)
	filesymname = objfile->intern (sym->name);
      else if (sym->flags & BSF_SECTION_SYM)
	continue;
      else if (sym->flags & (BSF_GLOBAL | BSF_LOCAL | BSF_WEAK
			     | BSF_GNU_UNIQUE))
	{
	  struct minimal_symbol *msym;

	  /* Select global/local/weak symbols.  Note that bfd puts abs
	     symbols in their own section, so all symbols we are
	     interested in will have a section.  */
	  /* Bfd symbols are section relative.  */
	  symaddr = sym->value + sym->section->vma;
	  /* For non-absolute symbols, use the type of the section
	     they are relative to, to intuit text/data.  Bfd provides
	     no way of figuring this out for absolute symbols.  */
	  if (sym->section == bfd_abs_section_ptr)
	    {
	      /* This is a hack to get the minimal symbol type
		 right for Irix 5, which has absolute addresses
		 with special section indices for dynamic symbols.

		 NOTE: uweigand-20071112: Synthetic symbols do not
		 have an ELF-private part, so do not touch those.  */
	      unsigned int shndx = type == ST_SYNTHETIC ? 0 :
		elf_sym->internal_elf_sym.st_shndx;

	      switch (shndx)
		{
		case SHN_MIPS_TEXT:
		  ms_type = mst_text;
		  break;
		case SHN_MIPS_DATA:
		  ms_type = mst_data;
		  break;
		case SHN_MIPS_ACOMMON:
		  ms_type = mst_bss;
		  break;
		default:
		  ms_type = mst_abs;
		}

	      /* If it is an Irix dynamic symbol, skip section name
		 symbols, relocate all others by section offset.  */
	      if (ms_type != mst_abs)
		{
		  if (sym->name[0] == '.')
		    continue;
		}
	    }
	  else if (sym->section->flags & SEC_CODE)
	    {
	      if (sym->flags & (BSF_GLOBAL | BSF_WEAK | BSF_GNU_UNIQUE))
		{
		  if (sym->flags & BSF_GNU_INDIRECT_FUNCTION)
		    ms_type = mst_text_gnu_ifunc;
		  else
		    ms_type = mst_text;
		}
	      /* The BSF_SYNTHETIC check is there to omit ppc64 function
		 descriptors mistaken for static functions starting with 'L'.
		 */
	      else if ((sym->name[0] == '.' && sym->name[1] == 'L'
			&& (sym->flags & BSF_SYNTHETIC) == 0)
		       || ((sym->flags & BSF_LOCAL)
			   && sym->name[0] == '$'
			   && sym->name[1] == 'L'))
		/* Looks like a compiler-generated label.  Skip
		   it.  The assembler should be skipping these (to
		   keep executables small), but apparently with
		   gcc on the (deleted) delta m88k SVR4, it loses.
		   So to have us check too should be harmless (but
		   I encourage people to fix this in the assembler
		   instead of adding checks here).  */
		continue;
	      else
		{
		  ms_type = mst_file_text;
		}
	    }
	  else if (sym->section->flags & SEC_ALLOC)
	    {
	      if (sym->flags & (BSF_GLOBAL | BSF_WEAK | BSF_GNU_UNIQUE))
		{
		  if (sym->flags & BSF_GNU_INDIRECT_FUNCTION)
		    {
		      ms_type = mst_data_gnu_ifunc;
		    }
		  else if (sym->section->flags & SEC_LOAD)
		    {
		      ms_type = mst_data;
		    }
		  else
		    {
		      ms_type = mst_bss;
		    }
		}
	      else if (sym->flags & BSF_LOCAL)
		{
		  if (sym->section->flags & SEC_LOAD)
		    {
		      ms_type = mst_file_data;
		    }
		  else
		    {
		      ms_type = mst_file_bss;
		    }
		}
	      else
		{
		  ms_type = mst_unknown;
		}
	    }
	  else
	    {
	      /* FIXME:  Solaris2 shared libraries include lots of
		 odd "absolute" and "undefined" symbols, that play
		 hob with actions like finding what function the PC
		 is in.  Ignore them if they aren't text, data, or bss.  */
	      /* ms_type = mst_unknown; */
	      continue;	/* Skip this symbol.  */
	    }
	  msym = record_minimal_symbol
	    (reader, sym->name, copy_names, unrelocated_addr (symaddr),
	     ms_type, sym->section, objfile);

	  if (msym)
	    {
	      /* NOTE: uweigand-20071112: A synthetic symbol does not have an
		 ELF-private part.  */
	      if (type != ST_SYNTHETIC)
		{
		  /* Pass symbol size field in via BFD.  FIXME!!!  */
		  msym->set_size (elf_sym->internal_elf_sym.st_size);
		}

	      msym->filename = filesymname;
	      if (elf_make_msymbol_special_p)
		gdbarch_elf_make_msymbol_special (gdbarch, sym, msym);
	    }

	  /* If we see a default versioned symbol, install it under
	     its version-less name.  */
	  if (msym != NULL)
	    {
	      const char *atsign = strchr (sym->name, '@');
	      bool is_at_symbol = atsign != nullptr && atsign > sym->name;
	      bool is_plt = is_at_symbol && strcmp (atsign, "@plt") == 0;
	      int len = is_at_symbol ? atsign - sym->name : 0;

	      if (is_at_symbol
		  && !is_plt
		  && (elf_sym->version & VERSYM_HIDDEN) == 0)
		record_minimal_symbol (reader,
				       std::string_view (sym->name, len),
				       true, unrelocated_addr (symaddr),
				       ms_type, sym->section, objfile);
	      else if (is_plt)
		{
		  /* For @plt symbols, also record a trampoline to the
		     destination symbol.  The @plt symbol will be used
		     in disassembly, and the trampoline will be used
		     when we are trying to find the target.  */
		  if (ms_type == mst_text && type == ST_SYNTHETIC)
		    {
		      struct minimal_symbol *mtramp;

		      mtramp = record_minimal_symbol
			(reader, std::string_view (sym->name, len), true,
			 unrelocated_addr (symaddr),
			 mst_solib_trampoline, sym->section, objfile);
		      if (mtramp)
			{
			  mtramp->set_size (msym->size());
			  mtramp->created_by_gdb = 1;
			  mtramp->filename = filesymname;
			  if (elf_make_msymbol_special_p)
			    gdbarch_elf_make_msymbol_special (gdbarch,
							      sym, mtramp);
			}
		    }
		}
	    }
	}
    }
}

/* Build minimal symbols named `function@got.plt' (see SYMBOL_GOT_PLT_SUFFIX)
   for later look ups of which function to call when user requests
   a STT_GNU_IFUNC function.  As the STT_GNU_IFUNC type is found at the target
   library defining `function' we cannot yet know while reading OBJFILE which
   of the SYMBOL_GOT_PLT_SUFFIX entries will be needed and later
   DYN_SYMBOL_TABLE is no longer easily available for OBJFILE.  */

static void
elf_rel_plt_read (minimal_symbol_reader &reader,
		  struct objfile *objfile, asymbol **dyn_symbol_table)
{
  bfd *obfd = objfile->obfd.get ();
  const struct elf_backend_data *bed = get_elf_backend_data (obfd);
  asection *relplt, *got_plt;
  bfd_size_type reloc_count, reloc;
  struct gdbarch *gdbarch = objfile->arch ();
  struct type *ptr_type = builtin_type (gdbarch)->builtin_data_ptr;
  size_t ptr_size = ptr_type->length ();

  if (objfile->separate_debug_objfile_backlink)
    return;

  got_plt = bfd_get_section_by_name (obfd, ".got.plt");
  if (got_plt == NULL)
    {
      /* For platforms where there is no separate .got.plt.  */
      got_plt = bfd_get_section_by_name (obfd, ".got");
      if (got_plt == NULL)
	return;
    }

  /* Depending on system, we may find jump slots in a relocation
     section for either .got.plt or .plt.  */
  asection *plt = bfd_get_section_by_name (obfd, ".plt");
  int plt_elf_idx = (plt != NULL) ? elf_section_data (plt)->this_idx : -1;

  int got_plt_elf_idx = elf_section_data (got_plt)->this_idx;

  /* This search algorithm is from _bfd_elf_canonicalize_dynamic_reloc.  */
  for (relplt = obfd->sections; relplt != NULL; relplt = relplt->next)
    {
      const auto &this_hdr = elf_section_data (relplt)->this_hdr;

      if (this_hdr.sh_type == SHT_REL || this_hdr.sh_type == SHT_RELA)
	{
	  if (this_hdr.sh_info == plt_elf_idx
	      || this_hdr.sh_info == got_plt_elf_idx)
	    break;
	}
    }
  if (relplt == NULL)
    return;

  if (! bed->s->slurp_reloc_table (obfd, relplt, dyn_symbol_table, TRUE))
    return;

  std::string string_buffer;

  /* Does ADDRESS reside in SECTION of OBFD?  */
  auto within_section = [obfd] (asection *section, CORE_ADDR address)
    {
      if (section == NULL)
	return false;

      return (bfd_section_vma (section) <= address
	      && (address < bfd_section_vma (section)
		  + bfd_section_size (section)));
    };

  reloc_count = relplt->size / elf_section_data (relplt)->this_hdr.sh_entsize;
  for (reloc = 0; reloc < reloc_count; reloc++)
    {
      const char *name;
      struct minimal_symbol *msym;
      CORE_ADDR address;
      const char *got_suffix = SYMBOL_GOT_PLT_SUFFIX;
      const size_t got_suffix_len = strlen (SYMBOL_GOT_PLT_SUFFIX);

      name = bfd_asymbol_name (*relplt->relocation[reloc].sym_ptr_ptr);
      address = relplt->relocation[reloc].address;

      asection *msym_section;

      /* Does the pointer reside in either the .got.plt or .plt
	 sections?  */
      if (within_section (got_plt, address))
	msym_section = got_plt;
      else if (within_section (plt, address))
	msym_section = plt;
      else
	continue;

      /* We cannot check if NAME is a reference to
	 mst_text_gnu_ifunc/mst_data_gnu_ifunc as in OBJFILE the
	 symbol is undefined and the objfile having NAME defined may
	 not yet have been loaded.  */

      string_buffer.assign (name);
      string_buffer.append (got_suffix, got_suffix + got_suffix_len);

      msym = record_minimal_symbol (reader, string_buffer,
				    true, unrelocated_addr (address),
				    mst_slot_got_plt, msym_section, objfile);
      if (msym)
	msym->set_size (ptr_size);
    }
}

/* The data pointer is htab_t for gnu_ifunc_record_cache_unchecked.  */

static const registry<objfile>::key<htab, htab_deleter>
  elf_objfile_gnu_ifunc_cache_data;

/* Map function names to CORE_ADDR in elf_objfile_gnu_ifunc_cache_data.  */

struct elf_gnu_ifunc_cache
{
  /* This is always a function entry address, not a function descriptor.  */
  CORE_ADDR addr;

  char name[1];
};

/* htab_hash for elf_objfile_gnu_ifunc_cache_data.  */

static hashval_t
elf_gnu_ifunc_cache_hash (const void *a_voidp)
{
  const struct elf_gnu_ifunc_cache *a
    = (const struct elf_gnu_ifunc_cache *) a_voidp;

  return htab_hash_string (a->name);
}

/* htab_eq for elf_objfile_gnu_ifunc_cache_data.  */

static int
elf_gnu_ifunc_cache_eq (const void *a_voidp, const void *b_voidp)
{
  const struct elf_gnu_ifunc_cache *a
    = (const struct elf_gnu_ifunc_cache *) a_voidp;
  const struct elf_gnu_ifunc_cache *b
    = (const struct elf_gnu_ifunc_cache *) b_voidp;

  return strcmp (a->name, b->name) == 0;
}

/* Record the target function address of a STT_GNU_IFUNC function NAME is the
   function entry address ADDR.  Return 1 if NAME and ADDR are considered as
   valid and therefore they were successfully recorded, return 0 otherwise.

   Function does not expect a duplicate entry.  Use
   elf_gnu_ifunc_resolve_by_cache first to check if the entry for NAME already
   exists.  */

static int
elf_gnu_ifunc_record_cache (const char *name, CORE_ADDR addr)
{
  struct bound_minimal_symbol msym;
  struct objfile *objfile;
  htab_t htab;
  struct elf_gnu_ifunc_cache entry_local, *entry_p;
  void **slot;

  msym = lookup_minimal_symbol_by_pc (addr);
  if (msym.minsym == NULL)
    return 0;
  if (msym.value_address () != addr)
    return 0;
  objfile = msym.objfile;

  /* If .plt jumps back to .plt the symbol is still deferred for later
     resolution and it has no use for GDB.  */
  const char *target_name = msym.minsym->linkage_name ();
  size_t len = strlen (target_name);

  /* Note we check the symbol's name instead of checking whether the
     symbol is in the .plt section because some systems have @plt
     symbols in the .text section.  */
  if (len > 4 && strcmp (target_name + len - 4, "@plt") == 0)
    return 0;

  if (strcmp (target_name, "_PROCEDURE_LINKAGE_TABLE_") == 0)
    return 0;

  htab = elf_objfile_gnu_ifunc_cache_data.get (objfile);
  if (htab == NULL)
    {
      htab = htab_create_alloc (1, elf_gnu_ifunc_cache_hash,
				elf_gnu_ifunc_cache_eq,
				NULL, xcalloc, xfree);
      elf_objfile_gnu_ifunc_cache_data.set (objfile, htab);
    }

  entry_local.addr = addr;
  obstack_grow (&objfile->objfile_obstack, &entry_local,
		offsetof (struct elf_gnu_ifunc_cache, name));
  obstack_grow_str0 (&objfile->objfile_obstack, name);
  entry_p
    = (struct elf_gnu_ifunc_cache *) obstack_finish (&objfile->objfile_obstack);

  slot = htab_find_slot (htab, entry_p, INSERT);
  if (*slot != NULL)
    {
      struct elf_gnu_ifunc_cache *entry_found_p
	= (struct elf_gnu_ifunc_cache *) *slot;
      struct gdbarch *gdbarch = objfile->arch ();

      if (entry_found_p->addr != addr)
	{
	  /* This case indicates buggy inferior program, the resolved address
	     should never change.  */

	    warning (_("gnu-indirect-function \"%s\" has changed its resolved "
		       "function_address from %s to %s"),
		     name, paddress (gdbarch, entry_found_p->addr),
		     paddress (gdbarch, addr));
	}

      /* New ENTRY_P is here leaked/duplicate in the OBJFILE obstack.  */
    }
  *slot = entry_p;

  return 1;
}

/* Try to find the target resolved function entry address of a STT_GNU_IFUNC
   function NAME.  If the address is found it is stored to *ADDR_P (if ADDR_P
   is not NULL) and the function returns 1.  It returns 0 otherwise.

   Only the elf_objfile_gnu_ifunc_cache_data hash table is searched by this
   function.  */

static int
elf_gnu_ifunc_resolve_by_cache (const char *name, CORE_ADDR *addr_p)
{
  int found = 0;

  /* FIXME: we only search the initial namespace.

     To search other namespaces, we would need to provide context, e.g. in
     form of an objfile in that namespace.  */
  gdbarch_iterate_over_objfiles_in_search_order
    (current_inferior ()->arch (),
     [name, &addr_p, &found] (struct objfile *objfile)
       {
	 htab_t htab;
	 elf_gnu_ifunc_cache *entry_p;
	 void **slot;

	 htab = elf_objfile_gnu_ifunc_cache_data.get (objfile);
	 if (htab == NULL)
	   return 0;

	 entry_p = ((elf_gnu_ifunc_cache *)
		    alloca (sizeof (*entry_p) + strlen (name)));
	 strcpy (entry_p->name, name);

	 slot = htab_find_slot (htab, entry_p, NO_INSERT);
	 if (slot == NULL)
	   return 0;
	 entry_p = (elf_gnu_ifunc_cache *) *slot;
	 gdb_assert (entry_p != NULL);

	 if (addr_p)
	   *addr_p = entry_p->addr;

	 found = 1;
	 return 1;
       }, nullptr);

  return found;
}

/* Try to find the target resolved function entry address of a STT_GNU_IFUNC
   function NAME.  If the address is found it is stored to *ADDR_P (if ADDR_P
   is not NULL) and the function returns 1.  It returns 0 otherwise.

   Only the SYMBOL_GOT_PLT_SUFFIX locations are searched by this function.
   elf_gnu_ifunc_resolve_by_cache must have been already called for NAME to
   prevent cache entries duplicates.  */

static int
elf_gnu_ifunc_resolve_by_got (const char *name, CORE_ADDR *addr_p)
{
  char *name_got_plt;
  const size_t got_suffix_len = strlen (SYMBOL_GOT_PLT_SUFFIX);
  int found = 0;

  name_got_plt = (char *) alloca (strlen (name) + got_suffix_len + 1);
  sprintf (name_got_plt, "%s" SYMBOL_GOT_PLT_SUFFIX, name);

  /* FIXME: we only search the initial namespace.

     To search other namespaces, we would need to provide context, e.g. in
     form of an objfile in that namespace.  */
  gdbarch_iterate_over_objfiles_in_search_order
    (current_inferior ()->arch (),
     [name, name_got_plt, &addr_p, &found] (struct objfile *objfile)
       {
	 bfd *obfd = objfile->obfd.get ();
	 struct gdbarch *gdbarch = objfile->arch ();
	 type *ptr_type = builtin_type (gdbarch)->builtin_data_ptr;
	 size_t ptr_size = ptr_type->length ();
	 CORE_ADDR pointer_address, addr;
	 asection *plt;
	 gdb_byte *buf = (gdb_byte *) alloca (ptr_size);
	 bound_minimal_symbol msym;

	 msym = lookup_minimal_symbol (name_got_plt, NULL, objfile);
	 if (msym.minsym == NULL)
	   return 0;
	 if (msym.minsym->type () != mst_slot_got_plt)
	   return 0;
	 pointer_address = msym.value_address ();

	 plt = bfd_get_section_by_name (obfd, ".plt");
	 if (plt == NULL)
	   return 0;

	 if (msym.minsym->size () != ptr_size)
	   return 0;
	 if (target_read_memory (pointer_address, buf, ptr_size) != 0)
	   return 0;
	 addr = extract_typed_address (buf, ptr_type);
	 addr = gdbarch_convert_from_func_ptr_addr
	   (gdbarch, addr, current_inferior ()->top_target ());
	 addr = gdbarch_addr_bits_remove (gdbarch, addr);

	 if (elf_gnu_ifunc_record_cache (name, addr))
	   {
	     if (addr_p != NULL)
	       *addr_p = addr;

	     found = 1;
	     return 1;
	   }

	 return 0;
       }, nullptr);

  return found;
}

/* Try to find the target resolved function entry address of a STT_GNU_IFUNC
   function NAME.  If the address is found it is stored to *ADDR_P (if ADDR_P
   is not NULL) and the function returns true.  It returns false otherwise.

   Both the elf_objfile_gnu_ifunc_cache_data hash table and
   SYMBOL_GOT_PLT_SUFFIX locations are searched by this function.  */

static bool
elf_gnu_ifunc_resolve_name (const char *name, CORE_ADDR *addr_p)
{
  if (elf_gnu_ifunc_resolve_by_cache (name, addr_p))
    return true;

  if (elf_gnu_ifunc_resolve_by_got (name, addr_p))
    return true;

  return false;
}

/* Call STT_GNU_IFUNC - a function returning addresss of a real function to
   call.  PC is theSTT_GNU_IFUNC resolving function entry.  The value returned
   is the entry point of the resolved STT_GNU_IFUNC target function to call.
   */

static CORE_ADDR
elf_gnu_ifunc_resolve_addr (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  const char *name_at_pc;
  CORE_ADDR start_at_pc, address;
  struct type *func_func_type = builtin_type (gdbarch)->builtin_func_func;
  struct value *function, *address_val;
  CORE_ADDR hwcap = 0;
  struct value *hwcap_val;

  /* Try first any non-intrusive methods without an inferior call.  */

  if (find_pc_partial_function (pc, &name_at_pc, &start_at_pc, NULL)
      && start_at_pc == pc)
    {
      if (elf_gnu_ifunc_resolve_name (name_at_pc, &address))
	return address;
    }
  else
    name_at_pc = NULL;

  function = value::allocate (func_func_type);
  function->set_lval (lval_memory);
  function->set_address (pc);

  /* STT_GNU_IFUNC resolver functions usually receive the HWCAP vector as
     parameter.  FUNCTION is the function entry address.  ADDRESS may be a
     function descriptor.  */

  target_auxv_search (AT_HWCAP, &hwcap);
  hwcap_val = value_from_longest (builtin_type (gdbarch)
				  ->builtin_unsigned_long, hwcap);
  address_val = call_function_by_hand (function, NULL, hwcap_val);
  address = value_as_address (address_val);
  address = gdbarch_convert_from_func_ptr_addr
    (gdbarch, address, current_inferior ()->top_target ());
  address = gdbarch_addr_bits_remove (gdbarch, address);

  if (name_at_pc)
    elf_gnu_ifunc_record_cache (name_at_pc, address);

  return address;
}

/* Handle inferior hit of bp_gnu_ifunc_resolver, see its definition.  */

static void
elf_gnu_ifunc_resolver_stop (code_breakpoint *b)
{
  struct breakpoint *b_return;
  frame_info_ptr prev_frame = get_prev_frame (get_current_frame ());
  struct frame_id prev_frame_id = get_stack_frame_id (prev_frame);
  CORE_ADDR prev_pc = get_frame_pc (prev_frame);
  int thread_id = inferior_thread ()->global_num;

  gdb_assert (b->type == bp_gnu_ifunc_resolver);

  for (b_return = b->related_breakpoint; b_return != b;
       b_return = b_return->related_breakpoint)
    {
      gdb_assert (b_return->type == bp_gnu_ifunc_resolver_return);
      gdb_assert (b_return->has_single_location ());
      gdb_assert (frame_id_p (b_return->frame_id));

      if (b_return->thread == thread_id
	  && b_return->first_loc ().requested_address == prev_pc
	  && b_return->frame_id == prev_frame_id)
	break;
    }

  if (b_return == b)
    {
      /* No need to call find_pc_line for symbols resolving as this is only
	 a helper breakpointer never shown to the user.  */

      symtab_and_line sal;
      sal.pspace = current_inferior ()->pspace;
      sal.pc = prev_pc;
      sal.section = find_pc_overlay (sal.pc);
      sal.explicit_pc = 1;
      b_return
	= set_momentary_breakpoint (get_frame_arch (prev_frame), sal,
				    prev_frame_id,
				    bp_gnu_ifunc_resolver_return).release ();

      /* set_momentary_breakpoint invalidates PREV_FRAME.  */
      prev_frame = NULL;

      /* Add new b_return to the ring list b->related_breakpoint.  */
      gdb_assert (b_return->related_breakpoint == b_return);
      b_return->related_breakpoint = b->related_breakpoint;
      b->related_breakpoint = b_return;
    }
}

/* Handle inferior hit of bp_gnu_ifunc_resolver_return, see its definition.  */

static void
elf_gnu_ifunc_resolver_return_stop (code_breakpoint *b)
{
  thread_info *thread = inferior_thread ();
  struct gdbarch *gdbarch = get_frame_arch (get_current_frame ());
  struct type *func_func_type = builtin_type (gdbarch)->builtin_func_func;
  struct type *value_type = func_func_type->target_type ();
  struct regcache *regcache = get_thread_regcache (thread);
  struct value *func_func;
  struct value *value;
  CORE_ADDR resolved_address, resolved_pc;

  gdb_assert (b->type == bp_gnu_ifunc_resolver_return);

  while (b->related_breakpoint != b)
    {
      struct breakpoint *b_next = b->related_breakpoint;

      switch (b->type)
	{
	case bp_gnu_ifunc_resolver:
	  break;
	case bp_gnu_ifunc_resolver_return:
	  delete_breakpoint (b);
	  break;
	default:
	  internal_error (_("handle_inferior_event: Invalid "
			    "gnu-indirect-function breakpoint type %d"),
			  (int) b->type);
	}
      b = gdb::checked_static_cast<code_breakpoint *> (b_next);
    }
  gdb_assert (b->type == bp_gnu_ifunc_resolver);
  gdb_assert (b->has_single_location ());

  func_func = value::allocate (func_func_type);
  func_func->set_lval (lval_memory);
  func_func->set_address (b->first_loc ().related_address);

  value = value::allocate (value_type);
  gdbarch_return_value_as_value (gdbarch, func_func, value_type, regcache,
				 &value, NULL);
  resolved_address = value_as_address (value);
  resolved_pc = gdbarch_convert_from_func_ptr_addr
    (gdbarch, resolved_address, current_inferior ()->top_target ());
  resolved_pc = gdbarch_addr_bits_remove (gdbarch, resolved_pc);

  gdb_assert (current_program_space == b->pspace || b->pspace == NULL);
  elf_gnu_ifunc_record_cache (b->locspec->to_string (), resolved_pc);

  b->type = bp_breakpoint;
  update_breakpoint_locations (b, current_program_space,
			       find_function_start_sal (resolved_pc, NULL, true),
			       {});
}

/* A helper function for elf_symfile_read that reads the minimal
   symbols.  */

static void
elf_read_minimal_symbols (struct objfile *objfile, int symfile_flags,
			  const struct elfinfo *ei)
{
  bfd *synth_abfd, *abfd = objfile->obfd.get ();
  long symcount = 0, dynsymcount = 0, synthcount, storage_needed;
  asymbol **symbol_table = NULL, **dyn_symbol_table = NULL;
  asymbol *synthsyms;

  symtab_create_debug_printf ("reading minimal symbols of objfile %s",
			      objfile_name (objfile));

  /* If we already have minsyms, then we can skip some work here.
     However, if there were stabs or mdebug sections, we go ahead and
     redo all the work anyway, because the psym readers for those
     kinds of debuginfo need extra information found here.  This can
     go away once all types of symbols are in the per-BFD object.  */
  if (objfile->per_bfd->minsyms_read
      && ei->stabsect == NULL
      && ei->mdebugsect == NULL
      && ei->ctfsect == NULL)
    {
      symtab_create_debug_printf ("minimal symbols were previously read");
      return;
    }

  minimal_symbol_reader reader (objfile);

  /* Process the normal ELF symbol table first.  */

  storage_needed = bfd_get_symtab_upper_bound (objfile->obfd.get ());
  if (storage_needed < 0)
    error (_("Can't read symbols from %s: %s"),
	   bfd_get_filename (objfile->obfd.get ()),
	   bfd_errmsg (bfd_get_error ()));

  if (storage_needed > 0)
    {
      /* Memory gets permanently referenced from ABFD after
	 bfd_canonicalize_symtab so it must not get freed before ABFD gets.  */

      symbol_table = (asymbol **) bfd_alloc (abfd, storage_needed);
      symcount = bfd_canonicalize_symtab (objfile->obfd.get (), symbol_table);

      if (symcount < 0)
	error (_("Can't read symbols from %s: %s"),
	       bfd_get_filename (objfile->obfd.get ()),
	       bfd_errmsg (bfd_get_error ()));

      elf_symtab_read (reader, objfile, ST_REGULAR, symcount, symbol_table,
		       false);
    }

  /* Add the dynamic symbols.  */

  storage_needed = bfd_get_dynamic_symtab_upper_bound (objfile->obfd.get ());

  if (storage_needed > 0)
    {
      /* Memory gets permanently referenced from ABFD after
	 bfd_get_synthetic_symtab so it must not get freed before ABFD gets.
	 It happens only in the case when elf_slurp_reloc_table sees
	 asection->relocation NULL.  Determining which section is asection is
	 done by _bfd_elf_get_synthetic_symtab which is all a bfd
	 implementation detail, though.  */

      dyn_symbol_table = (asymbol **) bfd_alloc (abfd, storage_needed);
      dynsymcount = bfd_canonicalize_dynamic_symtab (objfile->obfd.get (),
						     dyn_symbol_table);

      if (dynsymcount < 0)
	error (_("Can't read symbols from %s: %s"),
	       bfd_get_filename (objfile->obfd.get ()),
	       bfd_errmsg (bfd_get_error ()));

      elf_symtab_read (reader, objfile, ST_DYNAMIC, dynsymcount,
		       dyn_symbol_table, false);

      elf_rel_plt_read (reader, objfile, dyn_symbol_table);
    }

  /* Contrary to binutils --strip-debug/--only-keep-debug the strip command from
     elfutils (eu-strip) moves even the .symtab section into the .debug file.

     bfd_get_synthetic_symtab on ppc64 for each function descriptor ELF symbol
     'name' creates a new BSF_SYNTHETIC ELF symbol '.name' with its code
     address.  But with eu-strip files bfd_get_synthetic_symtab would fail to
     read the code address from .opd while it reads the .symtab section from
     a separate debug info file as the .opd section is SHT_NOBITS there.

     With SYNTH_ABFD the .opd section will be read from the original
     backlinked binary where it is valid.  */

  if (objfile->separate_debug_objfile_backlink)
    synth_abfd = objfile->separate_debug_objfile_backlink->obfd.get ();
  else
    synth_abfd = abfd;

  /* Add synthetic symbols - for instance, names for any PLT entries.  */

  synthcount = bfd_get_synthetic_symtab (synth_abfd, symcount, symbol_table,
					 dynsymcount, dyn_symbol_table,
					 &synthsyms);
  if (synthcount > 0)
    {
      long i;

      std::unique_ptr<asymbol *[]>
	synth_symbol_table (new asymbol *[synthcount]);
      for (i = 0; i < synthcount; i++)
	synth_symbol_table[i] = synthsyms + i;
      elf_symtab_read (reader, objfile, ST_SYNTHETIC, synthcount,
		       synth_symbol_table.get (), true);

      xfree (synthsyms);
      synthsyms = NULL;
    }

  /* Install any minimal symbols that have been collected as the current
     minimal symbols for this objfile.  The debug readers below this point
     should not generate new minimal symbols; if they do it's their
     responsibility to install them.  "mdebug" appears to be the only one
     which will do this.  */

  reader.install ();

  symtab_create_debug_printf ("done reading minimal symbols");
}

/* Dwarf-specific helper for elf_symfile_read.  Return true if we managed to
   load dwarf debug info.  */

static bool
elf_symfile_read_dwarf2 (struct objfile *objfile,
			 symfile_add_flags symfile_flags)
{
  bool has_dwarf2 = true;

  if (dwarf2_initialize_objfile (objfile, nullptr, true))
    {
      /* Nothing.  */
    }
  /* If the file has its own symbol tables it has no separate debug
     info.  `.dynsym'/`.symtab' go to MSYMBOLS, `.debug_info' goes to
     SYMTABS/PSYMTABS.	`.gnu_debuglink' may no longer be present with
     `.note.gnu.build-id'.

     .gnu_debugdata is !objfile::has_partial_symbols because it contains only
     .symtab, not .debug_* section.  But if we already added .gnu_debugdata as
     an objfile via find_separate_debug_file_in_section there was no separate
     debug info available.  Therefore do not attempt to search for another one,
     objfile->separate_debug_objfile->separate_debug_objfile GDB guarantees to
     be NULL and we would possibly violate it.	*/

  else if (!objfile->has_partial_symbols ()
	   && objfile->separate_debug_objfile == NULL
	   && objfile->separate_debug_objfile_backlink == NULL)
    {
      if (objfile->find_and_add_separate_symbol_file (symfile_flags))
	gdb_assert (objfile->separate_debug_objfile != nullptr);
      else
	has_dwarf2 = false;
    }

  return has_dwarf2;
}

/* Scan and build partial symbols for a symbol file.
   We have been initialized by a call to elf_symfile_init, which
   currently does nothing.

   This function only does the minimum work necessary for letting the
   user "name" things symbolically; it does not read the entire symtab.
   Instead, it reads the external and static symbols and puts them in partial
   symbol tables.  When more extensive information is requested of a
   file, the corresponding partial symbol table is mutated into a full
   fledged symbol table by going back and reading the symbols
   for real.

   We look for sections with specific names, to tell us what debug
   format to look for:  FIXME!!!

   elfstab_build_psymtabs() handles STABS symbols;
   mdebug_build_psymtabs() handles ECOFF debugging information.

   Note that ELF files have a "minimal" symbol table, which looks a lot
   like a COFF symbol table, but has only the minimal information necessary
   for linking.  We process this also, and use the information to
   build gdb's minimal symbol table.  This gives us some minimal debugging
   capability even for files compiled without -g.  */

static void
elf_symfile_read (struct objfile *objfile, symfile_add_flags symfile_flags)
{
  bfd *abfd = objfile->obfd.get ();
  struct elfinfo ei;

  memset ((char *) &ei, 0, sizeof (ei));
  if (!(objfile->flags & OBJF_READNEVER))
    {
      for (asection *sect : gdb_bfd_sections (abfd))
	elf_locate_sections (sect, &ei);
    }

  elf_read_minimal_symbols (objfile, symfile_flags, &ei);

  /* ELF debugging information is inserted into the psymtab in the
     order of least informative first - most informative last.  Since
     the psymtab table is searched `most recent insertion first' this
     increases the probability that more detailed debug information
     for a section is found.

     For instance, an object file might contain both .mdebug (XCOFF)
     and .debug_info (DWARF2) sections then .mdebug is inserted first
     (searched last) and DWARF2 is inserted last (searched first).  If
     we don't do this then the XCOFF info is found first - for code in
     an included file XCOFF info is useless.  */

  if (ei.mdebugsect)
    {
      const struct ecoff_debug_swap *swap;

      /* .mdebug section, presumably holding ECOFF debugging
	 information.  */
      swap = get_elf_backend_data (abfd)->elf_backend_ecoff_debug_swap;
      if (swap)
	elfmdebug_build_psymtabs (objfile, swap, ei.mdebugsect);
    }
  if (ei.stabsect)
    {
      asection *str_sect;

      /* Stab sections have an associated string table that looks like
	 a separate section.  */
      str_sect = bfd_get_section_by_name (abfd, ".stabstr");

      /* FIXME should probably warn about a stab section without a stabstr.  */
      if (str_sect)
	elfstab_build_psymtabs (objfile,
				ei.stabsect,
				str_sect->filepos,
				bfd_section_size (str_sect));
    }

  /* Read the CTF section only if there is no DWARF info.  */
  if (always_read_ctf && ei.ctfsect)
    {
      elfctf_build_psymtabs (objfile);
    }

  bool has_dwarf2 = elf_symfile_read_dwarf2 (objfile, symfile_flags);

  /* Read the CTF section only if there is no DWARF info.  */
  if (!always_read_ctf && !has_dwarf2 && ei.ctfsect)
    {
      elfctf_build_psymtabs (objfile);
    }

  /* Copy relocations are used by some ABIs using the ELF format, so
     set the objfile flag indicating this fact.  */
  objfile->object_format_has_copy_relocs = true;
}

/* Initialize anything that needs initializing when a completely new symbol
   file is specified (not just adding some symbols from another file, e.g. a
   shared library).  */

static void
elf_new_init (struct objfile *ignore)
{
}

/* Perform any local cleanups required when we are done with a particular
   objfile.  I.E, we are in the process of discarding all symbol information
   for an objfile, freeing up all memory held for it, and unlinking the
   objfile struct from the global list of known objfiles.  */

static void
elf_symfile_finish (struct objfile *objfile)
{
}

/* ELF specific initialization routine for reading symbols.  */

static void
elf_symfile_init (struct objfile *objfile)
{
}

/* Implementation of `sym_get_probes', as documented in symfile.h.  */

static const elfread_data &
elf_get_probes (struct objfile *objfile)
{
  elfread_data *probes_per_bfd = probe_key.get (objfile->obfd.get ());

  if (probes_per_bfd == NULL)
    {
      probes_per_bfd = probe_key.emplace (objfile->obfd.get ());

      /* Here we try to gather information about all types of probes from the
	 objfile.  */
      for (const static_probe_ops *ops : all_static_probe_ops)
	ops->get_probes (probes_per_bfd, objfile);
    }

  return *probes_per_bfd;
}



/* Implementation `sym_probe_fns', as documented in symfile.h.  */

static const struct sym_probe_fns elf_probe_fns =
{
  elf_get_probes,		    /* sym_get_probes */
};

/* Register that we are able to handle ELF object file formats.  */

static const struct sym_fns elf_sym_fns =
{
  elf_new_init,			/* init anything gbl to entire symtab */
  elf_symfile_init,		/* read initial info, setup for sym_read() */
  elf_symfile_read,		/* read a symbol file into symtab */
  elf_symfile_finish,		/* finished with file, cleanup */
  default_symfile_offsets,	/* Translate ext. to int. relocation */
  elf_symfile_segments,		/* Get segment information from a file.  */
  NULL,
  default_symfile_relocate,	/* Relocate a debug section.  */
  &elf_probe_fns,		/* sym_probe_fns */
};

/* STT_GNU_IFUNC resolver vector to be installed to gnu_ifunc_fns_p.  */

static const struct gnu_ifunc_fns elf_gnu_ifunc_fns =
{
  elf_gnu_ifunc_resolve_addr,
  elf_gnu_ifunc_resolve_name,
  elf_gnu_ifunc_resolver_stop,
  elf_gnu_ifunc_resolver_return_stop
};

void _initialize_elfread ();
void
_initialize_elfread ()
{
  add_symtab_fns (bfd_target_elf_flavour, &elf_sym_fns);

  gnu_ifunc_fns_p = &elf_gnu_ifunc_fns;

  /* Add "set always-read-ctf on/off".  */
  add_setshow_boolean_cmd ("always-read-ctf", class_support, &always_read_ctf,
			   _("\
Set whether CTF is always read."),
			   _("\
Show whether CTF is always read."),
			   _("\
When off, CTF is only read if DWARF is not present.  When on, CTF is read\
 regardless of whether DWARF is present."),
			   nullptr /* set_func */, nullptr /* show_func */,
			   &setlist, &showlist);
}
