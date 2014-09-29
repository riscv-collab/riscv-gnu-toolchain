/* RISC-V-specific support for ELF
   Copyright 2011-2014 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
   Based on MIPS target.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */


/* This file handles functionality common to the different RISC-V ABIs.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "libiberty.h"
#include "elf-bfd.h"
#include "elfxx-riscv.h"
#include "elf/riscv.h"
#include "opcode/riscv.h"

#include "hashtab.h"
#include <stdint.h>

/* This structure is used to hold information about one GOT entry.
   There are three types of entry:

      (1) absolute addresses
	    (abfd == NULL)
      (2) SYMBOL + OFFSET addresses, where SYMBOL is local to an input bfd
	    (abfd != NULL, symndx >= 0)
      (3) SYMBOL addresses, where SYMBOL is not local to an input bfd
	    (abfd != NULL, symndx == -1)

   Type (3) entries are treated differently for different types of GOT.
   In the "master" GOT -- i.e.  the one that describes every GOT
   reference needed in the link -- the riscv_got_entry is keyed on both
   the symbol and the input bfd that references it.  If it turns out
   that we need multiple GOTs, we can then use this information to
   create separate GOTs for each input bfd.

   However, we want each of these separate GOTs to have at most one
   entry for a given symbol, so their type (3) entries are keyed only
   on the symbol.  The input bfd given by the "abfd" field is somewhat
   arbitrary in this case.

   This means that when there are multiple GOTs, each GOT has a unique
   riscv_got_entry for every symbol within it.  We can therefore use the
   riscv_got_entry fields (tls_type and gotidx) to track the symbol's
   GOT index.

   However, if it turns out that we need only a single GOT, we continue
   to use the master GOT to describe it.  There may therefore be several
   riscv_got_entries for the same symbol, each with a different input bfd.
   We want to make sure that each symbol gets a unique GOT entry, so when
   there's a single GOT, we use the symbol's hash entry, not the
   riscv_got_entry fields, to track a symbol's GOT index.  */
struct riscv_got_entry
{
  /* The input bfd in which the symbol is defined.  */
  bfd *abfd;
  /* The index of the symbol, as stored in the relocation r_info, if
     we have a local symbol; -1 otherwise.  */
  long symndx;
  union
  {
    /* If abfd == NULL, an address that must be stored in the got.  */
    bfd_vma address;
    /* If abfd != NULL && symndx != -1, the addend of the relocation
       that should be added to the symbol value.  */
    bfd_vma addend;
    /* If abfd != NULL && symndx == -1, the hash table entry
       corresponding to symbol in the GOT.  The symbol's entry
       is in the local area if h->global_got_area is GGA_NONE,
       otherwise it is in the global area.  */
    struct riscv_elf_link_hash_entry *h;
  } d;

  /* The TLS types included in this GOT entry (specifically, GD and IE). */
  unsigned char tls_type;

  /* The offset from the beginning of the .got section to the entry
     corresponding to this symbol+addend.  If it's a global symbol
     whose offset is yet to be decided, it's going to be -1.  */
  long gotidx;
};

/* This structure is used to hold .got information when linking.  */

struct riscv_got_info
{
  /* The global symbol in the GOT with the lowest index in the dynamic
     symbol table.  */
  struct elf_link_hash_entry *global_gotsym;
  /* The number of global .got entries.  */
  unsigned int global_gotno;
  /* The number of global .got entries that are in the GGA_RELOC_ONLY area.  */
  unsigned int reloc_only_gotno;
  /* The number of .got slots used for TLS.  */
  unsigned int tls_gotno;
  /* The first unused TLS .got entry.  Used only during
     riscv_elf_initialize_tls_index.  */
  unsigned int tls_assigned_gotno;
  /* The number of local .got entries, eventually including page entries.  */
  unsigned int local_gotno;
  /* The number of local .got entries we have used.  */
  unsigned int assigned_gotno;
  /* A hash table holding members of the got.  */
  struct htab *got_entries;
};

/* Another structure used to pass arguments for got entries traversal.  */

struct riscv_elf_set_global_got_offset_arg
{
  struct riscv_got_info *g;
  int value;
  unsigned int needed_relocs;
  struct bfd_link_info *info;
};

/* A structure used to count TLS relocations or GOT entries, for GOT
   entry or ELF symbol table traversal.  */

struct riscv_elf_count_tls_arg
{
  struct bfd_link_info *info;
  unsigned int needed;
};

struct _riscv_elf_section_data
{
  struct bfd_elf_section_data elf;
  union
  {
    bfd_byte *tdata;
  } u;
};

#define riscv_elf_section_data(sec) \
  ((struct _riscv_elf_section_data *) elf_section_data (sec))

#define is_riscv_elf(bfd)				\
  (bfd_get_flavour (bfd) == bfd_target_elf_flavour	\
   && elf_tdata (bfd) != NULL				\
   && elf_object_id (bfd) == RISCV_ELF_DATA)

/* The ABI says that every symbol used by dynamic relocations must have
   a global GOT entry.  Among other things, this provides the dynamic
   linker with a free, directly-indexed cache.  The GOT can therefore
   contain symbols that are not referenced by GOT relocations themselves.

   GOT relocations are less likely to overflow if we put the associated
   GOT entries towards the beginning.  We therefore divide the global
   GOT entries into two areas: "normal" and "reloc-only".  Entries in
   the first area can be used for both dynamic relocations and GP-relative
   accesses, while those in the "reloc-only" area are for dynamic
   relocations only.

   These GGA_* ("Global GOT Area") values are organised so that lower
   values are more general than higher values.  Also, non-GGA_NONE
   values are ordered by the position of the area in the GOT.  */
#define GGA_NORMAL 0
#define GGA_RELOC_ONLY 1
#define GGA_NONE 2

/* This structure is passed to riscv_elf_sort_hash_table_f when sorting
   the dynamic symbols.  */

struct riscv_elf_hash_sort_data
{
  /* The symbol in the global GOT with the lowest dynamic symbol table
     index.  */
  struct elf_link_hash_entry *low;
  /* The least dynamic symbol table index corresponding to a non-TLS
     symbol with a GOT entry.  */
  long min_got_dynindx;
  /* The greatest dynamic symbol table index corresponding to a symbol
     with a GOT entry that is not referenced (e.g., a dynamic symbol
     with dynamic relocations pointing to it from non-primary GOTs).  */
  long max_unref_got_dynindx;
  /* The greatest dynamic symbol table index not corresponding to a
     symbol without a GOT entry.  */
  long max_non_got_dynindx;
};

/* The RISC-V ELF linker needs additional information for each symbol in
   the global hash table.  */

struct riscv_elf_link_hash_entry
{
  struct elf_link_hash_entry root;

  /* Number of R_RISCV_32, R_RISCV_REL32, or R_RISCV_64 relocs against
     this symbol.  */
  unsigned int possibly_dynamic_relocs;

#define GOT_NORMAL	0
#define GOT_TLS_GD	1
#define GOT_TLS_IE	4
#define GOT_TLS_OFFSET_DONE    0x40
#define GOT_TLS_DONE    0x80
  unsigned char tls_type;

  /* This is only used in single-GOT mode; in multi-GOT mode there
     is one riscv_got_entry per GOT entry, so the offset is stored
     there.  In single-GOT mode there may be many riscv_got_entry
     structures all referring to the same GOT slot.  It might be
     possible to use root.got.offset instead, but that field is
     overloaded already.  */
  bfd_vma tls_got_offset;

  /* The highest GGA_* value that satisfies all references to this symbol.  */
  unsigned int global_got_area : 2;

  /* True if one of the relocations described by possibly_dynamic_relocs
     is against a readonly section.  */
  unsigned int readonly_reloc : 1;

  /* True if there is a relocation against this symbol that must be
     resolved by the static linker (in other words, if the relocation
     cannot possibly be made dynamic).  */
  unsigned int has_static_relocs : 1;
};

/* RISC-V ELF linker hash table.  */

struct riscv_elf_link_hash_table
{
  struct elf_link_hash_table root;

  /* Shortcuts to some dynamic sections, or NULL if they are not
     being used.  */
  asection *srelbss;
  asection *sdynbss;
  asection *srelplt;
  asection *srelplt2;
  asection *sgotplt;
  asection *splt;
  asection *sgot;

  /* The master GOT information.  */
  struct riscv_got_info *got_info;

  /* The number of PLT entries. */
  bfd_vma nplt;

  /* The number of reserved entries at the beginning of the GOT.  */
  unsigned int reserved_gotno;

  /* Whether or not relaxation is enabled. */
  bfd_boolean relax;
};

/* Get the RISC-V ELF linker hash table from a link_info structure.  */

#define riscv_elf_hash_table(p) \
  (elf_hash_table_id ((struct elf_link_hash_table *) ((p)->hash)) \
  == RISCV_ELF_DATA ? ((struct riscv_elf_link_hash_table *) ((p)->hash)) : NULL)

#define TLS_RELOC_P(r_type) \
  ((r_type) == R_RISCV_TLS_DTPMOD32		\
   || (r_type) == R_RISCV_TLS_DTPMOD64		\
   || (r_type) == R_RISCV_TLS_DTPREL32		\
   || (r_type) == R_RISCV_TLS_DTPREL64		\
   || (r_type) == R_RISCV_TLS_TPREL32		\
   || (r_type) == R_RISCV_TLS_TPREL64		\
   || (r_type) == R_RISCV_TPREL_HI20		\
   || (r_type) == R_RISCV_TPREL_LO12_I		\
   || (r_type) == R_RISCV_TPREL_LO12_S		\
   || (r_type) == R_RISCV_TPREL_ADD		\
   || TLS_GD_RELOC_P(r_type)			\
   || TLS_GOTTPREL_RELOC_P(r_type))

#define TLS_GOTTPREL_RELOC_P(r_type) \
  ((r_type) == R_RISCV_TLS_IE_HI20		\
   || (r_type) == R_RISCV_TLS_IE_LO12		\
   || (r_type) == R_RISCV_TLS_IE_ADD		\
   || (r_type) == R_RISCV_TLS_IE_LO12_I		\
   || (r_type) == R_RISCV_TLS_IE_LO12_S		\
   || (r_type) == R_RISCV_TLS_GOT_HI20		\
   || (r_type) == R_RISCV_TLS_GOT_LO12)

#define TLS_GD_RELOC_P(r_type) \
  ((r_type) == R_RISCV_TLS_GD_HI20		\
   || (r_type) == R_RISCV_TLS_GD_LO12)

/* The structure of the runtime procedure descriptor created by the
   loader for use by the static exception system.  */

typedef struct runtime_pdr {
	bfd_vma	adr;		/* Memory address of start of procedure.  */
	long	regmask;	/* Save register mask.  */
	long	regoffset;	/* Save register offset.  */
	long	fregmask;	/* Save floating point register mask.  */
	long	fregoffset;	/* Save floating point register offset.  */
	long	frameoffset;	/* Frame size.  */
	short	framereg;	/* Frame pointer register.  */
	short	pcreg;		/* Offset or reg of return pc.  */
	long	irpss;		/* Index into the runtime string table.  */
	long	reserved;
	struct exception_info *exception_info;/* Pointer to exception array.  */
} RPDR, *pRPDR;
#define cbRPDR sizeof (RPDR)
#define rpdNil ((pRPDR) 0)


static struct riscv_got_entry *riscv_elf_create_local_got_entry
  (bfd *, struct bfd_link_info *, bfd *, bfd_vma, unsigned long,
   struct riscv_elf_link_hash_entry *, int);
static bfd_boolean riscv_elf_sort_hash_table_f
  (struct riscv_elf_link_hash_entry *, void *);
static bfd_boolean riscv_elf_create_dynamic_relocation
  (bfd *, struct bfd_link_info *, const Elf_Internal_Rela *,
   struct riscv_elf_link_hash_entry *, asection *, bfd_vma,
   bfd_vma *, asection *);
static hashval_t riscv_elf_got_entry_hash
  (const void *);

/* This will be used when we sort the dynamic relocation records.  */
static bfd *reldyn_sorting_bfd;

/* Nonzero if ABFD is using the RV64 ABI.  */
#define ABI_64_P(abfd) \
  (get_elf_backend_data (abfd)->s->elfclass == ELFCLASS64)

/* Nonzero if ABFD is using the RV32 ABI.  */
#define ABI_32_P(abfd) (!ABI_64_P(abfd))

/* Whether the section is readonly.  */
#define RISCV_ELF_READONLY_SECTION(sec) \
  ((sec->flags & (SEC_ALLOC | SEC_LOAD | SEC_READONLY))		\
   == (SEC_ALLOC | SEC_LOAD | SEC_READONLY))

/* The size of an external REL relocation.  */
#define RISCV_ELF_REL_SIZE(abfd) \
  (get_elf_backend_data (abfd)->s->sizeof_rel)

/* The size of an external dynamic table entry.  */
#define RISCV_ELF_DYN_SIZE(abfd) \
  (get_elf_backend_data (abfd)->s->sizeof_dyn)

/* The size of a GOT entry.  */
#define RISCV_ELF_GOT_SIZE(abfd) \
  (get_elf_backend_data (abfd)->s->arch_size / 8)

/* The size of a symbol-table entry.  */
#define RISCV_ELF_SYM_SIZE(abfd) \
  (get_elf_backend_data (abfd)->s->sizeof_sym)

/* The default alignment for sections, as a power of two.  */
#define RISCV_ELF_LOG_FILE_ALIGN(abfd)				\
  (get_elf_backend_data (abfd)->s->log_file_align)

/* Get word-sized data.  */
#define RISCV_ELF_GET_WORD(abfd, ptr) \
  (ABI_64_P (abfd) ? bfd_get_64 (abfd, ptr) : bfd_get_32 (abfd, ptr))

/* Put out word-sized data.  */
#define RISCV_ELF_PUT_WORD(abfd, val, ptr)	\
  (ABI_64_P (abfd) 				\
   ? bfd_put_64 (abfd, val, ptr) 		\
   : bfd_put_32 (abfd, val, ptr))

/* The name of the dynamic relocation section.  */
#define RISCV_ELF_REL_DYN_NAME(INFO) ".rel.dyn"

/* In case we're on a 32-bit machine, construct a 64-bit "-1" value
   from smaller values.  Start with zero, widen, *then* decrement.  */
#define MINUS_ONE	(((bfd_vma)0) - 1)
#define MINUS_TWO	(((bfd_vma)0) - 2)

/* The name of the dynamic interpreter.  This is put in the .interp
   section.  */

#define ELF_DYNAMIC_INTERPRETER(abfd) 		\
   (ABI_64_P (abfd) ? "/lib/ld.so.1" 	\
    : "/lib32/ld.so.1")

#ifdef BFD64
#define MNAME(bfd,pre,pos) \
  (ABI_64_P (bfd) ? CONCAT4 (pre,64,_,pos) : CONCAT4 (pre,32,_,pos))
#define ELF_R_SYM(bfd, i)					\
  (ABI_64_P (bfd) ? ELF64_R_SYM (i) : ELF32_R_SYM (i))
#define ELF_R_TYPE(bfd, i)					\
  (ABI_64_P (bfd) ? ELF64_R_TYPE (i) : ELF32_R_TYPE (i))
#define ELF_R_INFO(bfd, s, t)					\
  (ABI_64_P (bfd) ? ELF64_R_INFO (s, t) : ELF32_R_INFO (s, t))
#else
#define MNAME(bfd,pre,pos) CONCAT4 (pre,32,_,pos)
#define ELF_R_SYM(bfd, i)					\
  (ELF32_R_SYM (i))
#define ELF_R_TYPE(bfd, i)					\
  (ELF32_R_TYPE (i))
#define ELF_R_INFO(bfd, s, t)					\
  (ELF32_R_INFO (s, t))
#endif

#define MATCH_LREG(abfd) (ABI_64_P(abfd) ? MATCH_LD : MATCH_LW)
#define MATCH_SREG(abfd) (ABI_64_P(abfd) ? MATCH_SD : MATCH_SW)

#define OPCODE_MATCHES(OPCODE, OP) \
  (((OPCODE) & MASK_##OP) == MATCH_##OP)

#define MINUS_ONE	(((bfd_vma)0) - 1)

/* The relocation table used for SHT_RELA sections.  */

static reloc_howto_type howto_table[] =
{
  /* No relocation.  */
  HOWTO (R_RISCV_NONE,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_NONE",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  EMPTY_HOWTO (1),

  /* 32 bit relocation.  */
  HOWTO (R_RISCV_32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_32",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 32 bit symbol relative relocation.  */
  HOWTO (R_RISCV_REL32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_REL32",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 26 bit jump address.  */
  HOWTO (R_RISCV_JAL,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
				/* This needs complex overflow
				   detection, because the upper 36
				   bits must match the PC + 4.  */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_JAL",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UJTYPE_IMM(-1U),	/* dst_mask */
	 TRUE),		/* pcrel_offset */

  /* High 16 bits of symbol value.  */
  HOWTO (R_RISCV_HI20,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_HI20",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Low 12 bits of symbol value.  */
  HOWTO (R_RISCV_LO12_I,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_LO12_I",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U) | (OP_MASK_RS1 << OP_SH_RS1),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Low 12 bits of symbol value.  */
  HOWTO (R_RISCV_LO12_S,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_LO12_S",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_STYPE_IMM(-1U) | (OP_MASK_RS1 << OP_SH_RS1),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Distance between AUIPC and corresponding ADD/load.  */
  HOWTO (R_RISCV_PCREL_LO12_I,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_PCREL_LO12_I",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Distance between AUIPC and corresponding store.  */
  HOWTO (R_RISCV_PCREL_LO12_S,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_PCREL_LO12_S",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_STYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_BRANCH,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_BRANCH",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_SBTYPE_IMM(-1U),	/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_RISCV_CALL,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_CALL",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_RISCV_PCREL_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_PCREL_HI20",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 TRUE),			/* pcrel_offset */

  HOWTO (R_RISCV_CALL_PLT,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_CALL_PLT",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 TRUE),			/* pcrel_offset */

  EMPTY_HOWTO (14),
  EMPTY_HOWTO (15),
  EMPTY_HOWTO (16),
  EMPTY_HOWTO (17),

  /* 64 bit relocation.  */
  HOWTO (R_RISCV_64,		/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_64",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  EMPTY_HOWTO (19),
  EMPTY_HOWTO (20),
  EMPTY_HOWTO (21),

  /* High 16 bits of displacement in global offset table.  */
  HOWTO (R_RISCV_GOT_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_GOT_HI20",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Low 16 bits of displacement in global offset table.  */
  HOWTO (R_RISCV_GOT_LO12,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_GOT_LO12",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_COPY,		/* type */
	 0,			/* rightshift */
	 0,			/* this one is variable size */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_COPY",		/* name */
	 FALSE,			/* partial_inplace */
	 0x0,         		/* src_mask */
	 0x0,		        /* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_JUMP_SLOT,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_JUMP_SLOT",	/* name */
	 FALSE,			/* partial_inplace */
	 0x0,         		/* src_mask */
	 0x0,		        /* dst_mask */
	 FALSE),		/* pcrel_offset */

  EMPTY_HOWTO (26),
  EMPTY_HOWTO (27),
  EMPTY_HOWTO (28),

  /* TLS IE GOT access in non-PIC code.  */
  HOWTO (R_RISCV_TLS_IE_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_IE_HI20",	/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS IE GOT access in non-PIC code.  */
  HOWTO (R_RISCV_TLS_IE_LO12,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_IE_LO12_I",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS IE thread pointer usage.  */
  HOWTO (R_RISCV_TLS_IE_ADD,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_IE_ADD",	/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS IE low-part relocation for relaxation.  */
  HOWTO (R_RISCV_TLS_IE_LO12_I,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_IE_LO12_I",/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS IE low-part relocation for relaxation.  */
  HOWTO (R_RISCV_TLS_IE_LO12_S,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_IE_LO12_S",/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS LE thread pointer offset.  */
  HOWTO (R_RISCV_TPREL_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TPREL_HI20",	/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS LE thread pointer offset.  */
  HOWTO (R_RISCV_TPREL_LO12_I,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TPREL_LO12_I",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U) | (OP_MASK_RS1 << OP_SH_RS1),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS LE thread pointer offset.  */
  HOWTO (R_RISCV_TPREL_LO12_S,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TPREL_LO12_S",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_STYPE_IMM(-1U) | (OP_MASK_RS1 << OP_SH_RS1),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS LE thread pointer usage.  */
  HOWTO (R_RISCV_TPREL_ADD,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TPREL_ADD",	/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS relocations.  */
  HOWTO (R_RISCV_TLS_DTPMOD32,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_DTPMOD32", /* name */
	 FALSE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPREL32,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_DTPREL32",	/* name */
	 TRUE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPMOD64,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_DTPMOD64", /* name */
	 FALSE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPREL64,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_DTPREL64",	/* name */
	 TRUE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  EMPTY_HOWTO (42),
  EMPTY_HOWTO (43),
  EMPTY_HOWTO (44),
  EMPTY_HOWTO (45),
  EMPTY_HOWTO (46),

  HOWTO (R_RISCV_TLS_TPREL32,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_TPREL32",	/* name */
	 FALSE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_TLS_TPREL64,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_TPREL64",	/* name */
	 FALSE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  EMPTY_HOWTO (49),

  /* Distance between AUIPC and corresponding ADD/load.  */
  HOWTO (R_RISCV_TLS_PCREL_LO12,/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_TLS_PCREL_LO12",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* High 16 bits of displacement in global offset table.  */
  HOWTO (R_RISCV_TLS_GOT_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_TLS_GOT_HI20",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Low 16 bits of displacement in global offset table.  */
  HOWTO (R_RISCV_TLS_GOT_LO12,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_TLS_GOT_LO12",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* High 16 bits of displacement in global offset table.  */
  HOWTO (R_RISCV_TLS_GD_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_TLS_GD_HI20",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Low 16 bits of displacement in global offset table.  */
  HOWTO (R_RISCV_TLS_GD_LO12,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 RISCV_IMM_BITS,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_TLS_GD_LO12",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  EMPTY_HOWTO (55),
  EMPTY_HOWTO (56),

  /* 32 bit relocation with no addend.  */
  HOWTO (R_RISCV_GLOB_DAT,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc, /* special_function */
	 "R_RISCV_GLOB_DAT",	/* name */
	 FALSE,			/* partial_inplace */
	 0x0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 32 bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_ADD32",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 64 bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD64,		/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_ADD64",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 32 bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_SUB32",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 64 bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB64,		/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 _bfd_riscv_elf_generic_reloc,	/* special_function */
	 "R_RISCV_SUB64",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */
};

/* A mapping from BFD reloc types to RISC-V ELF reloc types.  */

struct elf_reloc_map {
  bfd_reloc_code_real_type bfd_val;
  enum elf_riscv_reloc_type elf_val;
};

static const struct elf_reloc_map riscv_reloc_map[] =
{
  { BFD_RELOC_NONE, R_RISCV_NONE },
  { BFD_RELOC_32, R_RISCV_32 },
  { BFD_RELOC_64, R_RISCV_64 },
  { BFD_RELOC_RISCV_ADD32, R_RISCV_ADD32 },
  { BFD_RELOC_RISCV_ADD64, R_RISCV_ADD64 },
  { BFD_RELOC_RISCV_SUB32, R_RISCV_SUB32 },
  { BFD_RELOC_RISCV_SUB64, R_RISCV_SUB64 },
  { BFD_RELOC_CTOR, R_RISCV_64 },
  { BFD_RELOC_12_PCREL, R_RISCV_BRANCH },
  { BFD_RELOC_RISCV_HI20, R_RISCV_HI20 },
  { BFD_RELOC_RISCV_LO12_I, R_RISCV_LO12_I },
  { BFD_RELOC_RISCV_LO12_S, R_RISCV_LO12_S },
  { BFD_RELOC_RISCV_PCREL_LO12_I, R_RISCV_PCREL_LO12_I },
  { BFD_RELOC_RISCV_PCREL_LO12_S, R_RISCV_PCREL_LO12_S },
  { BFD_RELOC_RISCV_CALL, R_RISCV_CALL },
  { BFD_RELOC_RISCV_CALL_PLT, R_RISCV_CALL_PLT },
  { BFD_RELOC_RISCV_PCREL_HI20, R_RISCV_PCREL_HI20 },
  { BFD_RELOC_RISCV_JMP, R_RISCV_JAL },
  { BFD_RELOC_RISCV_GOT_HI20, R_RISCV_GOT_HI20 },
  { BFD_RELOC_RISCV_GOT_LO12, R_RISCV_GOT_LO12 },
  { BFD_RELOC_RISCV_TLS_DTPMOD32, R_RISCV_TLS_DTPMOD32 },
  { BFD_RELOC_RISCV_TLS_DTPREL32, R_RISCV_TLS_DTPREL32 },
  { BFD_RELOC_RISCV_TLS_DTPMOD64, R_RISCV_TLS_DTPMOD64 },
  { BFD_RELOC_RISCV_TLS_DTPREL64, R_RISCV_TLS_DTPREL64 },
  { BFD_RELOC_RISCV_TLS_TPREL32, R_RISCV_TLS_TPREL32 },
  { BFD_RELOC_RISCV_TLS_TPREL64, R_RISCV_TLS_TPREL64 },
  { BFD_RELOC_RISCV_TPREL_HI20, R_RISCV_TPREL_HI20 },
  { BFD_RELOC_RISCV_TPREL_ADD, R_RISCV_TPREL_ADD },
  { BFD_RELOC_RISCV_TPREL_LO12_S, R_RISCV_TPREL_LO12_S },
  { BFD_RELOC_RISCV_TPREL_LO12_I, R_RISCV_TPREL_LO12_I },
  { BFD_RELOC_RISCV_TLS_IE_HI20, R_RISCV_TLS_IE_HI20 },
  { BFD_RELOC_RISCV_TLS_IE_LO12, R_RISCV_TLS_IE_LO12 },
  { BFD_RELOC_RISCV_TLS_IE_ADD, R_RISCV_TLS_IE_ADD },
  { BFD_RELOC_RISCV_TLS_IE_LO12_S, R_RISCV_TLS_IE_LO12_S },
  { BFD_RELOC_RISCV_TLS_IE_LO12_I, R_RISCV_TLS_IE_LO12_I },
  { BFD_RELOC_RISCV_TLS_GOT_HI20, R_RISCV_TLS_GOT_HI20 },
  { BFD_RELOC_RISCV_TLS_GOT_LO12, R_RISCV_TLS_GOT_LO12 },
  { BFD_RELOC_RISCV_TLS_GD_HI20, R_RISCV_TLS_GD_HI20 },
  { BFD_RELOC_RISCV_TLS_GD_LO12, R_RISCV_TLS_GD_LO12 },
  { BFD_RELOC_RISCV_TLS_PCREL_LO12, R_RISCV_TLS_PCREL_LO12 },
};

/* Given a BFD reloc type, return a howto structure.  */

reloc_howto_type *
riscv_elf_bfd_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				 bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = 0; i < sizeof (riscv_reloc_map) / sizeof (riscv_reloc_map[0]); i++)
    if (riscv_reloc_map[i].bfd_val == code)
      return &howto_table[(int) riscv_reloc_map[i].elf_val];

  bfd_set_error (bfd_error_bad_value);
  return NULL;
}

reloc_howto_type *
riscv_elf_bfd_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				 const char *r_name)
{
  unsigned int i;

  for (i = 0; i < sizeof (howto_table) / sizeof (howto_table[0]); i++)
    if (howto_table[i].name && strcasecmp (howto_table[i].name, r_name) == 0)
      return &howto_table[i];

  return NULL;
}

static reloc_howto_type *
riscv_elf_rtype_to_howto (unsigned int r_type)
{
  BFD_ASSERT (r_type < (unsigned int) R_RISCV_max);
  return &howto_table[r_type];
}

void
riscv_elf_info_to_howto_rela (bfd *abfd, arelent *cache_ptr,
			      Elf_Internal_Rela *dst ATTRIBUTE_UNUSED)
{
  unsigned int r_type;

  r_type = ELF_R_TYPE (abfd, dst->r_info);
  cache_ptr->howto = riscv_elf_rtype_to_howto (r_type);
  cache_ptr->addend = dst->r_addend;
}

#define sec_addr(sec) ((sec)->output_section->vma + (sec)->output_offset)

static bfd_vma
riscv_elf_got_plt_val (bfd_vma plt_index, struct bfd_link_info *info)
{
  struct riscv_elf_link_hash_table *htab = riscv_elf_hash_table (info);
  return sec_addr(htab->sgotplt)
	 + (2+plt_index) * RISCV_ELF_GOT_SIZE (elf_hash_table (info)->dynobj);
}

#define PLT_HEADER_INSNS 8
#define PLT_ENTRY_INSNS 4
#define PLT_HEADER_SIZE (PLT_HEADER_INSNS * 4)
#define PLT_ENTRY_SIZE (PLT_ENTRY_INSNS * 4)

#define X_V0 16
#define X_V1 17
#define X_T0 26
#define X_T1 27
#define X_T2 28

/* The format of the first PLT entry.  */

static void
riscv_make_plt0_entry(bfd* abfd, bfd_vma gotplt_addr, bfd_vma addr,
                      uint32_t entry[PLT_HEADER_INSNS])
{
  int regbytes = ABI_64_P(abfd) ? 8 : 4;

  /* auipc  t2, %hi(.got.plt)
     sub    v0, v0, v1               # shifted .got.plt offset + hdr size + 12
     l[w|d] v1, %lo(.got.plt)(t2)    # _dl_runtime_resolve
     addi   v0, v0, -(hdr size + 12) # shifted .got.plt offset
     addi   t0, t2, %lo(.got.plt)    # &.got.plt
     srli   t1, v0, log2(16/PTRSIZE) # .got.plt offset
     l[w|d] t0, PTRSIZE(t0)          # link map
     jr     v1 */

  entry[0] = RISCV_UTYPE(AUIPC, X_T2, RISCV_PCREL_HIGH_PART(gotplt_addr, addr));
  entry[1] = RISCV_RTYPE(SUB, X_V0, X_V0, X_V1);
  entry[2] = RISCV_ITYPE(LREG(abfd), X_V1, X_T2, RISCV_PCREL_LOW_PART(gotplt_addr, addr));
  entry[3] = RISCV_ITYPE(ADDI, X_V0, X_V0, -(PLT_HEADER_SIZE + 12));
  entry[4] = RISCV_ITYPE(ADDI, X_T0, X_T2, RISCV_PCREL_LOW_PART(gotplt_addr, addr));
  entry[5] = RISCV_ITYPE(SRLI, X_T1, X_V0, regbytes == 4 ? 2 : 1);
  entry[6] = RISCV_ITYPE(LREG(abfd), X_T0, X_T0, regbytes);
  entry[7] = RISCV_ITYPE(JALR, 0, X_V1, 0);
}

/* The format of subsequent PLT entries.  */

static bfd_vma
riscv_make_plt_entry(bfd* abfd, bfd_vma got_address, bfd_vma plt0_addr,
		     bfd_vma addr, uint32_t *entry)
{
  /* auipc  v0, %hi(.got.plt entry)
     l[w|d] v1, %lo(.got.plt entry)(v0)
     jalr   v0, v1
     nop */

  entry[0] = RISCV_UTYPE(AUIPC, X_V0, RISCV_PCREL_HIGH_PART(got_address, addr));
  entry[1] = RISCV_ITYPE(LREG(abfd),  X_V1, X_V0, RISCV_PCREL_LOW_PART(got_address, addr));
  entry[2] = RISCV_ITYPE(JALR, X_V0, X_V1, 0);
  entry[3] = RISCV_NOP;
  return plt0_addr;
}

/* Look up an entry in a RISC-V ELF linker hash table.  */

#define riscv_elf_link_hash_lookup(table, string, create, copy, follow)	\
  ((struct riscv_elf_link_hash_entry *)					\
   elf_link_hash_lookup (&(table)->root, (string), (create),		\
			 (copy), (follow)))

/* Traverse a RISC-V ELF linker hash table.  */

#define riscv_elf_link_hash_traverse(table, func, info)			\
  (elf_link_hash_traverse						\
   (&(table)->root,							\
    (bfd_boolean (*) (struct elf_link_hash_entry *, void *)) (func),	\
    (info)))

/* Find the base offsets for thread-local storage in this object,
   for GD/LD and IE/LE respectively.  */

#define TP_OFFSET 0
#define DTP_OFFSET 0x800

static bfd_vma
dtprel_base (struct bfd_link_info *info)
{
  /* If tls_sec is NULL, we should have signalled an error already.  */
  if (elf_hash_table (info)->tls_sec == NULL)
    return 0;
  return elf_hash_table (info)->tls_sec->vma + DTP_OFFSET;
}

static bfd_vma
tprel_base (struct bfd_link_info *info)
{
  /* If tls_sec is NULL, we should have signalled an error already.  */
  if (elf_hash_table (info)->tls_sec == NULL)
    return 0;
  return elf_hash_table (info)->tls_sec->vma + TP_OFFSET;
}

/* Create an entry in a RISC-V ELF linker hash table.  */

static struct bfd_hash_entry *
riscv_elf_link_hash_newfunc (struct bfd_hash_entry *entry,
			    struct bfd_hash_table *table, const char *string)
{
  struct riscv_elf_link_hash_entry *ret =
    (struct riscv_elf_link_hash_entry *) entry;

  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (ret == NULL)
    ret = bfd_hash_allocate (table, sizeof (struct riscv_elf_link_hash_entry));
  if (ret == NULL)
    return (struct bfd_hash_entry *) ret;

  /* Call the allocation method of the superclass.  */
  ret = ((struct riscv_elf_link_hash_entry *)
	 _bfd_elf_link_hash_newfunc ((struct bfd_hash_entry *) ret,
				     table, string));
  if (ret != NULL)
    {
      ret->possibly_dynamic_relocs = 0;
      ret->tls_type = GOT_NORMAL;
      ret->global_got_area = GGA_NONE;
      ret->readonly_reloc = FALSE;
      ret->has_static_relocs = FALSE;
    }

  return (struct bfd_hash_entry *) ret;
}

bfd_boolean
_bfd_riscv_elf_new_section_hook (bfd *abfd, asection *sec)
{
  if (!sec->used_by_bfd)
    {
      struct _riscv_elf_section_data *sdata;
      bfd_size_type amt = sizeof (*sdata);

      sdata = bfd_zalloc (abfd, amt);
      if (sdata == NULL)
	return FALSE;
      sec->used_by_bfd = sdata;
    }

  return _bfd_elf_new_section_hook (abfd, sec);
}

/* A generic howto special_function.  This calculates and installs the
   relocation itself, thus avoiding the oft-discussed problems in
   bfd_perform_relocation and bfd_install_relocation.  */

bfd_reloc_status_type
_bfd_riscv_elf_generic_reloc (bfd *abfd ATTRIBUTE_UNUSED, arelent *reloc_entry,
			     asymbol *symbol, void *data ATTRIBUTE_UNUSED,
			     asection *input_section, bfd *output_bfd,
			     char **error_message ATTRIBUTE_UNUSED)
{
  bfd_signed_vma val;
  bfd_reloc_status_type status;
  bfd_boolean relocatable;

  relocatable = (output_bfd != NULL);

  if (reloc_entry->address > bfd_get_section_limit (abfd, input_section))
    return bfd_reloc_outofrange;

  /* Build up the field adjustment in VAL.  */
  val = 0;
  if (!relocatable || (symbol->flags & BSF_SECTION_SYM) != 0)
    /* Either we're calculating the final field value or we have a
       relocation against a section symbol.  Add in the section's
       offset or address.  */
      val += sec_addr(symbol->section);

  if (!relocatable)
    {
      /* We're calculating the final field value.  Add in the symbol's value
	 and, if pc-relative, subtract the address of the field itself.  */
      val += symbol->value;
      if (reloc_entry->howto->pc_relative)
	{
	  val -= sec_addr(input_section);
	  val -= reloc_entry->address;
	}
    }

  /* VAL is now the final adjustment.  If we're keeping this relocation
     in the output file, and if the relocation uses a separate addend,
     we just need to add VAL to that addend.  Otherwise we need to add
     VAL to the relocation field itself.  */
  if (relocatable && !reloc_entry->howto->partial_inplace)
    reloc_entry->addend += val;
  else
    {
      bfd_byte *loc = (bfd_byte *) data + reloc_entry->address;
      struct reloc_howto_struct howto = *reloc_entry->howto;

      /* Add in the separate addend, if any.  */
      val += reloc_entry->addend;

      /* Add VAL to the reloc field.  */
      status = _bfd_relocate_contents (&howto, abfd, val, loc);

      if (status != bfd_reloc_ok)
	return status;
    }

  if (relocatable)
    reloc_entry->address += input_section->output_offset;

  return bfd_reloc_ok;
}

/* This function is called via qsort() to sort the dynamic relocation
   entries by increasing r_symndx value.  */

static int
sort_dynamic_relocs (const void *arg1, const void *arg2)
{
  Elf_Internal_Rela int_reloc1;
  Elf_Internal_Rela int_reloc2;
  int diff;

  bfd_elf32_swap_reloc_in (reldyn_sorting_bfd, arg1, &int_reloc1);
  bfd_elf32_swap_reloc_in (reldyn_sorting_bfd, arg2, &int_reloc2);

  diff = ELF32_R_SYM (int_reloc1.r_info) - ELF32_R_SYM (int_reloc2.r_info);
  if (diff != 0)
    return diff;

  if (int_reloc1.r_offset < int_reloc2.r_offset)
    return -1;
  if (int_reloc1.r_offset > int_reloc2.r_offset)
    return 1;
  return 0;
}

/* Like sort_dynamic_relocs, but used for elf64 relocations.  */

static int
sort_dynamic_relocs_64 (const void *arg1 ATTRIBUTE_UNUSED,
			const void *arg2 ATTRIBUTE_UNUSED)
{
#ifdef BFD64
  Elf_Internal_Rela int_reloc1;
  Elf_Internal_Rela int_reloc2;

  (*get_elf_backend_data (reldyn_sorting_bfd)->s->swap_reloc_in)
    (reldyn_sorting_bfd, arg1, &int_reloc1);
  (*get_elf_backend_data (reldyn_sorting_bfd)->s->swap_reloc_in)
    (reldyn_sorting_bfd, arg2, &int_reloc2);

  if (ELF64_R_SYM (int_reloc1.r_info) < ELF64_R_SYM (int_reloc2.r_info))
    return -1;
  if (ELF64_R_SYM (int_reloc1.r_info) > ELF64_R_SYM (int_reloc2.r_info))
    return 1;

  if (int_reloc1.r_offset < int_reloc2.r_offset)
    return -1;
  if (int_reloc1.r_offset > int_reloc2.r_offset)
    return 1;
  return 0;
#else
  abort ();
#endif
}

/* Functions to manage the got entry hash table.  */

/* Use all 64 bits of a bfd_vma for the computation of a 32-bit
   hash number.  */

static INLINE hashval_t
riscv_elf_hash_bfd_vma (bfd_vma addr)
{
#ifdef BFD64
  return addr + (addr >> 32);
#else
  return addr;
#endif
}

/* got_entries only match if they're identical, except for gotidx, so
   use all fields to compute the hash, and compare the appropriate
   union members.  */

static hashval_t
riscv_elf_got_entry_hash (const void *entry_)
{
  const struct riscv_got_entry *entry = (struct riscv_got_entry *)entry_;

  return entry->symndx
    + (! entry->abfd ? riscv_elf_hash_bfd_vma (entry->d.address)
       : entry->abfd->id
         + (entry->symndx >= 0 ? riscv_elf_hash_bfd_vma (entry->d.addend)
	    : entry->d.h->root.root.root.hash));
}

static int
riscv_elf_got_entry_eq (const void *entry1, const void *entry2)
{
  const struct riscv_got_entry *e1 = (struct riscv_got_entry *)entry1;
  const struct riscv_got_entry *e2 = (struct riscv_got_entry *)entry2;

  return e1->abfd == e2->abfd && e1->symndx == e2->symndx
    && (! e1->abfd ? e1->d.address == e2->d.address
	: e1->symndx >= 0 ? e1->d.addend == e2->d.addend
	: e1->d.h == e2->d.h);
}

/* Return the dynamic relocation section.  If it doesn't exist, try to
   create a new it if CREATE_P, otherwise return NULL.  Also return NULL
   if creation fails.  */

static asection *
riscv_elf_rel_dyn_section (struct bfd_link_info *info, bfd_boolean create_p)
{
  const char *dname;
  asection *sreloc;
  bfd *dynobj;

  dname = RISCV_ELF_REL_DYN_NAME (info);
  dynobj = elf_hash_table (info)->dynobj;
  sreloc = bfd_get_section_by_name (dynobj, dname);
  if (sreloc == NULL && create_p)
    {
      sreloc = bfd_make_section_with_flags (dynobj, dname,
					    (SEC_ALLOC
					     | SEC_LOAD
					     | SEC_HAS_CONTENTS
					     | SEC_IN_MEMORY
					     | SEC_LINKER_CREATED
					     | SEC_READONLY));
      if (sreloc == NULL
	  || ! bfd_set_section_alignment (dynobj, sreloc,
					  RISCV_ELF_LOG_FILE_ALIGN (dynobj)))
	return NULL;
    }
  return sreloc;
}

/* Count the number of relocations needed for a TLS GOT entry, with
   access types from TLS_TYPE, and symbol H (or a local symbol if H
   is NULL).  */

static int
riscv_tls_got_relocs (struct bfd_link_info *info, unsigned char tls_type,
		      struct elf_link_hash_entry *h)
{
  int indx = 0;
  int ret = 0;
  bfd_boolean need_relocs = FALSE;
  bfd_boolean dyn = elf_hash_table (info)->dynamic_sections_created;

  if (h && WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn, info->shared, h)
      && (!info->shared || !SYMBOL_REFERENCES_LOCAL (info, h)))
    indx = h->dynindx;

  if ((info->shared || indx != 0)
      && (h == NULL
	  || ELF_ST_VISIBILITY (h->other) == STV_DEFAULT
	  || h->root.type != bfd_link_hash_undefweak))
    need_relocs = TRUE;

  if (!need_relocs)
    return FALSE;

  if (tls_type & GOT_TLS_GD)
    {
      ret++;
      if (indx != 0)
	ret++;
    }

  if (tls_type & GOT_TLS_IE)
    ret++;

  return ret;
}

/* Count the number of TLS relocations required for the GOT entry in
   ARG1, if it describes a local symbol.  */

static int
riscv_elf_count_local_tls_relocs (void **arg1, void *arg2)
{
  struct riscv_got_entry *entry = * (struct riscv_got_entry **) arg1;
  struct riscv_elf_count_tls_arg *arg = arg2;

  if (entry->abfd != NULL && entry->symndx != -1)
    arg->needed += riscv_tls_got_relocs (arg->info, entry->tls_type, NULL);

  return 1;
}

/* Count the number of TLS GOT entries required for the global (or
   forced-local) symbol in ARG1.  */

static int
riscv_elf_count_global_tls_entries (void *arg1, void *arg2)
{
  struct riscv_elf_link_hash_entry *hm
    = (struct riscv_elf_link_hash_entry *) arg1;
  struct riscv_elf_count_tls_arg *arg = arg2;

  if (hm->tls_type & GOT_TLS_GD)
    arg->needed += 2;
  if (hm->tls_type & GOT_TLS_IE)
    arg->needed += 1;

  return 1;
}

/* Count the number of TLS relocations required for the global (or
   forced-local) symbol in ARG1.  */

static int
riscv_elf_count_global_tls_relocs (void *arg1, void *arg2)
{
  struct riscv_elf_link_hash_entry *hm
    = (struct riscv_elf_link_hash_entry *) arg1;
  struct riscv_elf_count_tls_arg *arg = arg2;

  arg->needed += riscv_tls_got_relocs (arg->info, hm->tls_type, &hm->root);

  return 1;
}

/* Output a simple dynamic relocation into SRELOC.  */

static void
riscv_elf_output_dynamic_relocation (bfd *output_bfd,
				    asection *sreloc,
				    unsigned long reloc_index,
				    unsigned long indx,
				    int r_type,
				    bfd_vma offset)
{
  Elf_Internal_Rela rel;

  memset (&rel, 0, sizeof (rel));

  rel.r_info = ELF_R_INFO (output_bfd, indx, r_type);
  rel.r_offset = offset;

  if (ABI_64_P (output_bfd))
    bfd_elf64_swap_reloc_out
      (output_bfd, &rel,
       (sreloc->contents + reloc_index * sizeof (Elf64_External_Rel)));
  else
    bfd_elf32_swap_reloc_out
      (output_bfd, &rel,
       (sreloc->contents + reloc_index * sizeof (Elf32_External_Rel)));
}

/* Initialize a set of TLS GOT entries for one symbol.  */

static void
riscv_elf_initialize_tls_slots (bfd *abfd, bfd_vma got_offset,
			       unsigned char *tls_type_p,
			       struct bfd_link_info *info,
			       struct riscv_elf_link_hash_entry *h,
			       bfd_vma value)
{
  struct riscv_elf_link_hash_table *htab;
  int indx;
  asection *sreloc, *sgot;
  bfd_vma offset, offset2;
  bfd_boolean need_relocs = FALSE;

  htab = riscv_elf_hash_table (info);
  if (htab == NULL)
    return;

  sgot = htab->sgot;

  indx = 0;
  if (h != NULL)
    {
      bfd_boolean dyn = elf_hash_table (info)->dynamic_sections_created;

      if (WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn, info->shared, &h->root)
	  && (!info->shared || !SYMBOL_REFERENCES_LOCAL (info, &h->root)))
	indx = h->root.dynindx;
    }

  if (*tls_type_p & GOT_TLS_DONE)
    return;

  if ((info->shared || indx != 0)
      && (h == NULL
	  || ELF_ST_VISIBILITY (h->root.other) == STV_DEFAULT
	  || h->root.type != bfd_link_hash_undefweak))
    need_relocs = TRUE;

  /* MINUS_ONE means the symbol is not defined in this object.  It may not
     be defined at all; assume that the value doesn't matter in that
     case.  Otherwise complain if we would use the value.  */
  BFD_ASSERT (value != MINUS_ONE || (indx != 0 && need_relocs)
	      || h->root.root.type == bfd_link_hash_undefweak);

  /* Emit necessary relocations.  */
  sreloc = riscv_elf_rel_dyn_section (info, FALSE);

  /* General Dynamic.  */
  if (*tls_type_p & GOT_TLS_GD)
    {
      offset = got_offset;
      offset2 = offset + RISCV_ELF_GOT_SIZE (abfd);

      if (need_relocs)
	{
	  riscv_elf_output_dynamic_relocation
	    (abfd, sreloc, sreloc->reloc_count++, indx,
	     ABI_64_P (abfd) ? R_RISCV_TLS_DTPMOD64 : R_RISCV_TLS_DTPMOD32,
	     sec_addr(sgot) + offset);

	  if (indx)
	    riscv_elf_output_dynamic_relocation
	      (abfd, sreloc, sreloc->reloc_count++, indx,
	       ABI_64_P (abfd) ? R_RISCV_TLS_DTPREL64 : R_RISCV_TLS_DTPREL32,
	       sec_addr(sgot) + offset2);
	  else
	    RISCV_ELF_PUT_WORD (abfd, value - dtprel_base (info),
			       sgot->contents + offset2);
	}
      else
	{
	  RISCV_ELF_PUT_WORD (abfd, 1,
			     sgot->contents + offset);
	  RISCV_ELF_PUT_WORD (abfd, value - dtprel_base (info),
			     sgot->contents + offset2);
	}

      got_offset += 2 * RISCV_ELF_GOT_SIZE (abfd);
    }

  /* Initial Exec model.  */
  if (*tls_type_p & GOT_TLS_IE)
    {
      offset = got_offset;

      if (need_relocs)
	{
	  if (indx == 0)
	    RISCV_ELF_PUT_WORD (abfd, value - elf_hash_table (info)->tls_sec->vma,
			       sgot->contents + offset);
	  else
	    RISCV_ELF_PUT_WORD (abfd, 0,
			       sgot->contents + offset);

	  riscv_elf_output_dynamic_relocation
	    (abfd, sreloc, sreloc->reloc_count++, indx,
	     ABI_64_P (abfd) ? R_RISCV_TLS_TPREL64 : R_RISCV_TLS_TPREL32,
	     sec_addr(sgot) + offset);
	}
      else
	RISCV_ELF_PUT_WORD (abfd, value - tprel_base (info),
			   sgot->contents + offset);
    }

  *tls_type_p |= GOT_TLS_DONE;
}

/* Return the GOT index to use for a relocation of type R_TYPE against
   a symbol accessed using TLS_TYPE models.  The GOT entries for this
   symbol in this GOT start at GOT_INDEX.  This function initializes the
   GOT entries and corresponding relocations.  */

static bfd_vma
riscv_tls_got_index (bfd *abfd, bfd_vma got_index, unsigned char *tls_type,
		     int r_type, struct bfd_link_info *info,
		     struct riscv_elf_link_hash_entry *h, bfd_vma symbol)
{
  BFD_ASSERT (TLS_GOTTPREL_RELOC_P(r_type) || TLS_GD_RELOC_P(r_type));

  riscv_elf_initialize_tls_slots (abfd, got_index, tls_type, info, h, symbol);

  if (TLS_GOTTPREL_RELOC_P (r_type))
    {
      BFD_ASSERT (*tls_type & GOT_TLS_IE);
      if (*tls_type & GOT_TLS_GD)
	return got_index + 2 * RISCV_ELF_GOT_SIZE (abfd);
      else
	return got_index;
    }

  BFD_ASSERT (*tls_type & GOT_TLS_GD);
  return got_index;
}

/* Return the GOT offset for address VALUE.   If there is not yet a GOT
   entry for this value, create one.  If R_SYMNDX refers to a TLS symbol,
   create a TLS GOT entry instead.  Return -1 if no satisfactory GOT
   offset can be found.  */

static bfd_vma
riscv_elf_local_got_index (bfd *abfd, bfd *ibfd, struct bfd_link_info *info,
			  bfd_vma value, unsigned long r_symndx,
			  struct riscv_elf_link_hash_entry *h, int r_type)
{
  struct riscv_elf_link_hash_table *htab;
  struct riscv_got_entry *entry;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  entry = riscv_elf_create_local_got_entry (abfd, info, ibfd, value,
					   r_symndx, h, r_type);
  if (!entry)
    return MINUS_ONE;

  if (TLS_RELOC_P (r_type))
    {
      if (entry->symndx == -1)
	/* A type (3) entry in the single-GOT case.  We use the symbol's
	   hash table entry to track the index.  */
	return riscv_tls_got_index (abfd, h->tls_got_offset, &h->tls_type,
				   r_type, info, h, value);
      else
	return riscv_tls_got_index (abfd, entry->gotidx, &entry->tls_type,
				   r_type, info, h, value);
    }
  else
    return entry->gotidx;
}

/* Returns the GOT index for the global symbol indicated by H.  */

static bfd_vma
riscv_elf_global_got_index (bfd *abfd, struct elf_link_hash_entry *h,
			   int r_type, struct bfd_link_info *info)
{
  struct riscv_elf_link_hash_table *htab;
  bfd_vma got_index;
  struct riscv_got_info *g;
  long global_got_dynindx = 0;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  g = htab->got_info;

  if (g->global_gotsym != NULL)
    global_got_dynindx = g->global_gotsym->dynindx;

  if (TLS_RELOC_P (r_type))
    {
      struct riscv_elf_link_hash_entry *hm
	= (struct riscv_elf_link_hash_entry *) h;
      bfd_vma value = MINUS_ONE;

      if ((h->root.type == bfd_link_hash_defined
	   || h->root.type == bfd_link_hash_defweak)
	  && h->root.u.def.section->output_section)
	value = h->root.u.def.value + sec_addr(h->root.u.def.section);

      got_index = riscv_tls_got_index (abfd, hm->tls_got_offset, &hm->tls_type,
				      r_type, info, hm, value);
    }
  else
    {
      /* Once we determine the global GOT entry with the lowest dynamic
	 symbol table index, we must put all dynamic symbols with greater
	 indices into the GOT.  That makes it easy to calculate the GOT
	 offset.  */
      BFD_ASSERT (h->dynindx >= global_got_dynindx);
      got_index = ((h->dynindx - global_got_dynindx + g->local_gotno)
		   * RISCV_ELF_GOT_SIZE (abfd));
    }
  BFD_ASSERT (got_index < htab->sgot->size);

  return got_index;
}

/* Create and return a local GOT entry for VALUE, which was calculated
   from a symbol belonging to INPUT_SECTON.  Return NULL if it could not
   be created.  If R_SYMNDX refers to a TLS symbol, create a TLS entry
   instead.  */

static struct riscv_got_entry *
riscv_elf_create_local_got_entry (bfd *abfd, struct bfd_link_info *info,
				 bfd *ibfd, bfd_vma value,
				 unsigned long r_symndx,
				 struct riscv_elf_link_hash_entry *h,
				 int r_type)
{
  struct riscv_got_entry entry, **loc;
  struct riscv_got_info *g;
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  entry.abfd = NULL;
  entry.symndx = -1;
  entry.d.address = value;
  entry.tls_type = 0;

  g = htab->got_info;

  /* This function shouldn't be called for symbols that live in the global
     area of the GOT.  */
  BFD_ASSERT (h == NULL || h->global_got_area == GGA_NONE);
  if (TLS_RELOC_P (r_type))
    {
      struct riscv_got_entry *p;

      entry.abfd = ibfd;
      if (h == NULL)
	{
	  entry.symndx = r_symndx;
	  entry.d.addend = 0;
	}
      else
	entry.d.h = h;

      p = (struct riscv_got_entry *)
	htab_find (g->got_entries, &entry);

      BFD_ASSERT (p);
      return p;
    }

  loc = (struct riscv_got_entry **) htab_find_slot (g->got_entries, &entry,
						   INSERT);
  if (*loc)
    return *loc;

  entry.gotidx = RISCV_ELF_GOT_SIZE (abfd) * g->assigned_gotno++;
  entry.tls_type = 0;

  *loc = (struct riscv_got_entry *)bfd_alloc (abfd, sizeof entry);

  if (! *loc)
    return NULL;

  memcpy (*loc, &entry, sizeof entry);

  if (g->assigned_gotno > g->local_gotno)
    {
      (*loc)->gotidx = -1;
      /* We didn't allocate enough space in the GOT.  */
      (*_bfd_error_handler)
	(_("not enough GOT space for local GOT entries"));
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }

  RISCV_ELF_PUT_WORD (abfd, value,
		     (htab->sgot->contents + entry.gotidx));

  return *loc;
}

/* Return the number of dynamic section symbols required by OUTPUT_BFD.
   The number might be exact or a worst-case estimate, depending on how
   much information is available to elf_backend_omit_section_dynsym at
   the current linking stage.  */

static bfd_size_type
count_section_dynsyms (bfd *output_bfd, struct bfd_link_info *info)
{
  bfd_size_type count;

  count = 0;
  if (info->shared || elf_hash_table (info)->is_relocatable_executable)
    {
      asection *p;
      const struct elf_backend_data *bed;

      bed = get_elf_backend_data (output_bfd);
      for (p = output_bfd->sections; p ; p = p->next)
	if ((p->flags & SEC_EXCLUDE) == 0
	    && (p->flags & SEC_ALLOC) != 0
	    && !(*bed->elf_backend_omit_section_dynsym) (output_bfd, info, p))
	  ++count;
    }
  return count;
}

/* Sort the dynamic symbol table so that symbols that need GOT entries
   appear towards the end.  */

static bfd_boolean
riscv_elf_sort_hash_table (bfd *abfd, struct bfd_link_info *info)
{
  struct riscv_elf_link_hash_table *htab;
  struct riscv_elf_hash_sort_data hsd;
  struct riscv_got_info *g;

  if (elf_hash_table (info)->dynsymcount == 0)
    return TRUE;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  g = htab->got_info;
  if (g == NULL)
    return TRUE;

  hsd.low = NULL;
  hsd.max_unref_got_dynindx
    = hsd.min_got_dynindx
    = (elf_hash_table (info)->dynsymcount - g->reloc_only_gotno);
  hsd.max_non_got_dynindx = count_section_dynsyms (abfd, info) + 1;
  riscv_elf_link_hash_traverse (((struct riscv_elf_link_hash_table *)
				elf_hash_table (info)),
			       riscv_elf_sort_hash_table_f,
			       &hsd);

  /* There should have been enough room in the symbol table to
     accommodate both the GOT and non-GOT symbols.  */
  BFD_ASSERT (hsd.max_non_got_dynindx <= hsd.min_got_dynindx);
  BFD_ASSERT ((unsigned long) hsd.max_unref_got_dynindx
	      == elf_hash_table (info)->dynsymcount);
  BFD_ASSERT (elf_hash_table (info)->dynsymcount - hsd.min_got_dynindx
	      == g->global_gotno);

  /* Now we know which dynamic symbol has the lowest dynamic symbol
     table index in the GOT.  */
  g->global_gotsym = hsd.low;

  return TRUE;
}

/* If H needs a GOT entry, assign it the highest available dynamic
   index.  Otherwise, assign it the lowest available dynamic
   index.  */

static bfd_boolean
riscv_elf_sort_hash_table_f (struct riscv_elf_link_hash_entry *h, void *data)
{
  struct riscv_elf_hash_sort_data *hsd = data;

  if (h->root.root.type == bfd_link_hash_warning)
    h = (struct riscv_elf_link_hash_entry *) h->root.root.u.i.link;

  /* Symbols without dynamic symbol table entries aren't interesting
     at all.  */
  if (h->root.dynindx == -1)
    return TRUE;

  switch (h->global_got_area)
    {
    case GGA_NONE:
      h->root.dynindx = hsd->max_non_got_dynindx++;
      break;

    case GGA_NORMAL:
      BFD_ASSERT (h->tls_type == GOT_NORMAL);

      h->root.dynindx = --hsd->min_got_dynindx;
      hsd->low = (struct elf_link_hash_entry *) h;
      break;

    case GGA_RELOC_ONLY:
      BFD_ASSERT (h->tls_type == GOT_NORMAL);

      if (hsd->max_unref_got_dynindx == hsd->min_got_dynindx)
	hsd->low = (struct elf_link_hash_entry *) h;
      h->root.dynindx = hsd->max_unref_got_dynindx++;
      break;
    }

  return TRUE;
}

/* If H is a symbol that needs a global GOT entry, but has a dynamic
   symbol table index lower than any we've seen to date, record it for
   posterity.  FOR_CALL is true if the caller is only interested in
   using the GOT entry for calls.  */

static bfd_boolean
riscv_elf_record_global_got_symbol (struct elf_link_hash_entry *h,
				   bfd *abfd, struct bfd_link_info *info,
				   unsigned char tls_flag)
{
  struct riscv_elf_link_hash_table *htab;
  struct riscv_elf_link_hash_entry *hriscv;
  struct riscv_got_entry entry, **loc;
  struct riscv_got_info *g;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  hriscv = (struct riscv_elf_link_hash_entry *) h;

  /* A global symbol in the GOT must also be in the dynamic symbol
     table.  */
  if (h->dynindx == -1)
    {
      switch (ELF_ST_VISIBILITY (h->other))
	{
	case STV_INTERNAL:
	case STV_HIDDEN:
	  _bfd_elf_link_hash_hide_symbol (info, h, TRUE);
	  break;
	}
      if (!bfd_elf_link_record_dynamic_symbol (info, h))
	return FALSE;
    }

  /* Make sure we have a GOT to put this entry into.  */
  g = htab->got_info;
  BFD_ASSERT (g != NULL);

  entry.abfd = abfd;
  entry.symndx = -1;
  entry.d.h = (struct riscv_elf_link_hash_entry *) h;
  entry.tls_type = 0;

  loc = (struct riscv_got_entry **) htab_find_slot (g->got_entries, &entry,
						   INSERT);

  /* If we've already marked this entry as needing GOT space, we don't
     need to do it again.  */
  if (*loc)
    {
      (*loc)->tls_type |= tls_flag;
      return TRUE;
    }

  *loc = (struct riscv_got_entry *)bfd_alloc (abfd, sizeof entry);

  if (! *loc)
    return FALSE;

  entry.gotidx = -1;
  entry.tls_type = tls_flag;

  memcpy (*loc, &entry, sizeof entry);

  if (tls_flag == 0)
    hriscv->global_got_area = GGA_NORMAL;

  return TRUE;
}

/* Reserve space in G for a GOT entry containing the value of symbol
   SYMNDX in input bfd ABDF, plus ADDEND.  */

static bfd_boolean
riscv_elf_record_local_got_symbol (bfd *abfd, long symndx, bfd_vma addend,
				  struct bfd_link_info *info,
				  unsigned char tls_flag)
{
  struct riscv_elf_link_hash_table *htab;
  struct riscv_got_info *g;
  struct riscv_got_entry entry, **loc;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  g = htab->got_info;
  BFD_ASSERT (g != NULL);

  entry.abfd = abfd;
  entry.symndx = symndx;
  entry.d.addend = addend;
  entry.tls_type = tls_flag;
  loc = (struct riscv_got_entry **)
    htab_find_slot (g->got_entries, &entry, INSERT);

  if (*loc)
    {
      if (tls_flag == GOT_TLS_GD && !((*loc)->tls_type & GOT_TLS_GD))
	{
	  g->tls_gotno += 2;
	  (*loc)->tls_type |= tls_flag;
	}
      else if (tls_flag == GOT_TLS_IE && !((*loc)->tls_type & GOT_TLS_IE))
	{
	  g->tls_gotno += 1;
	  (*loc)->tls_type |= tls_flag;
	}
      return TRUE;
    }

  if (tls_flag != 0)
    {
      entry.gotidx = -1;
      entry.tls_type = tls_flag;
      BFD_ASSERT (tls_flag & (GOT_TLS_IE | GOT_TLS_GD));
      if (tls_flag == GOT_TLS_IE)
	g->tls_gotno += 1;
      else
	g->tls_gotno += 2;
    }
  else
    {
      entry.gotidx = g->local_gotno++;
      entry.tls_type = 0;
    }

  *loc = (struct riscv_got_entry *)bfd_alloc (abfd, sizeof entry);

  if (! *loc)
    return FALSE;

  memcpy (*loc, &entry, sizeof entry);

  return TRUE;
}

/* Add room for N relocations to the .rel(a).dyn section in ABFD.  */

static void
riscv_elf_allocate_dynamic_relocations (bfd *abfd, struct bfd_link_info *info,
				       unsigned int n)
{
  asection *s;
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  s = riscv_elf_rel_dyn_section (info, FALSE);
  BFD_ASSERT (s != NULL);

  if (s->size == 0)
    {
      /* Make room for a null element.  */
      s->size += RISCV_ELF_REL_SIZE (abfd);
      ++s->reloc_count;
    }
  s->size += n * RISCV_ELF_REL_SIZE (abfd);
}

/* A htab_traverse callback for GOT entries.  Set boolean *DATA to true
   if the GOT entry is for an indirect or warning symbol.  */

static int
riscv_elf_check_recreate_got (void **entryp, void *data)
{
  struct riscv_got_entry *entry;
  bfd_boolean *must_recreate;

  entry = (struct riscv_got_entry *) *entryp;
  must_recreate = (bfd_boolean *) data;
  if (entry->abfd != NULL && entry->symndx == -1)
    {
      struct riscv_elf_link_hash_entry *h;

      h = entry->d.h;
      if (h->root.root.type == bfd_link_hash_indirect
	  || h->root.root.type == bfd_link_hash_warning)
	{
	  *must_recreate = TRUE;
	  return 0;
	}
    }
  return 1;
}

/* A htab_traverse callback for GOT entries.  Add all entries to
   hash table *DATA, converting entries for indirect and warning
   symbols into entries for the target symbol.  Set *DATA to null
   on error.  */

static int
riscv_elf_recreate_got (void **entryp, void *data)
{
  htab_t *new_got;
  struct riscv_got_entry *entry;
  void **slot;

  new_got = (htab_t *) data;
  entry = (struct riscv_got_entry *) *entryp;
  if (entry->abfd != NULL && entry->symndx == -1)
    {
      struct riscv_elf_link_hash_entry *h;

      h = entry->d.h;
      while (h->root.root.type == bfd_link_hash_indirect
	     || h->root.root.type == bfd_link_hash_warning)
	{
	  BFD_ASSERT (h->global_got_area == GGA_NONE);
	  h = (struct riscv_elf_link_hash_entry *) h->root.root.u.i.link;
	}
      entry->d.h = h;
    }
  slot = htab_find_slot (*new_got, entry, INSERT);
  if (slot == NULL)
    {
      *new_got = NULL;
      return 0;
    }
  if (*slot == NULL)
    *slot = entry;
  else
    free (entry);
  return 1;
}

/* If any entries in G->got_entries are for indirect or warning symbols,
   replace them with entries for the target symbol.  */

static bfd_boolean
riscv_elf_resolve_final_got_entries (struct riscv_got_info *g)
{
  bfd_boolean must_recreate;
  htab_t new_got;

  must_recreate = FALSE;
  htab_traverse (g->got_entries, riscv_elf_check_recreate_got, &must_recreate);
  if (must_recreate)
    {
      new_got = htab_create (htab_size (g->got_entries),
			     riscv_elf_got_entry_hash,
			     riscv_elf_got_entry_eq, NULL);
      htab_traverse (g->got_entries, riscv_elf_recreate_got, &new_got);
      if (new_got == NULL)
	return FALSE;

      /* Each entry in g->got_entries has either been copied to new_got
	 or freed.  Now delete the hash table itself.  */
      htab_delete (g->got_entries);
      g->got_entries = new_got;
    }
  return TRUE;
}

/* A riscv_elf_link_hash_traverse callback for which DATA points
   to the link_info structure.  Count the number of type (3) entries
   in the master GOT.  */

static int
riscv_elf_count_got_symbols (struct riscv_elf_link_hash_entry *h, void *data)
{
  struct bfd_link_info *info;
  struct riscv_elf_link_hash_table *htab;
  struct riscv_got_info *g;

  info = (struct bfd_link_info *) data;
  htab = riscv_elf_hash_table (info);
  g = htab->got_info;
  if (h->global_got_area != GGA_NONE)
    {
      /* Make a final decision about whether the symbol belongs in the
	 local or global GOT.  Symbols that bind locally can (and in the
	 case of forced-local symbols, must) live in the local GOT.
	 Those that are aren't in the dynamic symbol table must also
	 live in the local GOT.

	 Note that the former condition does not always imply the
	 latter: symbols do not bind locally if they are completely
	 undefined.  We'll report undefined symbols later if appropriate.  */
      if (h->root.dynindx == -1 || SYMBOL_REFERENCES_LOCAL (info, &h->root))
	{
	  /* The symbol belongs in the local GOT.  We no longer need this
	     entry if it was only used for relocations; those relocations
	     will be against the null or section symbol instead of H.  */
	  if (h->global_got_area != GGA_RELOC_ONLY)
	    g->local_gotno++;
	  h->global_got_area = GGA_NONE;
	}
      else
	{
	  g->global_gotno++;
	  if (h->global_got_area == GGA_RELOC_ONLY)
	    g->reloc_only_gotno++;
	}
    }
  return 1;
}

/* Set the TLS GOT index for the GOT entry in ENTRYP.  ENTRYP's NEXT field
   is null iff there is just a single GOT.  */

static int
riscv_elf_initialize_tls_index (void **entryp, void *p)
{
  struct riscv_got_entry *entry = (struct riscv_got_entry *)*entryp;
  struct riscv_got_info *g = p;
  bfd_vma next_index;
  unsigned char tls_type;

  /* We're only interested in TLS symbols.  */
  if (entry->tls_type == 0)
    return 1;

  next_index = RISCV_ELF_GOT_SIZE (entry->abfd) * (long) g->tls_assigned_gotno;

  if (entry->symndx == -1)
    {
      /* A type (3) got entry in the single-GOT case.  We use the symbol's
	 hash table entry to track its index.  */
      if (entry->d.h->tls_type & GOT_TLS_OFFSET_DONE)
	return 1;
      entry->d.h->tls_type |= GOT_TLS_OFFSET_DONE;
      entry->d.h->tls_got_offset = next_index;
      tls_type = entry->d.h->tls_type;
    }
  else
    {
      entry->gotidx = next_index;
      tls_type = entry->tls_type;
    }

  /* Account for the entries we've just allocated.  */
  if (tls_type & GOT_TLS_GD)
    g->tls_assigned_gotno += 2;
  if (tls_type & GOT_TLS_IE)
    g->tls_assigned_gotno += 1;

  return 1;
}

/* Return whether an input relocation is against a local symbol.  */

static bfd_boolean
riscv_elf_local_relocation_p (bfd *input_bfd,
			     const Elf_Internal_Rela *relocation,
			     asection **local_sections)
{
  unsigned long r_symndx;
  Elf_Internal_Shdr *symtab_hdr;
  size_t extsymoff;

  r_symndx = ELF_R_SYM (input_bfd, relocation->r_info);
  symtab_hdr = &elf_tdata (input_bfd)->symtab_hdr;
  extsymoff = (elf_bad_symtab (input_bfd)) ? 0 : symtab_hdr->sh_info;

  if (r_symndx < extsymoff)
    return TRUE;
  if (elf_bad_symtab (input_bfd) && local_sections[r_symndx] != NULL)
    return TRUE;

  return FALSE;
}

/* Create the .got section to hold the global offset table.  */

static bfd_boolean
riscv_elf_create_got_section (bfd *abfd, struct bfd_link_info *info)
{
  flagword flags;
  register asection *s;
  struct elf_link_hash_entry *h;
  struct bfd_link_hash_entry *bh;
  struct riscv_got_info *g;
  bfd_size_type amt;
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  /* This function may be called more than once.  */
  if (htab->sgot)
    return TRUE;

  flags = (SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_IN_MEMORY
	   | SEC_LINKER_CREATED);

  /* We have to use an alignment of 2**4 here because this is hardcoded
     in the function stub generation and in the linker script.  */
  s = bfd_make_section_with_flags (abfd, ".got", flags);
  if (s == NULL
      || ! bfd_set_section_alignment (abfd, s, 4))
    return FALSE;
  htab->sgot = s;

  /* Define the symbol _GLOBAL_OFFSET_TABLE_.  We don't do this in the
     linker script because we don't want to define the symbol if we
     are not creating a global offset table.  */
  bh = NULL;
  if (! (_bfd_generic_link_add_one_symbol
	 (info, abfd, "_GLOBAL_OFFSET_TABLE_", BSF_GLOBAL, s,
	  0, NULL, FALSE, get_elf_backend_data (abfd)->collect, &bh)))
    return FALSE;

  h = (struct elf_link_hash_entry *) bh;
  h->non_elf = 0;
  h->def_regular = 1;
  h->type = STT_OBJECT;
  elf_hash_table (info)->hgot = h;

  if (info->shared
      && ! bfd_elf_link_record_dynamic_symbol (info, h))
    return FALSE;

  amt = sizeof (struct riscv_got_info);
  g = bfd_alloc (abfd, amt);
  if (g == NULL)
    return FALSE;
  g->global_gotsym = NULL;
  g->global_gotno = 0;
  g->reloc_only_gotno = 0;
  g->tls_gotno = 0;
  g->local_gotno = 0;
  g->assigned_gotno = 0;
  g->got_entries = htab_try_create (1, riscv_elf_got_entry_hash,
				    riscv_elf_got_entry_eq, NULL);
  if (g->got_entries == NULL)
    return FALSE;
  htab->got_info = g;
  riscv_elf_section_data (s)->elf.this_hdr.sh_flags |= SHF_ALLOC | SHF_WRITE;

  /* We also need a .got.plt section when generating PLTs.  */
  s = bfd_make_section_with_flags (abfd, ".got.plt",
				   SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS
				   | SEC_IN_MEMORY | SEC_LINKER_CREATED);
  if (s == NULL)
    return FALSE;
  htab->sgotplt = s;

  return TRUE;
}

/* Return address for Ith PLT stub in section PLT, for relocation REL
   or (bfd_vma) -1 if it should not be included.  */

bfd_vma
_bfd_riscv_elf_plt_sym_val (bfd_vma i, const asection *s,
			   const arelent *rel ATTRIBUTE_UNUSED)
{
  uint32_t plt0_1;
  if (!bfd_get_section_contents (s->owner, (sec_ptr)s, &plt0_1, 4, 4))
    return MINUS_ONE;
  return s->vma + PLT_HEADER_SIZE + i * PLT_ENTRY_SIZE;
}

/* Obtain the field relocated by RELOCATION.  */

static bfd_vma
riscv_elf_obtain_contents (reloc_howto_type *howto,
			  const Elf_Internal_Rela *relocation,
			  bfd *input_bfd, bfd_byte *contents)
{
  bfd_vma x;
  bfd_byte *location = contents + relocation->r_offset;

  /* Obtain the bytes.  */
  x = bfd_get ((8 * bfd_get_reloc_size (howto)), input_bfd, location);

  return x;
}

/* It has been determined that the result of the RELOCATION is the
   VALUE.  Use HOWTO to place VALUE into the output file at the
   appropriate position.  The SECTION is the section to which the
   relocation applies.  

   Returns FALSE if anything goes wrong.  */

static bfd_boolean
riscv_elf_perform_relocation (reloc_howto_type *howto,
			     const Elf_Internal_Rela *relocation,
			     bfd_vma value,
			     bfd *input_bfd,
			     bfd_byte *contents)
{
  bfd_vma x;
  bfd_byte *location;
  bfd_vma dst_mask = howto->dst_mask;

  /* Figure out where the relocation is occurring.  */
  location = contents + relocation->r_offset;

  /* Obtain the current value.  */
  x = riscv_elf_obtain_contents (howto, relocation, input_bfd, contents);

  /* Update the field. */
  x = (x &~ dst_mask) | (value & dst_mask);

  /* Put the value into the output.  */
  bfd_put (8 * bfd_get_reloc_size (howto), input_bfd, x, location);

  return TRUE;
}

/* Remember all PC-relative high-part relocs we've encountered to help us
   later resolve the corresponding low-part relocs.  */

typedef struct {
  bfd_vma address;
  bfd_vma value;
} riscv_pcrel_hi_reloc;

typedef struct riscv_pcrel_lo_reloc {
  asection *input_section;
  struct bfd_link_info *info;
  reloc_howto_type *howto;
  const Elf_Internal_Rela *reloc;
  bfd_vma addr;
  const char *name;
  bfd_byte *contents;
  struct riscv_pcrel_lo_reloc *next;
} riscv_pcrel_lo_reloc;

typedef struct {
  htab_t hi_relocs;
  riscv_pcrel_lo_reloc *lo_relocs;
} riscv_pcrel_relocs;

static bfd_boolean
riscv_init_pcrel_relocs (riscv_pcrel_relocs *p)
{
  hashval_t hash (const void *entry)
  {
    const riscv_pcrel_hi_reloc *e = entry;
    return (hashval_t)(e->address >> 2);
  }

  bfd_boolean eq (const void *entry1, const void *entry2)
  {
    const riscv_pcrel_hi_reloc *e1 = entry1, *e2 = entry2;
    return e1->address == e2->address;
  }

  p->lo_relocs = NULL;
  p->hi_relocs = htab_create (1024, hash, eq, free);
  return p->hi_relocs != NULL;
}

static void
riscv_free_pcrel_relocs (riscv_pcrel_relocs *p)
{
  riscv_pcrel_lo_reloc *cur = p->lo_relocs;
  while (cur != NULL)
    {
      riscv_pcrel_lo_reloc *next = cur->next;
      free (cur);
      cur = next;
    }

  htab_delete (p->hi_relocs);
}

static bfd_boolean
riscv_record_pcrel_hi_reloc (riscv_pcrel_relocs *p, bfd_vma addr, bfd_vma value)
{
  riscv_pcrel_hi_reloc entry = {addr, value};
  riscv_pcrel_hi_reloc **slot =
    (riscv_pcrel_hi_reloc **) htab_find_slot (p->hi_relocs, &entry, INSERT);
  BFD_ASSERT (*slot == NULL);
  *slot = (riscv_pcrel_hi_reloc *) bfd_malloc (sizeof (riscv_pcrel_hi_reloc));
  if (*slot == NULL)
    return FALSE;
  **slot = entry;
  return TRUE;
}

static bfd_boolean
riscv_record_pcrel_lo_reloc (riscv_pcrel_relocs *p,
			     asection *input_section,
			     struct bfd_link_info *info,
			     reloc_howto_type *howto,
			     const Elf_Internal_Rela *reloc,
			     bfd_vma addr,
			     const char *name,
			     bfd_byte *contents)
{
  riscv_pcrel_lo_reloc *entry;
  entry = (riscv_pcrel_lo_reloc *) bfd_malloc (sizeof (riscv_pcrel_lo_reloc));
  if (entry == NULL)
    return FALSE;
  *entry = (riscv_pcrel_lo_reloc) {input_section, info, howto, reloc, addr,
				   name, contents, p->lo_relocs};
  p->lo_relocs = entry;
  return TRUE;
}

static bfd_boolean
riscv_resolve_pcrel_lo_relocs (riscv_pcrel_relocs *p)
{
  riscv_pcrel_lo_reloc *r;
  for (r = p->lo_relocs; r != NULL; r = r->next)
    {
      bfd *input_bfd = r->input_section->owner;
      riscv_pcrel_hi_reloc search = {r->addr, 0};
      riscv_pcrel_hi_reloc *entry = htab_find (p->hi_relocs, &search);
      if (entry == NULL)
	return ((*r->info->callbacks->reloc_overflow)
		 (r->info, NULL, r->name, r->howto->name, (bfd_vma) 0,
		  input_bfd, r->input_section, r->reloc->r_offset));

      bfd_vma value = entry->value + r->reloc->r_addend;
      switch (ELF_R_TYPE (input_bfd, r->reloc->r_info))
	{
	case R_RISCV_PCREL_LO12_S:
	  value = ENCODE_STYPE_IMM (value);
	  break;
	default:
	  value = ENCODE_ITYPE_IMM (value);
	  break;
	}

      if (! riscv_elf_perform_relocation (r->howto, r->reloc, value,
					  input_bfd, r->contents))
	return FALSE;
    }

  return TRUE;
}

/* Calculate the value produced by the RELOCATION (which comes from
   the INPUT_BFD).  The ADDEND is the addend to use for this
   RELOCATION; RELOCATION->R_ADDEND is ignored.
   The result of the relocation calculation is stored in VALUEP.

   This function returns bfd_reloc_continue if the caller need take no
   further action regarding this relocation, bfd_reloc_notsupported if
   something goes dramatically wrong, bfd_reloc_overflow if an
   overflow occurs, and bfd_reloc_ok to indicate success.  */

static bfd_reloc_status_type
riscv_elf_calculate_relocation (bfd *abfd,
				asection *input_section,
				struct bfd_link_info *info,
				riscv_pcrel_relocs *pcrel_relocs,
				const Elf_Internal_Rela *relocation,
				bfd_vma addend, reloc_howto_type *howto,
				Elf_Internal_Sym *local_syms,
				asection **local_sections, bfd_vma *valuep,
				const char **namep, bfd_byte *contents)
{
  /* The eventual value we will return.  */
  bfd_vma value;
  /* The address of the symbol against which the relocation is
     occurring.  */
  bfd_vma symbol = 0;
  /* The place (section offset or address) of the storage unit being
     relocated.  */
  bfd_vma p;
  /* The offset into the global offset table at which the address of
     the relocation entry symbol, adjusted by the addend, resides
     during execution.  */
  bfd_vma g = MINUS_ONE;
  /* The section in which the symbol referenced by the relocation is
     located.  */
  asection *sec = NULL;
  struct riscv_elf_link_hash_entry *h = NULL;
  Elf_Internal_Shdr *symtab_hdr;
  size_t extsymoff;
  unsigned long r_symndx;
  int r_type;
  /* TRUE if overflow occurred during the calculation of the
     relocation value.  */
  bfd_boolean overflowed_p;
  struct riscv_elf_link_hash_table *htab;
  bfd *dynobj;
  bfd_vma gp = _bfd_get_gp_value (abfd);
  bfd *input_bfd = input_section->owner;

  dynobj = elf_hash_table (info)->dynobj;
  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  /* Parse the relocation.  */
  r_symndx = ELF_R_SYM (input_bfd, relocation->r_info);
  r_type = ELF_R_TYPE (input_bfd, relocation->r_info);
  p = sec_addr(input_section) + relocation->r_offset;

  /* Assume that there will be no overflow.  */
  overflowed_p = FALSE;

  /* Figure out whether or not the symbol is local, and get the offset
     used in the array of hash table entries.  */
  symtab_hdr = &elf_tdata (input_bfd)->symtab_hdr;
  if (! elf_bad_symtab (input_bfd))
    extsymoff = symtab_hdr->sh_info;
  else
    {
      /* The symbol table does not follow the rule that local symbols
	 must come before globals.  */
      extsymoff = 0;
    }

  /* Figure out the value of the symbol.  */
  if (riscv_elf_local_relocation_p (input_bfd, relocation, local_sections))
    {
      Elf_Internal_Sym *sym;

      sym = local_syms + r_symndx;
      sec = local_sections[r_symndx];

      symbol = sec_addr(sec);
      if (ELF_ST_TYPE (sym->st_info) != STT_SECTION
	  || (sec->flags & SEC_MERGE))
	symbol += sym->st_value;
      if ((sec->flags & SEC_MERGE)
	  && ELF_ST_TYPE (sym->st_info) == STT_SECTION)
	{
	  addend = _bfd_elf_rel_local_sym (abfd, sym, &sec, addend);
	  addend -= symbol;
	  addend += sec_addr(sec);
	}

      /* Record the name of this symbol, for our caller.  */
      *namep = bfd_elf_string_from_elf_section (input_bfd,
						symtab_hdr->sh_link,
						sym->st_name);
      if (*namep == '\0')
	*namep = bfd_section_name (input_bfd, sec);
    }
  else
    {
      /* ??? Could we use RELOC_FOR_GLOBAL_SYMBOL here ?  */

      /* For global symbols we look up the symbol in the hash-table.  */
      h = ((struct riscv_elf_link_hash_entry *)
	   elf_sym_hashes (input_bfd) [r_symndx - extsymoff]);
      /* Find the real hash-table entry for this symbol.  */
      while (h->root.root.type == bfd_link_hash_indirect
	     || h->root.root.type == bfd_link_hash_warning)
	h = (struct riscv_elf_link_hash_entry *) h->root.root.u.i.link;

      /* Record the name of this symbol, for our caller.  */
      *namep = h->root.root.root.string;

      /* If this symbol is defined, calculate its address. */
      if ((h->root.root.type == bfd_link_hash_defined
		|| h->root.root.type == bfd_link_hash_defweak)
	       && h->root.root.u.def.section)
	{
	  sec = h->root.root.u.def.section;
	  if (sec->output_section)
	    symbol = h->root.root.u.def.value + sec_addr(sec);
	  else
	    symbol = h->root.root.u.def.value;
	}
      else if (h->root.root.type == bfd_link_hash_undefweak)
	/* We allow relocations against undefined weak symbols, giving
	   it the value zero, so that you can undefined weak functions
	   and check to see if they exist by looking at their
	   addresses.  */
	symbol = 0;
      else if (info->unresolved_syms_in_objects == RM_IGNORE
	       && ELF_ST_VISIBILITY (h->root.other) == STV_DEFAULT)
	symbol = 0;
      else if (strcmp (*namep, "_DYNAMIC_LINKING") == 0)
	{
	  /* If this is a dynamic link, we should have created a
	     _DYNAMIC_LINKING symbol
	     in in _bfd_riscv_elf_create_dynamic_sections.
	     Otherwise, we should define the symbol with a value of 0.
	     FIXME: It should probably get into the symbol table
	     somehow as well.  */
	  BFD_ASSERT (! info->shared);
	  BFD_ASSERT (bfd_get_section_by_name (abfd, ".dynamic") == NULL);
	  symbol = 0;
	}
      else if ((*info->callbacks->undefined_symbol)
	       (info, h->root.root.root.string, input_bfd,
		input_section, relocation->r_offset,
		(info->unresolved_syms_in_objects == RM_GENERATE_ERROR)
		 || ELF_ST_VISIBILITY (h->root.other)))
	{
	  return bfd_reloc_undefined;
	}
      else
	{
	  return bfd_reloc_notsupported;
	}
    }

  /* If we haven't already determined the GOT offset, and we're going
     to need it, get it now.  */
  switch (r_type)
    {
    case R_RISCV_GOT_HI20:
    case R_RISCV_TLS_GD_HI20:
    case R_RISCV_TLS_GOT_HI20:
    case R_RISCV_TLS_IE_HI20:
    case R_RISCV_TLS_IE_LO12:
      if (h != NULL && !SYMBOL_REFERENCES_LOCAL (info, &h->root))
	{
	  BFD_ASSERT (addend == 0);
	  g = riscv_elf_global_got_index (dynobj, &h->root, r_type, info);
	  if (h->tls_type == GOT_NORMAL
	      && !elf_hash_table (info)->dynamic_sections_created)
	    /* This is a static link.  We must initialize the GOT entry.  */
	    RISCV_ELF_PUT_WORD (dynobj, symbol, htab->sgot->contents + g);
	}
      else
	{
	  g = riscv_elf_local_got_index (abfd, input_bfd, info,
					symbol + addend, r_symndx, h, r_type);
	  if (g == MINUS_ONE)
	    return bfd_reloc_outofrange;
	}

      /* Convert GOT indices to actual offsets.  */
      g += sec_addr (riscv_elf_hash_table (info)->sgot);
      break;
    }

  /* Figure out what kind of relocation is being performed.  */
  switch (r_type)
    {
    case R_RISCV_NONE:
      return bfd_reloc_continue;

    case R_RISCV_32:
    case R_RISCV_REL32:
    case R_RISCV_64:
      if ((info->shared
	   || (htab->root.dynamic_sections_created
	       && h != NULL
	       && h->root.def_dynamic
	       && !h->root.def_regular
	       && !h->has_static_relocs))
	  && r_symndx != STN_UNDEF
	  && (h == NULL
	      || h->root.root.type != bfd_link_hash_undefweak
	      || ELF_ST_VISIBILITY (h->root.other) == STV_DEFAULT)
	  && (input_section->flags & SEC_ALLOC) != 0)
	{
	  /* If we're creating a shared library, then we can't know
	     where the symbol will end up.  So, we create a relocation
	     record in the output, and leave the job up to the dynamic
	     linker.  We must do the same for executable references to
	     shared library symbols, unless we've decided to use copy
	     relocs or PLTs instead.  */
	  value = addend;
	  if (!riscv_elf_create_dynamic_relocation (abfd, info, relocation,
						    h, sec, symbol, &value,
						    input_section))
	    return bfd_reloc_undefined;
	}
      else
	{
	  if (r_type != R_RISCV_REL32)
	    value = symbol + addend;
	  else
	    value = addend;
	}
      value &= howto->dst_mask;
      break;

    case R_RISCV_ADD32:
    case R_RISCV_ADD64:
      value = addend + symbol;
      break;

    case R_RISCV_SUB32:
    case R_RISCV_SUB64:
      value = bfd_get (howto->bitsize, input_bfd,
		       contents + relocation->r_offset);
      value -= addend + symbol;
      break;

    case R_RISCV_CALL_PLT:
    case R_RISCV_CALL:
    {
      bfd_vma auipc = bfd_get (32, input_bfd, contents + relocation->r_offset);
      bfd_vma jalr = bfd_get (32, input_bfd, contents + relocation->r_offset + 4);

      if (info->shared && h && h->root.plt.offset != MINUS_ONE)
	symbol = sec_addr(htab->splt) + h->root.plt.offset;
      value = addend + (symbol ? symbol : p);

      auipc |= ENCODE_UTYPE_IMM (RISCV_PCREL_HIGH_PART (value, p));
      jalr |= ENCODE_ITYPE_IMM (RISCV_PCREL_LOW_PART (value, p));

      bfd_put (32, input_bfd, auipc, contents + relocation->r_offset);
      bfd_put (32, input_bfd, jalr, contents + relocation->r_offset + 4);

      return bfd_reloc_continue;
    }
    case R_RISCV_JAL:
      if (info->shared && h && h->root.plt.offset != MINUS_ONE)
	symbol = sec_addr(htab->splt) + h->root.plt.offset;
      value = addend;
      if (symbol)
	value += symbol - p;
      overflowed_p = !VALID_UJTYPE_IMM (value);
      if (overflowed_p && !info->shared && VALID_ITYPE_IMM (value + p))
	{
	  /* Not all is lost: we can instead use JALR rd, x0, address. */
	  bfd_vma jal = bfd_get (32, input_bfd, contents+relocation->r_offset);
	  jal = (jal & (OP_MASK_RD << OP_SH_RD)) | MATCH_JALR;
	  jal |= ENCODE_ITYPE_IMM (value + p);
	  bfd_put (32, input_bfd, jal, contents+relocation->r_offset);
	  return bfd_reloc_continue;
	}
      value = ENCODE_UJTYPE_IMM (value);
      break;

    case R_RISCV_BRANCH:
      value = addend;
      if (symbol)
	value += symbol - p;
      overflowed_p = !VALID_SBTYPE_IMM (value);
      value = ENCODE_SBTYPE_IMM (value);
      break;

    case R_RISCV_TLS_DTPREL32:
    case R_RISCV_TLS_DTPREL64:
      value = ENCODE_ITYPE_IMM (addend + symbol - dtprel_base (info));
      break;

    case R_RISCV_TPREL_HI20:
      value = RISCV_LUI_HIGH_PART (addend + symbol - tprel_base (info));
      value = ENCODE_UTYPE_IMM (value);
      break;

    case R_RISCV_TPREL_ADD:
    case R_RISCV_TLS_IE_ADD:
    case R_RISCV_TLS_IE_LO12_I:
    case R_RISCV_TLS_IE_LO12_S:
      value = 0;
      break;

    case R_RISCV_TPREL_LO12_I:
    case R_RISCV_TPREL_LO12_S:
      {
	bfd_vma insn = bfd_get (32, input_bfd, contents + relocation->r_offset);
	bfd_vma rs1 = (insn >> OP_SH_RS1) & OP_MASK_RS1;

	value = symbol + addend - tprel_base (info);
	if (htab->relax && RISCV_CONST_HIGH_PART (value) == 0)
	  rs1 = TP_REG; /* Reference TP directly if possible. */

	if (r_type == R_RISCV_TPREL_LO12_I)
	  value = ENCODE_ITYPE_IMM (value);
	else
	  value = ENCODE_STYPE_IMM (value);
	value |= rs1 << OP_SH_RS1;
	break;
      }

    case R_RISCV_TLS_IE_HI20:
      value = ENCODE_UTYPE_IMM (RISCV_LUI_HIGH_PART (g));
      break;

    case R_RISCV_TLS_IE_LO12:
      value = ENCODE_ITYPE_IMM (g);
      break;

    case R_RISCV_HI20:
      value = ENCODE_UTYPE_IMM (RISCV_LUI_HIGH_PART (addend + symbol));
      break;

    case R_RISCV_LO12_I:
    case R_RISCV_LO12_S:
      {
	bfd_vma insn = bfd_get (32, input_bfd, contents + relocation->r_offset);
	bfd_vma rs1 = (insn >> OP_SH_RS1) & OP_MASK_RS1;

	value = symbol + addend;
	if (htab->relax && gp != 0 && value != gp
	    && RISCV_CONST_HIGH_PART (value - gp) == 0)
	  {
	    /* Convert to GP-relative reference. */
	    value -= gp;
	    rs1 = GP_REG;
	  }

	if (r_type == R_RISCV_LO12_I)
	  value = ENCODE_ITYPE_IMM (value);
	else
	  value = ENCODE_STYPE_IMM (value);
	value |= rs1 << OP_SH_RS1;
      }
      break;

    case R_RISCV_PCREL_HI20:
      value = addend + symbol - p;
      if (!riscv_record_pcrel_hi_reloc (pcrel_relocs, p, value))
	overflowed_p = TRUE;
      value = ENCODE_UTYPE_IMM (RISCV_LUI_HIGH_PART (value));
      break;

    case R_RISCV_TLS_GOT_HI20:
    case R_RISCV_TLS_GD_HI20:
    case R_RISCV_GOT_HI20:
      value = g - p;
      if (!riscv_record_pcrel_hi_reloc (pcrel_relocs, p, value))
	overflowed_p = TRUE;
      value = ENCODE_UTYPE_IMM (RISCV_LUI_HIGH_PART (value));
      break;

    case R_RISCV_TLS_PCREL_LO12:
    case R_RISCV_PCREL_LO12_I:
    case R_RISCV_PCREL_LO12_S:
    case R_RISCV_TLS_GOT_LO12:
    case R_RISCV_TLS_GD_LO12:
    case R_RISCV_GOT_LO12:
      if (riscv_record_pcrel_lo_reloc (pcrel_relocs, input_section, info, howto,
				       relocation, symbol, *namep, contents))
	return bfd_reloc_continue;

      value = 0;
      overflowed_p = TRUE;
      break;

    default:
      /* An unrecognized relocation type.  */
      return bfd_reloc_notsupported;
    }

  /* Store the VALUE for our caller.  */
  *valuep = value;
  return overflowed_p ? bfd_reloc_overflow : bfd_reloc_ok;
}

/* Create a rel.dyn relocation for the dynamic linker to resolve.  REL
   is the original relocation, which is now being transformed into a
   dynamic relocation.  The ADDENDP is adjusted if necessary; the
   caller should store the result in place of the original addend.  */

static bfd_boolean
riscv_elf_create_dynamic_relocation (bfd *output_bfd,
				    struct bfd_link_info *info,
				    const Elf_Internal_Rela *rel,
				    struct riscv_elf_link_hash_entry *h,
				    asection *sec, bfd_vma symbol,
				    bfd_vma *addendp, asection *input_section)
{
  Elf_Internal_Rela outrel;
  asection *sreloc;
  int r_type;
  long indx;
  bfd_boolean defined_p;
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  r_type = ELF_R_TYPE (output_bfd, rel->r_info);
  sreloc = riscv_elf_rel_dyn_section (info, FALSE);
  BFD_ASSERT (sreloc != NULL);
  BFD_ASSERT (sreloc->contents != NULL);
  BFD_ASSERT (sreloc->reloc_count * RISCV_ELF_REL_SIZE (output_bfd)
	      < sreloc->size);

  outrel.r_offset =
    _bfd_elf_section_offset (output_bfd, info, input_section, rel[0].r_offset);

  if (outrel.r_offset == MINUS_ONE)
    /* The relocation field has been deleted.  */
    return TRUE;

  if (outrel.r_offset == MINUS_TWO)
    {
      /* The relocation field has been converted into a relative value of
	 some sort.  Functions like _bfd_elf_write_section_eh_frame expect
	 the field to be fully relocated, so add in the symbol's value.  */
      *addendp += symbol;
      return TRUE;
    }

  /* We must now calculate the dynamic symbol table index to use
     in the relocation.  */
  if (h != NULL && ! SYMBOL_REFERENCES_LOCAL (info, &h->root))
    {
      BFD_ASSERT (h->global_got_area != GGA_NONE);
      indx = h->root.dynindx;
      /* ??? glibc's ld.so just adds the final GOT entry to the
         relocation field.  It therefore treats relocs against
         defined symbols in the same way as relocs against
         undefined symbols.  */
      defined_p = FALSE;
    }
  else
    {
      if (sec != NULL && bfd_is_abs_section (sec))
	indx = 0;
      else if (sec == NULL || sec->owner == NULL)
	{
	  bfd_set_error (bfd_error_bad_value);
	  return FALSE;
	}
      else
	{
	  indx = elf_section_data (sec->output_section)->dynindx;
	  if (indx == 0)
	    {
	      asection *osec = htab->root.text_index_section;
	      indx = elf_section_data (osec)->dynindx;
	    }
	  if (indx == 0)
	    abort ();
	}

      /* Instead of generating a relocation using the section
	 symbol, we may as well make it a fully relative
	 relocation.  We want to avoid generating relocations to
	 local symbols because we used to generate them
	 incorrectly, without adding the original symbol value,
	 which is mandated by the ABI for section symbols.  In
	 order to give dynamic loaders and applications time to
	 phase out the incorrect use, we refrain from emitting
	 section-relative relocations.  It's not like they're
	 useful, after all.  This should be a bit more efficient
	 as well.  */
      /* ??? Although this behavior is compatible with glibc's ld.so,
	 the ABI says that relocations against STN_UNDEF should have
	 a symbol value of 0.  Irix rld honors this, so relocations
	 against STN_UNDEF have no effect.  */
      indx = 0;
      defined_p = TRUE;
    }

  /* If the relocation was previously an absolute relocation and
     this symbol will not be referred to by the relocation, we must
     adjust it by the value we give it in the dynamic symbol table.
     Otherwise leave the job up to the dynamic linker.  */
  if (defined_p && r_type != R_RISCV_REL32)
    *addendp += symbol;

  /* The relocation is always an REL32 relocation because we don't
     know where the shared library will wind up at load-time.  */
  outrel.r_info = ELF_R_INFO (output_bfd, (unsigned long) indx,
				 R_RISCV_REL32);

  /* Adjust the output offset of the relocation to reference the
     correct location in the output file.  */
  outrel.r_offset += sec_addr(input_section);

  /* Put the relocation back out. */
  if (ABI_64_P (output_bfd))
    bfd_elf64_swap_reloc_out
      (output_bfd, &outrel,
       (sreloc->contents + sreloc->reloc_count * sizeof (Elf64_External_Rel)));
  else
    bfd_elf32_swap_reloc_out
      (output_bfd, &outrel,
       (sreloc->contents + sreloc->reloc_count * sizeof (Elf32_External_Rel)));

  /* We've now added another relocation.  */
  ++sreloc->reloc_count;

  /* Make sure the output section is writable.  The dynamic linker
     will be writing to it.  */
  elf_section_data (input_section->output_section)->this_hdr.sh_flags
    |= SHF_WRITE;

  /* If we've written this relocation for a readonly section,
     we need to set DF_TEXTREL again, so that we do not delete the
     DT_TEXTREL tag.  */
  if (RISCV_ELF_READONLY_SECTION (input_section))
    info->flags |= DF_TEXTREL;

  return TRUE;
}

/* Return printable name for ABI.  */

static INLINE char *
elf_riscv_abi_name (bfd *abfd)
{
  return ABI_32_P (abfd) ? "rv32" : "rv64";
}

/* This is used for both the 32-bit and the 64-bit ABI.  */

void
_bfd_riscv_elf_symbol_processing (bfd *abfd ATTRIBUTE_UNUSED, asymbol *asym)
{
  elf_symbol_type *elfsym;

  /* Handle the special RISC-V section numbers that a symbol may use.  */
  elfsym = (elf_symbol_type *) asym;
  switch (elfsym->internal_elf_sym.st_shndx)
    {
    case SHN_COMMON:
      /* TODO: put small common data in the .scommon section. */
      break;
  }
}

/* Implement elf_backend_eh_frame_address_size. */

unsigned int
_bfd_riscv_elf_eh_frame_address_size (bfd *abfd, asection *sec ATTRIBUTE_UNUSED)
{
  if (elf_elfheader (abfd)->e_ident[EI_CLASS] == ELFCLASS64)
    return 8;
  return 4;
}

/* Functions for the dynamic linker.  */

/* Create dynamic sections when linking against a dynamic object.  */

bfd_boolean
_bfd_riscv_elf_create_dynamic_sections (bfd *abfd, struct bfd_link_info *info)
{
  struct elf_link_hash_entry *h;
  struct bfd_link_hash_entry *bh;
  flagword flags;
  register asection *s;
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  flags = (SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_IN_MEMORY
	   | SEC_LINKER_CREATED | SEC_READONLY);

  /* The psABI requires a read-only .dynamic section. */
  s = bfd_get_section_by_name (abfd, ".dynamic");
  if (s != NULL)
    {
      if (! bfd_set_section_flags (abfd, s, flags))
        return FALSE;
    }

  /* We need to create .got section.  */
  if (!riscv_elf_create_got_section (abfd, info))
    return FALSE;

  if (! riscv_elf_rel_dyn_section (info, TRUE))
    return FALSE;

  if (!info->shared)
    {
      const char *name;

      name = "_DYNAMIC_LINKING";
      bh = NULL;
      if (!(_bfd_generic_link_add_one_symbol
	    (info, abfd, name, BSF_GLOBAL, bfd_abs_section_ptr, 0,
	     NULL, FALSE, get_elf_backend_data (abfd)->collect, &bh)))
	return FALSE;

      h = (struct elf_link_hash_entry *) bh;
      h->non_elf = 0;
      h->def_regular = 1;
      h->type = STT_SECTION;

      if (! bfd_elf_link_record_dynamic_symbol (info, h))
	return FALSE;
    }

  /* Create the .plt, .rel(a).plt, .dynbss and .rel(a).bss sections.
     Also create the _PROCEDURE_LINKAGE_TABLE symbol.  */
  if (!_bfd_elf_create_dynamic_sections (abfd, info))
    return FALSE;

  /* Cache the sections created above.  */
  htab->splt = bfd_get_section_by_name (abfd, ".plt");
  htab->sdynbss = bfd_get_section_by_name (abfd, ".dynbss");
  htab->srelplt = bfd_get_section_by_name (abfd, ".rel.plt");
  if (!htab->sdynbss
      || !htab->srelplt
      || !htab->splt)
    abort ();

  return TRUE;
}

/* Look through the relocs for a section during the first phase, and
   allocate space in the global offset table.  */

bfd_boolean
_bfd_riscv_elf_check_relocs (bfd *abfd, struct bfd_link_info *info,
			    asection *sec, const Elf_Internal_Rela *relocs)
{
  const char *name;
  bfd *dynobj;
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  size_t extsymoff;
  const Elf_Internal_Rela *rel;
  const Elf_Internal_Rela *rel_end;
  asection *sreloc;
  const struct elf_backend_data *bed;

  if (info->relocatable)
    return TRUE;

  dynobj = elf_hash_table (info)->dynobj;
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);
  extsymoff = (elf_bad_symtab (abfd)) ? 0 : symtab_hdr->sh_info;

  bed = get_elf_backend_data (abfd);
  rel_end = relocs + sec->reloc_count * bed->s->int_rels_per_ext_rel;

  name = bfd_get_section_name (abfd, sec);

  sreloc = NULL;
  for (rel = relocs; rel < rel_end; ++rel)
    {
      unsigned long r_symndx;
      unsigned int r_type;
      struct elf_link_hash_entry *h;
      bfd_boolean can_make_dynamic_p;

      r_symndx = ELF_R_SYM (abfd, rel->r_info);
      r_type = ELF_R_TYPE (abfd, rel->r_info);

      if (r_symndx < extsymoff)
	h = NULL;
      else if (r_symndx >= extsymoff + NUM_SHDR_ENTRIES (symtab_hdr))
	{
	  (*_bfd_error_handler)
	    (_("%B: Malformed reloc detected for section %s"),
	     abfd, name);
	  bfd_set_error (bfd_error_bad_value);
	  return FALSE;
	}
      else
	{
	  h = sym_hashes[r_symndx - extsymoff];
	  while (h != NULL
		 && (h->root.type == bfd_link_hash_indirect
		     || h->root.type == bfd_link_hash_warning))
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;
	}

      /* Set CAN_MAKE_DYNAMIC_P to true if we can convert this
	 relocation into a dynamic one.  */
      can_make_dynamic_p = FALSE;
      switch (r_type)
	{
	case R_RISCV_GOT_HI20:
	case R_RISCV_GOT_LO12:
	case R_RISCV_TLS_GOT_HI20:
	case R_RISCV_TLS_GOT_LO12:
	case R_RISCV_TLS_GD_HI20:
	case R_RISCV_TLS_GD_LO12:
	case R_RISCV_TLS_IE_HI20:
	case R_RISCV_TLS_IE_LO12:
	case R_RISCV_TLS_IE_ADD:
	case R_RISCV_TLS_IE_LO12_I:
	case R_RISCV_TLS_IE_LO12_S:
	  if (dynobj == NULL)
	    elf_hash_table (info)->dynobj = dynobj = abfd;
	  if (!riscv_elf_create_got_section (dynobj, info))
	    return FALSE;
	  break;

	case R_RISCV_32:
	case R_RISCV_REL32:
	case R_RISCV_64:
	  /* For executables that use PLTs and copy-relocs, we have a
	     choice between converting the relocation into a dynamic
	     one or using copy relocations or PLT entries.  It is
	     usually better to do the former, unless the relocation is
	     against a read-only section.  */
	  if ((info->shared
	       || (h != NULL
		   && !(!info->nocopyreloc && RISCV_ELF_READONLY_SECTION (sec))))
	      && (sec->flags & SEC_ALLOC) != 0)
	    {
	      can_make_dynamic_p = TRUE;
	      if (dynobj == NULL)
		elf_hash_table (info)->dynobj = dynobj = abfd;
	      break;
	    }
	  /* For sections that are not SEC_ALLOC a copy reloc would be
	     output if possible (implying questionable semantics for
	     read-only data objects) or otherwise the final link would
	     fail as ld.so will not process them and could not therefore
	     handle any outstanding dynamic relocations.

	     For such sections that are also SEC_DEBUGGING, we can avoid
	     these problems by simply ignoring any relocs as these
	     sections have a predefined use and we know it is safe to do
	     so.

	     This is needed in cases such as a global symbol definition
	     in a shared library causing a common symbol from an object
	     file to be converted to an undefined reference.  If that
	     happens, then all the relocations against this symbol from
	     SEC_DEBUGGING sections in the object file will resolve to
	     nil.  */
	  if ((sec->flags & SEC_DEBUGGING) != 0)
	    break;
	  /* Fall through.  */

	default:
	  /* Most static relocations require pointer equality, except
	     for branches.  */
	  if (h)
	    h->pointer_equality_needed = TRUE;
	  /* Fall through.  */

	case R_RISCV_JAL:
	case R_RISCV_CALL:
	case R_RISCV_CALL_PLT:
	case R_RISCV_BRANCH:
	  if (h)
	    ((struct riscv_elf_link_hash_entry *) h)->has_static_relocs = TRUE;
	  break;
	}

      switch (r_type)
	{
	case R_RISCV_GOT_LO12:
	  if (!riscv_elf_record_local_got_symbol (abfd, r_symndx,
						 rel->r_addend, info, 0))
	    return FALSE;
	  /* Fall through. */
	case R_RISCV_GOT_HI20:
	  if (h && !riscv_elf_record_global_got_symbol (h, abfd, info, 0))
	    return FALSE;
	  break;

	case R_RISCV_TLS_GOT_HI20:
	case R_RISCV_TLS_GOT_LO12:
	case R_RISCV_TLS_IE_HI20:
	case R_RISCV_TLS_IE_LO12:
	  if (info->shared)
	    info->flags |= DF_STATIC_TLS;
	  /* Fall through */

	case R_RISCV_TLS_GD_HI20:
	case R_RISCV_TLS_GD_LO12:
	  /* This symbol requires a global offset table entry, or two
	     for TLS GD relocations.  */
	  {
	    unsigned char flag = (TLS_GD_RELOC_P(r_type)
				  ? GOT_TLS_GD
				  : GOT_TLS_IE);
	    if (h != NULL)
	      {
		struct riscv_elf_link_hash_entry *hriscv =
		  (struct riscv_elf_link_hash_entry *) h;
		hriscv->tls_type |= flag;

		if (h && !riscv_elf_record_global_got_symbol (h, abfd, info, flag))
		  return FALSE;
	      }
	    else
	      {
		BFD_ASSERT (r_symndx != STN_UNDEF);

		if (!riscv_elf_record_local_got_symbol (abfd, r_symndx,
						       rel->r_addend,
						       info, flag))
		  return FALSE;
	      }
	  }
	  break;

	case R_RISCV_32:
	case R_RISCV_REL32:
	case R_RISCV_64:
	  /* In VxWorks executables, references to external symbols
	     are handled using copy relocs or PLT stubs, so there's
	     no need to add a .rela.dyn entry for this relocation.  */
	  if (can_make_dynamic_p)
	    {
	      if (sreloc == NULL)
		{
		  sreloc = riscv_elf_rel_dyn_section (info, TRUE);
		  if (sreloc == NULL)
		    return FALSE;
		}
	      if (info->shared && h == NULL)
		{
		  /* When creating a shared object, we must copy these
		     reloc types into the output file as R_RISCV_REL32
		     relocs.  Make room for this reloc in .rel(a).dyn.  */
		  riscv_elf_allocate_dynamic_relocations (dynobj, info, 1);
		  if (RISCV_ELF_READONLY_SECTION (sec))
		    /* We tell the dynamic linker that there are
		       relocations against the text segment.  */
		    info->flags |= DF_TEXTREL;
		}
	      else
		{
		  struct riscv_elf_link_hash_entry *hriscv;

		  /* For a shared object, we must copy this relocation
		     unless the symbol turns out to be undefined and
		     weak with non-default visibility, in which case
		     it will be left as zero.

		     We could elide R_RISCV_REL32 for locally binding symbols
		     in shared libraries, but do not yet do so.

		     For an executable, we only need to copy this
		     reloc if the symbol is defined in a dynamic
		     object.  */
		  hriscv = (struct riscv_elf_link_hash_entry *) h;
		  ++hriscv->possibly_dynamic_relocs;
		  if (RISCV_ELF_READONLY_SECTION (sec))
		    /* We need it to tell the dynamic linker if there
		       are relocations against the text segment.  */
		    hriscv->readonly_reloc = TRUE;
		}
	    }

	  break;

	case R_RISCV_CALL_PLT:
	  if (info->shared && h)
	    h->needs_plt = TRUE;
	  break;

	case R_RISCV_HI20:
	case R_RISCV_TPREL_HI20:
	  /* Can't use these in a shared library. */
	  if (info->shared)
	    {
	      reloc_howto_type *howto = riscv_elf_rtype_to_howto (r_type);
	      (*_bfd_error_handler)
		(_("%B: relocation %s against `%s' can not be used when making a shared object; recompile with -fPIC"),
		  abfd, howto->name,
		  (h) ? h->root.root.string : "a local symbol");
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }
	  break;

	default:
	  break;
	}
    }

  return TRUE;
}

/* Allocate space for global sym dynamic relocs.  */

static bfd_boolean
allocate_dynrelocs (struct elf_link_hash_entry *h, void *inf)
{
  struct bfd_link_info *info = inf;
  bfd *dynobj;
  struct riscv_elf_link_hash_entry *hriscv;
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  dynobj = elf_hash_table (info)->dynobj;
  hriscv = (struct riscv_elf_link_hash_entry *) h;

  /* Ignore indirect and warning symbols.  All relocations against
     such symbols will be redirected to the target symbol.  */
  if (h->root.type == bfd_link_hash_indirect
      || h->root.type == bfd_link_hash_warning)
    return TRUE;

  /* If this symbol is defined in a dynamic object, or we are creating
     a shared library, we will need to copy any R_RISCV_32 or
     R_RISCV_REL32 relocs against it into the output file.  */
  if (! info->relocatable
      && hriscv->possibly_dynamic_relocs != 0
      && (h->root.type == bfd_link_hash_defweak
	  || !h->def_regular
	  || info->shared))
    {
      bfd_boolean do_copy = TRUE;

      if (h->root.type == bfd_link_hash_undefweak)
	{
	  /* Do not copy relocations for undefined weak symbols with
	     non-default visibility.  */
	  if (ELF_ST_VISIBILITY (h->other) != STV_DEFAULT)
	    do_copy = FALSE;

	  /* Make sure undefined weak symbols are output as a dynamic
	     symbol in PIEs.  */
	  else if (h->dynindx == -1 && !h->forced_local)
	    {
	      if (! bfd_elf_link_record_dynamic_symbol (info, h))
		return FALSE;
	    }
	}

      if (do_copy)
	{
	  /* Even though we don't directly need a GOT entry for this symbol,
	     the SVR4 psABI requires it to have a dynamic symbol table
	     index greater that DT_RISCV_GOTSYM if there are dynamic
	     relocations against it. */
	  if (hriscv->global_got_area > GGA_RELOC_ONLY)
	    hriscv->global_got_area = GGA_RELOC_ONLY;

	  riscv_elf_allocate_dynamic_relocations
	    (dynobj, info, hriscv->possibly_dynamic_relocs);
	  if (hriscv->readonly_reloc)
	    /* We tell the dynamic linker that there are relocations
	       against the text segment.  */
	    info->flags |= DF_TEXTREL;
	}
    }

  return TRUE;
}

/* Adjust a symbol defined by a dynamic object and referenced by a
   regular object.  The current definition is in some section of the
   dynamic object, but we're not including those sections.  We have to
   change the definition to something the rest of the link can
   understand.  */

bfd_boolean
_bfd_riscv_elf_adjust_dynamic_symbol (struct bfd_link_info *info,
				     struct elf_link_hash_entry *h)
{
  bfd *dynobj;
  struct riscv_elf_link_hash_entry *hriscv;
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  dynobj = elf_hash_table (info)->dynobj;
  hriscv = (struct riscv_elf_link_hash_entry *) h;

  /* Make sure we know what is going on here.  */
  BFD_ASSERT (dynobj != NULL
	      && (h->needs_plt
		  || h->type == STT_GNU_IFUNC
		  || h->u.weakdef != NULL
		  || (h->def_dynamic
		      && h->ref_regular
		      && !h->def_regular)));

  hriscv = (struct riscv_elf_link_hash_entry *) h;

  /* Establish PLT entries for functions that don't bind locally. */
  if ((h->type == STT_FUNC || h->type == STT_GNU_IFUNC)
      && hriscv->has_static_relocs
      && !SYMBOL_CALLS_LOCAL (info, h)
      && !(ELF_ST_VISIBILITY (h->other) != STV_DEFAULT
	   && h->root.type == bfd_link_hash_undefweak))
    {
      /* We'll turn this into an actual address once we know the PLT size. */
      h->plt.offset = htab->nplt++;

      return TRUE;
    }

  /* If this is a weak symbol, and there is a real definition, the
     processor independent code will have arranged for us to see the
     real definition first, and we can just use the same value.  */
  if (h->u.weakdef != NULL)
    {
      BFD_ASSERT (h->u.weakdef->root.type == bfd_link_hash_defined
		  || h->u.weakdef->root.type == bfd_link_hash_defweak);
      h->root.u.def.section = h->u.weakdef->root.u.def.section;
      h->root.u.def.value = h->u.weakdef->root.u.def.value;
      return TRUE;
    }

  if (!info->shared && !h->def_regular && hriscv->has_static_relocs)
    {
     /* We must allocate the symbol in our .dynbss section, which will
	become part of the .bss section of the executable.  There will be
	an entry for this symbol in the .dynsym section.  The dynamic
	object will contain position independent code, so all references
	from the dynamic object to this symbol will go through the global
	offset table.  The dynamic linker will use the .dynsym entry to
	determine the address it must put in the global offset table, so
	both the dynamic object and the regular object will refer to the
	same memory location for the variable.  */

      if ((h->root.u.def.section->flags & SEC_ALLOC) != 0)
	{
	  riscv_elf_allocate_dynamic_relocations (dynobj, info, 1);
	  h->needs_copy = 1;
	}

      /* All relocations against this symbol that could have been made
	 dynamic will now refer to the local copy instead.  */
      hriscv->possibly_dynamic_relocs = 0;

      return _bfd_elf_adjust_dynamic_copy (h, htab->sdynbss);
    }

  return TRUE;
}

bfd_boolean
_bfd_riscv_elf_always_size_sections (bfd *output_bfd ATTRIBUTE_UNUSED,
				     struct bfd_link_info *info ATTRIBUTE_UNUSED)
{
  return TRUE;
}

static struct bfd_link_hash_entry *
bfd_riscv_gp_hash (struct bfd_link_info *info)
{
  if (info->shared)
    return NULL;
  return bfd_link_hash_lookup (info->hash, "_gp", FALSE, FALSE, TRUE);
}

static bfd_vma
bfd_riscv_init_gp_value (bfd *abfd, struct bfd_link_info *info)
{
  struct bfd_link_hash_entry *h = bfd_riscv_gp_hash (info);

  if (h != NULL && h->type == bfd_link_hash_defined)
    elf_gp (abfd) = h->u.def.value + sec_addr(h->u.def.section);

  return elf_gp (abfd);
}

/* After the PLT has been sized, compute PLT entry offsets. */

static int
riscv_elf_compute_plt_offset (void *arg1, void *arg2)
{
  struct elf_link_hash_entry *h = arg1;
  struct riscv_elf_link_hash_table *htab = arg2;

  if (h->plt.offset != MINUS_ONE)
    {
      h->plt.offset = h->plt.offset * PLT_ENTRY_SIZE + PLT_HEADER_SIZE;

      /* If the output file has no definition of the symbol, set the
	 symbol's value to the address of the stub.  */
      if (!h->def_regular)
	{
	  h->root.u.def.section = htab->splt;
	  h->root.u.def.value = h->plt.offset;
	}
    }

  return 1;
}

/* If the link uses a GOT, lay it out and work out its size.  */

static bfd_boolean
riscv_elf_lay_out_got (bfd *output_bfd, struct bfd_link_info *info)
{
  bfd *dynobj;
  asection *s;
  struct riscv_got_info *g;
  bfd_size_type loadable_size = 0;
  bfd *sub;
  struct riscv_elf_count_tls_arg count_tls_arg;
  struct riscv_elf_link_hash_table *htab;
  struct riscv_elf_count_tls_arg arg;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  s = htab->sgot;
  if (s == NULL)
    return TRUE;

  dynobj = elf_hash_table (info)->dynobj;
  g = htab->got_info;

  /* Allocate room for the reserved entries. */
  BFD_ASSERT (g->assigned_gotno == 0);
  htab->reserved_gotno = 2;
  g->local_gotno += htab->reserved_gotno;
  g->assigned_gotno = htab->reserved_gotno;

  /* Replace entries for indirect and warning symbols with entries for
     the target symbol.  */
  if (!riscv_elf_resolve_final_got_entries (g))
    return FALSE;

  /* Count the number of GOT symbols.  */
  riscv_elf_link_hash_traverse (htab, riscv_elf_count_got_symbols, info);

  /* Calculate the total loadable size of the output.  That
     will give us the maximum number of GOT_PAGE entries
     required.  */
  for (sub = info->input_bfds; sub; sub = sub->link_next)
    {
      asection *subsection;

      for (subsection = sub->sections;
	   subsection;
	   subsection = subsection->next)
	{
	  if ((subsection->flags & SEC_ALLOC) == 0)
	    continue;
	  loadable_size += ((subsection->size + 0xf)
			    &~ (bfd_size_type) 0xf);
	}
    }

  s->size += g->local_gotno * RISCV_ELF_GOT_SIZE (output_bfd);
  s->size += g->global_gotno * RISCV_ELF_GOT_SIZE (output_bfd);

  /* We need to calculate tls_gotno for global symbols at this point
     instead of building it up earlier, to avoid doublecounting
     entries for one global symbol from multiple input files.  */
  count_tls_arg.info = info;
  count_tls_arg.needed = 0;
  elf_link_hash_traverse (elf_hash_table (info),
			  riscv_elf_count_global_tls_entries,
			  &count_tls_arg);
  g->tls_gotno += count_tls_arg.needed;
  s->size += g->tls_gotno * RISCV_ELF_GOT_SIZE (output_bfd);

  /* Set up TLS entries.  */
  g->tls_assigned_gotno = g->global_gotno + g->local_gotno;
  htab_traverse (g->got_entries, riscv_elf_initialize_tls_index, g);

  /* Allocate room for the TLS relocations.  */
  arg.info = info;
  arg.needed = 0;
  htab_traverse (g->got_entries, riscv_elf_count_local_tls_relocs, &arg);
  elf_link_hash_traverse (elf_hash_table (info),
			riscv_elf_count_global_tls_relocs, &arg);
  if (arg.needed)
    riscv_elf_allocate_dynamic_relocations (dynobj, info, arg.needed);

  return TRUE;
}

/* Set the sizes of the dynamic sections.  */

bfd_boolean
_bfd_riscv_elf_size_dynamic_sections (bfd *output_bfd,
				     struct bfd_link_info *info)
{
  bfd *dynobj;
  asection *s, *sreldyn;
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);
  dynobj = elf_hash_table (info)->dynobj;
  BFD_ASSERT (dynobj != NULL);

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* If we have a PLT, size it and create its symbol. */
      if (htab->nplt != 0 && htab->root.hplt == NULL)
	{
	  struct elf_link_hash_entry *h;

	  BFD_ASSERT (htab->splt->size == 0);
	  htab->splt->size = PLT_HEADER_SIZE + htab->nplt * PLT_ENTRY_SIZE;

	  /* The last and first two entries in .got.plt are reserved.  */
	  BFD_ASSERT (htab->sgotplt->size == 0);
	  htab->sgotplt->size = (3 + htab->nplt) * RISCV_ELF_GOT_SIZE (dynobj);

	  /* Make room for the R_RISCV_JUMP_SLOT relocations. */
	  BFD_ASSERT (htab->srelplt->size == 0);
	  htab->srelplt->size = htab->nplt * RISCV_ELF_REL_SIZE (dynobj);

	  /* Adjust the PLT offsets. */
	  elf_link_hash_traverse (elf_hash_table (info),
				  riscv_elf_compute_plt_offset,
				  htab);

	  /* PLT entries are 16 bytes.  Don't let them span I$ lines. */
	  if (!bfd_set_section_alignment (dynobj, htab->splt, 4))
	    return FALSE;

	  /* The PLT header requires .got.plt be 2-word aligned. */
	  if (!bfd_set_section_alignment (dynobj, htab->sgotplt,
					  RISCV_ELF_LOG_FILE_ALIGN (dynobj)+1))
	    return FALSE;

	  /* Make the symbol. */
	  h = _bfd_elf_define_linkage_sym (dynobj, info, htab->splt,
					   "_PROCEDURE_LINKAGE_TABLE_");
	  htab->root.hplt = h;
	  if (h == NULL)
	    return FALSE;
	  h->type = STT_FUNC;
	}

      /* Set the contents of the .interp section to the interpreter.  */
      if (info->executable)
	{
	  s = bfd_get_section_by_name (dynobj, ".interp");
	  BFD_ASSERT (s != NULL);
	  s->size
	    = strlen (ELF_DYNAMIC_INTERPRETER (output_bfd)) + 1;
	  s->contents
	    = (bfd_byte *) ELF_DYNAMIC_INTERPRETER (output_bfd);
	}
    }

  /* Allocate space for global sym dynamic relocs.  */
  elf_link_hash_traverse (&htab->root, allocate_dynrelocs, (PTR) info);

  if (!riscv_elf_lay_out_got (output_bfd, info))
    return FALSE;

  /* The check_relocs and adjust_dynamic_symbol entry points have
     determined the sizes of the various dynamic sections.  Allocate
     memory for them.  */
  for (s = dynobj->sections; s != NULL; s = s->next)
    {
      const char *name;

      /* It's OK to base decisions on the section name, because none
	 of the dynobj section names depend upon the input files.  */
      name = bfd_get_section_name (dynobj, s);

      if ((s->flags & SEC_LINKER_CREATED) == 0)
	continue;

      if (CONST_STRNEQ (name, ".rel"))
	{
	  if (s->size != 0)
	    {
	      /* We use the reloc_count field as a counter if we need
		 to copy relocs into the output file.  */
	      if (strcmp (name, RISCV_ELF_REL_DYN_NAME (info)) != 0)
		s->reloc_count = 0;

	      /* If combreloc is enabled, elf_link_sort_relocs() will
		 sort relocations, but in a different way than we do,
		 and before we're done creating relocations.  Also, it
		 will move them around between input sections'
		 relocation's contents, so our sorting would be
		 broken, so don't let it run.  */
	      info->combreloc = 0;
	    }
	}
      else if (s == htab->splt)
	{
	}
      else if (! CONST_STRNEQ (name, ".init")
	       && s != htab->sgot
	       && s != htab->sgotplt
	       && s != htab->sdynbss)
	{
	  /* It's not one of our sections, so don't allocate space.  */
	  continue;
	}

      if (s->size == 0)
	{
	  s->flags |= SEC_EXCLUDE;
	  continue;
	}

      if ((s->flags & SEC_HAS_CONTENTS) == 0)
	continue;

      /* Allocate memory for the section contents.  */
      s->contents = bfd_zalloc (dynobj, s->size);
      if (s->contents == NULL)
	{
	  bfd_set_error (bfd_error_no_memory);
	  return FALSE;
	}
    }

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* Add some entries to the .dynamic section.  We fill in the
	 values later, in _bfd_riscv_elf_finish_dynamic_sections, but we
	 must add the entries now so that we get the correct size for
	 the .dynamic section.  */

      /* The DT_DEBUG entry may be filled in by the dynamic linker and
	 used by the debugger.  */
      if (info->executable
	  && !_bfd_elf_add_dynamic_entry (info, DT_DEBUG, 0))
	return FALSE;

      if ((info->flags & DF_TEXTREL) != 0)
	{
	  if (! _bfd_elf_add_dynamic_entry (info, DT_TEXTREL, 0))
	    return FALSE;

	  /* Clear the DF_TEXTREL flag.  It will be set again if we
	     write out an actual text relocation; we may not, because
	     at this point we do not know whether e.g. any .eh_frame
	     absolute relocations have been converted to PC-relative.  */
	  info->flags &= ~DF_TEXTREL;
	}

      if (! _bfd_elf_add_dynamic_entry (info, DT_PLTGOT, 0))
	return FALSE;

      sreldyn = riscv_elf_rel_dyn_section (info, FALSE);
	{
	  if (sreldyn && sreldyn->size > 0)
	    {
	      if (! _bfd_elf_add_dynamic_entry (info, DT_REL, 0))
		return FALSE;

	      if (! _bfd_elf_add_dynamic_entry (info, DT_RELSZ, 0))
		return FALSE;

	      if (! _bfd_elf_add_dynamic_entry (info, DT_RELENT, 0))
		return FALSE;
	    }

	  if (! _bfd_elf_add_dynamic_entry (info, DT_RISCV_LOCAL_GOTNO, 0))
	    return FALSE;

	  if (! _bfd_elf_add_dynamic_entry (info, DT_RISCV_SYMTABNO, 0))
	    return FALSE;

	  if (! _bfd_elf_add_dynamic_entry (info, DT_RISCV_GOTSYM, 0))
	    return FALSE;
	}
      if (htab->splt->size > 0)
	{
	  if (! _bfd_elf_add_dynamic_entry (info, DT_PLTREL, 0))
	    return FALSE;

	  if (! _bfd_elf_add_dynamic_entry (info, DT_JMPREL, 0))
	    return FALSE;

	  if (! _bfd_elf_add_dynamic_entry (info, DT_PLTRELSZ, 0))
	    return FALSE;

	  if (! _bfd_elf_add_dynamic_entry (info, DT_RISCV_PLTGOT, 0))
	    return FALSE;
	}
    }

  return TRUE;
}

/* REL is a relocation in INPUT_BFD that is being copied to OUTPUT_BFD.
   Adjust its R_ADDEND field so that it is correct for the output file.
   LOCAL_SYMS and LOCAL_SECTIONS are arrays of INPUT_BFD's local symbols
   and sections respectively; both use symbol indexes.  */

static void
riscv_elf_adjust_addend (bfd *output_bfd, struct bfd_link_info *info,
			bfd *input_bfd, Elf_Internal_Sym *local_syms,
			asection **local_sections, Elf_Internal_Rela *rel)
{
  unsigned int r_symndx;
  Elf_Internal_Sym *sym;
  asection *sec;

  if (riscv_elf_local_relocation_p (input_bfd, rel, local_sections))
    {
      r_symndx = ELF_R_SYM (output_bfd, rel->r_info);
      sym = local_syms + r_symndx;

      /* Adjust REL's addend to account for section merging.  */
      if (!info->relocatable)
	{
	  sec = local_sections[r_symndx];
	  _bfd_elf_rela_local_sym (output_bfd, sym, &sec, rel);
	}

      /* This would normally be done by the rela_normal code in elflink.c.  */
      if (ELF_ST_TYPE (sym->st_info) == STT_SECTION)
	rel->r_addend += local_sections[r_symndx]->output_offset;
    }
}


/* Handle relocations against symbols from removed linkonce sections,
   or sections discarded by a linker script.  We use this wrapper around
   RELOC_AGAINST_DISCARDED_SECTION to handle triplets of compound relocs
   on 64-bit ELF targets.  In this case for any relocation handled, which
   always be the first in a triplet, the remaining two have to be processed
   together with the first, even if they are R_RISCV_NONE.  It is the symbol
   index referred by the first reloc that applies to all the three and the
   remaining two never refer to an object symbol.  And it is the final
   relocation (the last non-null one) that determines the output field of
   the whole relocation so retrieve the corresponding howto structure for
   the relocatable field to be cleared by RELOC_AGAINST_DISCARDED_SECTION.

   Note that RELOC_AGAINST_DISCARDED_SECTION is a macro that uses "continue"
   and therefore requires to be pasted in a loop.  It also defines a block
   and does not protect any of its arguments, hence the extra brackets.  */

static void
riscv_reloc_against_discarded_section (bfd *output_bfd,
				       struct bfd_link_info *info,
				       bfd *input_bfd, asection *input_section,
				       Elf_Internal_Rela **rel,
				       const Elf_Internal_Rela **relend,
				       reloc_howto_type *howto,
				       bfd_byte *contents)
{
  const struct elf_backend_data *bed = get_elf_backend_data (output_bfd);
  int count = bed->s->int_rels_per_ext_rel;
  unsigned int r_type;
  int i;

  for (i = count - 1; i > 0; i--)
    {
      r_type = ELF_R_TYPE (output_bfd, (*rel)[i].r_info);
      if (r_type != R_RISCV_NONE)
	{
	  howto = riscv_elf_rtype_to_howto (r_type);
	  break;
	}
    }
  do
    {
       RELOC_AGAINST_DISCARDED_SECTION (info, input_bfd, input_section,
					(*rel), count, (*relend),
					howto, i, contents);
    }
  while (0);
}

/* Relocate a RISC-V ELF section.  */

bfd_boolean
_bfd_riscv_elf_relocate_section (bfd *output_bfd, struct bfd_link_info *info,
				bfd *input_bfd, asection *input_section,
				bfd_byte *contents, Elf_Internal_Rela *relocs,
				Elf_Internal_Sym *local_syms,
				asection **local_sections)
{
  Elf_Internal_Rela *rel;
  const Elf_Internal_Rela *relend;
  bfd_vma addend = 0;
  const struct elf_backend_data *bed;
  bfd_boolean ret = FALSE;
  riscv_pcrel_relocs pcrel_relocs;

  if (!riscv_init_pcrel_relocs (&pcrel_relocs))
    return FALSE;

  bed = get_elf_backend_data (output_bfd);
  relend = relocs + input_section->reloc_count * bed->s->int_rels_per_ext_rel;
  for (rel = relocs; rel < relend; ++rel)
    {
      const char *name;
      bfd_vma value = 0;
      /* TRUE if the relocation is a RELA relocation, rather than a
         REL relocation.  */
      const char *msg;
      unsigned long r_symndx;
      asection *sec;
      Elf_Internal_Shdr *symtab_hdr;
      struct elf_link_hash_entry *h;
      unsigned int r_type = ELF_R_TYPE (output_bfd, rel->r_info);
      reloc_howto_type *howto = riscv_elf_rtype_to_howto (r_type);

      r_symndx = ELF_R_SYM (input_bfd, rel->r_info);
      symtab_hdr = &elf_tdata (input_bfd)->symtab_hdr;
      if (riscv_elf_local_relocation_p (input_bfd, rel, local_sections))
	{
	  sec = local_sections[r_symndx];
	  h = NULL;
	}
      else
	{
	  unsigned long extsymoff;

	  extsymoff = 0;
	  if (!elf_bad_symtab (input_bfd))
	    extsymoff = symtab_hdr->sh_info;
	  h = elf_sym_hashes (input_bfd) [r_symndx - extsymoff];
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;

	  sec = NULL;
	  if (h->root.type == bfd_link_hash_defined
	      || h->root.type == bfd_link_hash_defweak)
	    sec = h->root.u.def.section;
	}

      if (sec != NULL && discarded_section (sec))
	{
	  riscv_reloc_against_discarded_section (output_bfd, info, input_bfd,
						 input_section, &rel, &relend,
						 howto, contents);
	  continue;
	}

      addend = rel->r_addend;
      riscv_elf_adjust_addend (output_bfd, info, input_bfd,
			      local_syms, local_sections, rel);

      if (info->relocatable)
	/* Go on to the next relocation.  */
	continue;

      /* Figure out what value we are supposed to relocate.  */
      switch (riscv_elf_calculate_relocation (output_bfd, input_section, info,
					      &pcrel_relocs, rel, addend, howto,
					      local_syms, local_sections,
					      &value, &name, contents))
	{
	case bfd_reloc_continue:
	  /* There's nothing to do.  */
	  continue;

	case bfd_reloc_undefined:
	  /* riscv_elf_calculate_relocation already called the
	     undefined_symbol callback.  There's no real point in
	     trying to perform the relocation at this point, so we
	     just skip ahead to the next relocation.  */
	  continue;

	case bfd_reloc_notsupported:
	  msg = _("internal error: unsupported relocation error");
	  info->callbacks->warning
	    (info, msg, name, input_bfd, input_section, rel->r_offset);
	  goto out;

	case bfd_reloc_overflow:
	  BFD_ASSERT (name != NULL);
	  if (! ((*info->callbacks->reloc_overflow)
		  (info, NULL, name, howto->name, (bfd_vma) 0,
		  input_bfd, input_section, rel->r_offset)))
	    goto out;
	  break;

	case bfd_reloc_ok:
	  break;

	default:
	  abort ();
	  break;
	}

      /* Actually perform the relocation.  */
      if (! riscv_elf_perform_relocation (howto, rel, value, input_bfd,
					  contents))
	goto out;
    }

  ret = riscv_resolve_pcrel_lo_relocs (&pcrel_relocs);
out:
  riscv_free_pcrel_relocs (&pcrel_relocs);
  return ret;
}

/* Finish up dynamic symbol handling.  We set the contents of various
   dynamic sections here.  */

bfd_boolean
_bfd_riscv_elf_finish_dynamic_symbol (bfd *output_bfd,
				     struct bfd_link_info *info,
				     struct elf_link_hash_entry *h,
				     Elf_Internal_Sym *sym)
{
  bfd *dynobj;
  asection *sgot;
  struct riscv_got_info *g;
  const char *name;
  struct riscv_elf_link_hash_table *htab;
  struct riscv_elf_link_hash_entry *hriscv;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);
  dynobj = elf_hash_table (info)->dynobj;
  hriscv = (struct riscv_elf_link_hash_entry *) h;

  if (h->plt.offset != MINUS_ONE)
    {
      /* We've decided to create a PLT entry for this symbol.  */
      bfd_byte *loc;
      bfd_vma i, header_address, plt_index, got_address, got_val;
      uint32_t plt_entry[PLT_ENTRY_INSNS];

      BFD_ASSERT (h->dynindx != -1);
      BFD_ASSERT (htab->splt != NULL);
      BFD_ASSERT (h->plt.offset <= htab->splt->size);

      /* Calculate the address of the PLT header.  */
      header_address = sec_addr(htab->splt);

      /* Calculate the index of the entry.  */
      plt_index = (h->plt.offset - PLT_HEADER_SIZE) / PLT_ENTRY_SIZE;

      /* Calculate the address of the .got.plt entry.  */
      got_address = riscv_elf_got_plt_val (plt_index, info);

      /* Find out where the .plt entry should go.  */
      loc = htab->splt->contents + h->plt.offset;

      /* Fill in the PLT entry itself.  */
      got_val = riscv_make_plt_entry (output_bfd, got_address, header_address,
				      header_address + h->plt.offset,
				      plt_entry);
      for (i = 0; i < PLT_ENTRY_INSNS; i++)
        bfd_put_32 (output_bfd, plt_entry[i], loc + 4*i);

      /* Fill in the initial value of the .got.plt entry. */
      loc = htab->sgotplt->contents + (got_address - sec_addr(htab->sgotplt));
      RISCV_ELF_PUT_WORD (output_bfd, got_val, loc);

      /* Emit an R_RISCV_JUMP_SLOT relocation against the .got.plt entry.  */
      riscv_elf_output_dynamic_relocation (output_bfd, htab->srelplt,
					  plt_index, h->dynindx,
					  R_RISCV_JUMP_SLOT, got_address);

      if (!h->def_regular)
	sym->st_shndx = SHN_UNDEF;
    }

  BFD_ASSERT (h->dynindx != -1
	      || h->forced_local);

  sgot = htab->sgot;
  g = htab->got_info;
  BFD_ASSERT (g != NULL);

  /* Run through the global symbol table, creating GOT entries for all
     the symbols that need them.  */
  if (hriscv->global_got_area != GGA_NONE)
    {
      bfd_vma offset;
      bfd_vma value;

      value = sym->st_value;
      offset = riscv_elf_global_got_index (dynobj, h, R_RISCV_GOT_HI20, info);
      RISCV_ELF_PUT_WORD (output_bfd, value, sgot->contents + offset);
    }

  /* Mark _DYNAMIC and _GLOBAL_OFFSET_TABLE_ as absolute.  */
  name = h->root.root.string;
  if (strcmp (name, "_DYNAMIC") == 0
      || h == elf_hash_table (info)->hgot)
    sym->st_shndx = SHN_ABS;
  else if (strcmp (name, "_DYNAMIC_LINKING") == 0)
    {
      sym->st_shndx = SHN_ABS;
      sym->st_info = ELF_ST_INFO (STB_GLOBAL, STT_SECTION);
      sym->st_value = 1;
    }

  /* Emit a copy reloc, if needed.  */
  if (h->needs_copy)
    {
      asection *s;
      bfd_vma symval;

      BFD_ASSERT (h->dynindx != -1);

      s = riscv_elf_rel_dyn_section (info, FALSE);
      symval = sec_addr(h->root.u.def.section) + h->root.u.def.value;
      riscv_elf_output_dynamic_relocation (output_bfd, s, s->reloc_count++,
					  h->dynindx, R_RISCV_COPY, symval);
    }

  return TRUE;
}

/* Write out a plt0 entry to the beginning of .plt.  */

static void
riscv_finish_exec_plt (bfd *output_bfd, struct bfd_link_info *info)
{
  bfd_byte *loc;
  uint32_t plt_entry[PLT_HEADER_INSNS];
  struct riscv_elf_link_hash_table *htab;
  int i;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  /* Install the PLT header.  */
  loc = htab->splt->contents;
  riscv_make_plt0_entry (output_bfd, sec_addr(htab->sgotplt),
			 sec_addr(htab->splt), plt_entry);
  for (i = 0; i < PLT_HEADER_INSNS; i++)
    bfd_put_32 (output_bfd, plt_entry[i], loc + 4*i);
}

/* Finish up the dynamic sections.  */

bfd_boolean
_bfd_riscv_elf_finish_dynamic_sections (bfd *output_bfd,
				       struct bfd_link_info *info)
{
  bfd *dynobj;
  asection *sdyn;
  asection *sgot;
  struct riscv_got_info *gg, *g;
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  dynobj = elf_hash_table (info)->dynobj;

  sdyn = bfd_get_section_by_name (dynobj, ".dynamic");

  sgot = htab->sgot;
  g = gg = htab->got_info;

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      bfd_byte *b;
      int dyn_to_skip = 0, dyn_skipped = 0;

      BFD_ASSERT (sdyn != NULL);
      BFD_ASSERT (gg != NULL);

      for (b = sdyn->contents;
	   b < sdyn->contents + sdyn->size;
	   b += RISCV_ELF_DYN_SIZE (dynobj))
	{
	  Elf_Internal_Dyn dyn;
	  const char *name;
	  size_t elemsize;
	  asection *s;
	  bfd_boolean swap_out_p;

	  /* Read in the current dynamic entry.  */
	  (*get_elf_backend_data (dynobj)->s->swap_dyn_in) (dynobj, b, &dyn);

	  /* Assume that we're going to modify it and write it out.  */
	  swap_out_p = TRUE;

	  switch (dyn.d_tag)
	    {
	    case DT_RELENT:
	      dyn.d_un.d_val = RISCV_ELF_REL_SIZE (dynobj);
	      break;

	    case DT_STRSZ:
	      /* Rewrite DT_STRSZ.  */
	      dyn.d_un.d_val =
		_bfd_elf_strtab_size (elf_hash_table (info)->dynstr);
	      break;

	    case DT_PLTGOT:
	      dyn.d_un.d_ptr = sec_addr(htab->sgot);
	      break;

	    case DT_RISCV_PLTGOT:
	      dyn.d_un.d_ptr = sec_addr(htab->sgotplt);
	      break;

	    case DT_RISCV_LOCAL_GOTNO:
	      dyn.d_un.d_val = g->local_gotno;
	      break;

	    case DT_RISCV_GOTSYM:
	      if (gg->global_gotsym)
		{
		  dyn.d_un.d_val = gg->global_gotsym->dynindx;
		  break;
		}
	      /* In case if we don't have global got symbols we default
		 to setting DT_RISCV_GOTSYM to the same value as
		 DT_RISCV_SYMTABNO, so we just fall through.  */

	    case DT_RISCV_SYMTABNO:
	      name = ".dynsym";
	      elemsize = RISCV_ELF_SYM_SIZE (output_bfd);
	      s = bfd_get_section_by_name (output_bfd, name);
	      BFD_ASSERT (s != NULL);

	      dyn.d_un.d_val = s->size / elemsize;
	      break;

	    case DT_PLTREL:
	      dyn.d_un.d_val = DT_REL;
	      break;

	    case DT_PLTRELSZ:
	      dyn.d_un.d_val = htab->srelplt->size;
	      break;

	    case DT_JMPREL:
	      dyn.d_un.d_ptr = sec_addr(htab->srelplt);
	      break;

	    case DT_TEXTREL:
	      /* If we didn't need any text relocations after all, delete
		 the dynamic tag.  */
	      if (!(info->flags & DF_TEXTREL))
		{
		  dyn_to_skip = RISCV_ELF_DYN_SIZE (dynobj);
		  swap_out_p = FALSE;
		}
	      break;

	    case DT_FLAGS:
	      /* If we didn't need any text relocations after all, clear
		 DF_TEXTREL from DT_FLAGS.  */
	      if (!(info->flags & DF_TEXTREL))
		dyn.d_un.d_val &= ~DF_TEXTREL;
	      else
		swap_out_p = FALSE;
	      break;

	    default:
	      swap_out_p = FALSE;
	      break;
	    }

	  if (swap_out_p || dyn_skipped)
	    (*get_elf_backend_data (dynobj)->s->swap_dyn_out)
	      (dynobj, &dyn, b - dyn_skipped);

	  if (dyn_to_skip)
	    {
	      dyn_skipped += dyn_to_skip;
	      dyn_to_skip = 0;
	    }
	}

      /* Wipe out any trailing entries if we shifted down a dynamic tag.  */
      if (dyn_skipped > 0)
	memset (b - dyn_skipped, 0, dyn_skipped);
    }

  if (sgot != NULL && sgot->size > 0
      && !bfd_is_abs_section (sgot->output_section))
    {
      /* The first two entries of the GOT will be filled at runtime. */
      RISCV_ELF_PUT_WORD (output_bfd, (bfd_vma) 0, sgot->contents);
      RISCV_ELF_PUT_WORD (output_bfd, (bfd_vma) 0,
			  sgot->contents + RISCV_ELF_GOT_SIZE (output_bfd));

      elf_section_data (sgot->output_section)->this_hdr.sh_entsize
	 = RISCV_ELF_GOT_SIZE (output_bfd);
    }

  /* The generation of dynamic relocations for the non-primary gots
     adds more dynamic relocations.  We cannot count them until
     here.  */

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      bfd_byte *b;
      bfd_boolean swap_out_p;

      BFD_ASSERT (sdyn != NULL);

      for (b = sdyn->contents;
	   b < sdyn->contents + sdyn->size;
	   b += RISCV_ELF_DYN_SIZE (dynobj))
	{
	  Elf_Internal_Dyn dyn;
	  asection *s;

	  /* Read in the current dynamic entry.  */
	  (*get_elf_backend_data (dynobj)->s->swap_dyn_in) (dynobj, b, &dyn);

	  /* Assume that we're going to modify it and write it out.  */
	  swap_out_p = TRUE;

	  switch (dyn.d_tag)
	    {
	    case DT_RELSZ:
	      /* Reduce DT_RELSZ to account for any relocations we
		 decided not to make.  This is for the n64 irix rld,
		 which doesn't seem to apply any relocations if there
		 are trailing null entries.  */
	      s = riscv_elf_rel_dyn_section (info, FALSE);
	      dyn.d_un.d_val = (s->reloc_count
				* (ABI_64_P (output_bfd)
				   ? sizeof (Elf64_External_Rel)
				   : sizeof (Elf32_External_Rel)));
	      /* Adjust the section size too.  Tools like the prelinker
		 can reasonably expect the values to the same.  */
	      elf_section_data (s->output_section)->this_hdr.sh_size
		= dyn.d_un.d_val;
	      break;

	    default:
	      swap_out_p = FALSE;
	      break;
	    }

	  if (swap_out_p)
	    (*get_elf_backend_data (dynobj)->s->swap_dyn_out)
	      (dynobj, &dyn, b);
	}
    }

  {
    asection *s;

    /* The psABI says that the dynamic relocations must be sorted in
       increasing order of r_symndx. */
    s = riscv_elf_rel_dyn_section (info, FALSE);
    if (s != NULL && s->size > (bfd_vma)2 * RISCV_ELF_REL_SIZE (output_bfd))
      {
        reldyn_sorting_bfd = output_bfd;

        if (ABI_64_P (output_bfd))
          qsort ((Elf64_External_Rel *) s->contents + 1,
	         s->reloc_count - 1, sizeof (Elf64_External_Rel),
	         sort_dynamic_relocs_64);
        else
          qsort ((Elf32_External_Rel *) s->contents + 1,
	         s->reloc_count - 1, sizeof (Elf32_External_Rel),
	         sort_dynamic_relocs);
      }
  }

  if (htab->splt && htab->splt->size > 0)
    riscv_finish_exec_plt (output_bfd, info);
  return TRUE;
}

int
_bfd_riscv_elf_additional_program_headers (bfd *abfd,
					  struct bfd_link_info *info ATTRIBUTE_UNUSED)
{
  /* Allocate a PT_NULL header in dynamic objects.  See
     _bfd_riscv_elf_modify_segment_map for details.  */
  if (bfd_get_section_by_name (abfd, ".dynamic"))
    return 1;
  return 0;
}

/* Modify the segment map for an IRIX5 executable.  */

bfd_boolean
_bfd_riscv_elf_modify_segment_map (bfd *abfd,
				  struct bfd_link_info *info)
{
  struct elf_segment_map *m, **pm;

  /* Allocate a spare program header in dynamic objects so that tools
     like the prelinker can add an extra PT_LOAD entry.

     If the prelinker needs to make room for a new PT_LOAD entry, its
     standard procedure is to move the first (read-only) sections into
     the new (writable) segment.  However, the RISC-V ABI requires
     .dynamic to be in a read-only segment, and the section will often
     start within sizeof (ElfNN_Phdr) bytes of the last program header.

     Although the prelinker could in principle move .dynamic to a
     writable segment, it seems better to allocate a spare program
     header instead, and avoid the need to move any sections.
     There is a long tradition of allocating spare dynamic tags,
     so allocating a spare program header seems like a natural
     extension.

     If INFO is NULL, we may be copying an already prelinked binary
     with objcopy or strip, so do not add this header.  */
  if (info != NULL && bfd_get_section_by_name (abfd, ".dynamic"))
    {
      for (pm = &elf_seg_map (abfd); *pm != NULL; pm = &(*pm)->next)
	if ((*pm)->p_type == PT_NULL)
	  break;
      if (*pm == NULL)
	{
	  m = bfd_zalloc (abfd, sizeof (*m));
	  if (m == NULL)
	    return FALSE;

	  m->p_type = PT_NULL;
	  *pm = m;
	}
    }

  return TRUE;
}

/* Copy data from a RISC-V ELF indirect symbol to its direct symbol,
   hiding the old indirect symbol.  Process additional relocation
   information.  Also called for weakdefs, in which case we just let
   _bfd_elf_link_hash_copy_indirect copy the flags for us.  */

void
_bfd_riscv_elf_copy_indirect_symbol (struct bfd_link_info *info,
				    struct elf_link_hash_entry *dir,
				    struct elf_link_hash_entry *ind)
{
  struct riscv_elf_link_hash_entry *dirriscv, *indriscv;

  _bfd_elf_link_hash_copy_indirect (info, dir, ind);

  dirriscv = (struct riscv_elf_link_hash_entry *) dir;
  indriscv = (struct riscv_elf_link_hash_entry *) ind;
  /* Any absolute non-dynamic relocations against an indirect or weak
     definition will be against the target symbol.  */
  if (indriscv->has_static_relocs)
    dirriscv->has_static_relocs = TRUE;

  if (ind->root.type != bfd_link_hash_indirect)
    return;

  dirriscv->possibly_dynamic_relocs += indriscv->possibly_dynamic_relocs;
  if (indriscv->readonly_reloc)
    dirriscv->readonly_reloc = TRUE;
  if (indriscv->global_got_area < dirriscv->global_got_area)
    dirriscv->global_got_area = indriscv->global_got_area;
  if (indriscv->global_got_area < GGA_NONE)
    indriscv->global_got_area = GGA_NONE;

  if (dirriscv->tls_type == 0)
    dirriscv->tls_type = indriscv->tls_type;
}

#define PDR_SIZE 32

bfd_boolean
_bfd_riscv_elf_discard_info (bfd *abfd, struct elf_reloc_cookie *cookie,
			    struct bfd_link_info *info)
{
  asection *o;
  bfd_boolean ret = FALSE;
  unsigned char *tdata;
  size_t i, skip;

  o = bfd_get_section_by_name (abfd, ".pdr");
  if (! o)
    return FALSE;
  if (o->size == 0)
    return FALSE;
  if (o->size % PDR_SIZE != 0)
    return FALSE;
  if (o->output_section != NULL
      && bfd_is_abs_section (o->output_section))
    return FALSE;

  tdata = bfd_zmalloc (o->size / PDR_SIZE);
  if (! tdata)
    return FALSE;

  cookie->rels = _bfd_elf_link_read_relocs (abfd, o, NULL, NULL,
					    info->keep_memory);
  if (!cookie->rels)
    {
      free (tdata);
      return FALSE;
    }

  cookie->rel = cookie->rels;
  cookie->relend = cookie->rels + o->reloc_count;

  for (i = 0, skip = 0; i < o->size / PDR_SIZE; i ++)
    {
      if (bfd_elf_reloc_symbol_deleted_p (i * PDR_SIZE, cookie))
	{
	  tdata[i] = 1;
	  skip ++;
	}
    }

  if (skip != 0)
    {
      riscv_elf_section_data (o)->u.tdata = tdata;
      o->size -= skip * PDR_SIZE;
      ret = TRUE;
    }
  else
    free (tdata);

  if (! info->keep_memory)
    free (cookie->rels);

  return ret;
}

bfd_boolean
_bfd_riscv_elf_ignore_discarded_relocs (asection *sec)
{
  if (strcmp (sec->name, ".pdr") == 0)
    return TRUE;
  return FALSE;
}

bfd_boolean
_bfd_riscv_elf_write_section (bfd *output_bfd,
			     struct bfd_link_info *link_info ATTRIBUTE_UNUSED,
                             asection *sec, bfd_byte *contents)
{
  bfd_byte *to, *from, *end;
  int i;

  if (strcmp (sec->name, ".pdr") != 0)
    return FALSE;

  if (riscv_elf_section_data (sec)->u.tdata == NULL)
    return FALSE;

  to = contents;
  end = contents + sec->size;
  for (from = contents, i = 0;
       from < end;
       from += PDR_SIZE, i++)
    {
      if ((riscv_elf_section_data (sec)->u.tdata)[i] == 1)
	continue;
      if (to != from)
	memcpy (to, from, PDR_SIZE);
      to += PDR_SIZE;
    }
  bfd_set_section_contents (output_bfd, sec->output_section, contents,
			    sec->output_offset, sec->size);
  return TRUE;
}

/* Create a RISC-V ELF linker hash table.  */

struct bfd_link_hash_table *
_bfd_riscv_elf_link_hash_table_create (bfd *abfd)
{
  struct riscv_elf_link_hash_table *ret;
  bfd_size_type amt = sizeof (struct riscv_elf_link_hash_table);

  ret = bfd_malloc (amt);
  if (ret == NULL)
    return NULL;

  if (!_bfd_elf_link_hash_table_init (&ret->root, abfd,
				      riscv_elf_link_hash_newfunc,
				      sizeof (struct riscv_elf_link_hash_entry),
				      RISCV_ELF_DATA))
    {
      free (ret);
      return NULL;
    }

  ret->srelbss = NULL;
  ret->sdynbss = NULL;
  ret->srelplt = NULL;
  ret->srelplt2 = NULL;
  ret->sgotplt = NULL;
  ret->splt = NULL;
  ret->sgot = NULL;
  ret->got_info = NULL;
  ret->nplt = 0;

  return &ret->root.root;
}

/* We need to use a special link routine to handle the .reginfo and
   the .mdebug sections.  We need to merge all instances of these
   sections together, not write them all out sequentially.  */

bfd_boolean
_bfd_riscv_elf_final_link (bfd *abfd, struct bfd_link_info *info)
{
  struct riscv_elf_link_hash_table *htab;

  bfd_riscv_init_gp_value (abfd, info);
  /* Sort the dynamic symbols so that those with GOT entries come after
     those without.  */
  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  if (!riscv_elf_sort_hash_table (abfd, info))
    return FALSE;

  /* Invoke the regular ELF backend linker to do all the work.  */
  return bfd_elf_final_link (abfd, info);
}

/* Return true if bfd machine EXTENSION is an extension of machine BASE.  */

static bfd_boolean
riscv_mach_extends_p (unsigned long base, unsigned long extension)
{
  return extension == base;
}

/* Merge backend specific data from an object file to the output
   object file when linking.  */

bfd_boolean
_bfd_riscv_elf_merge_private_bfd_data (bfd *ibfd, bfd *obfd)
{
  flagword old_flags;
  flagword new_flags;
  bfd_boolean ok;
  bfd_boolean null_input_bfd = TRUE;
  asection *sec;

  /* Check if we have the same endianess */
  if (! _bfd_generic_verify_endian_match (ibfd, obfd))
    {
      (*_bfd_error_handler)
	(_("%B: endianness incompatible with that of the selected emulation"),
	 ibfd);
      return FALSE;
    }

  if (!is_riscv_elf (ibfd) || !is_riscv_elf (obfd))
    return TRUE;

  if (strcmp (bfd_get_target (ibfd), bfd_get_target (obfd)) != 0)
    {
      (*_bfd_error_handler)
	(_("%B: ABI is incompatible with that of the selected emulation"),
	 ibfd);
      return FALSE;
    }

  if (!_bfd_elf_merge_object_attributes (ibfd, obfd))
    return FALSE;

  new_flags = elf_elfheader (ibfd)->e_flags;
  old_flags = elf_elfheader (obfd)->e_flags;

  if (! elf_flags_init (obfd))
    {
      elf_flags_init (obfd) = TRUE;
      elf_elfheader (obfd)->e_flags = new_flags;
      elf_elfheader (obfd)->e_ident[EI_CLASS]
	= elf_elfheader (ibfd)->e_ident[EI_CLASS];

      if (bfd_get_arch (obfd) == bfd_get_arch (ibfd)
	  && (bfd_get_arch_info (obfd)->the_default
	      || riscv_mach_extends_p (bfd_get_mach (obfd), 
				       bfd_get_mach (ibfd))))
	{
	  if (! bfd_set_arch_mach (obfd, bfd_get_arch (ibfd),
				   bfd_get_mach (ibfd)))
	    return FALSE;
	}

      return TRUE;
    }

  /* Check flag compatibility.  */

  if (new_flags == old_flags)
    return TRUE;

  /* Check to see if the input BFD actually contains any sections.
     If not, its flags may not have been initialised either, but it cannot
     actually cause any incompatibility.  */
  for (sec = ibfd->sections; sec != NULL; sec = sec->next)
    {
      /* Ignore synthetic sections and empty .text, .data and .bss sections
	 which are automatically generated by gas.  Also ignore fake
	 (s)common sections, since merely defining a common symbol does
	 not affect compatibility.  */
      if ((sec->flags & SEC_IS_COMMON) == 0
	  && (sec->size != 0
	      || (strcmp (sec->name, ".text")
		  && strcmp (sec->name, ".data")
		  && strcmp (sec->name, ".bss"))))
	{
	  null_input_bfd = FALSE;
	  break;
	}
    }
  if (null_input_bfd)
    return TRUE;

  ok = TRUE;

  /* Don't link RV32 and RV64. */
  if (elf_elfheader (ibfd)->e_ident[EI_CLASS]
      != elf_elfheader (obfd)->e_ident[EI_CLASS])
    {
      (*_bfd_error_handler)
	(_("%B: ABI mismatch: linking %s module with previous %s modules"),
	  ibfd,
	  elf_riscv_abi_name (ibfd),
	  elf_riscv_abi_name (obfd));
      ok = FALSE;
    }

  /* Warn about any other mismatches */
  if (new_flags != old_flags)
    {
      if (!EF_IS_RISCV_EXT_Xcustom(new_flags) &&
          !EF_IS_RISCV_EXT_Xcustom(old_flags)) {
        (*_bfd_error_handler)
          (_("%B: uses different e_flags (0x%lx) fields than previous modules (0x%lx)"),
           ibfd, (unsigned long) new_flags,
           (unsigned long) old_flags);
        ok = FALSE;
      } else if (EF_IS_RISCV_EXT_Xcustom(new_flags)) {
        EF_SET_RISCV_EXT(elf_elfheader (obfd)->e_flags, EF_GET_RISCV_EXT(old_flags));
      }
    }

  if (! ok)
    {
      bfd_set_error (bfd_error_bad_value);
      return FALSE;
    }

  return TRUE;
}

char *
_bfd_riscv_elf_get_target_dtag (bfd_vma dtag)
{
  switch (dtag)
    {
    case DT_RISCV_LOCAL_GOTNO:
      return "RISCV_LOCAL_GOTNO";
    case DT_RISCV_SYMTABNO:
      return "RISCV_SYMTABNO";
    case DT_RISCV_GOTSYM:
      return "RISCV_GOTSYM";
    case DT_RISCV_PLTGOT:
      return "RISCV_PLTGOT";
    default:
      return "";
    }
}

bfd_boolean
_bfd_riscv_elf_print_private_bfd_data (bfd *abfd, void *ptr)
{
  FILE *file = ptr;

  BFD_ASSERT (abfd != NULL && ptr != NULL);

  /* Print normal ELF private data.  */
  _bfd_elf_print_private_bfd_data (abfd, ptr);

  /* xgettext:c-format */
  fprintf (file, _("private flags = %lx:"), elf_elfheader (abfd)->e_flags);

  if (ABI_32_P (abfd))
    fprintf (file, _(" [rv32]"));
  else if (ABI_64_P (abfd))
    fprintf (file, _(" [rv64]"));
  else
    fprintf (file, _(" [no abi set]"));

  fputc ('\n', file);

  return TRUE;
}

const struct bfd_elf_special_section _bfd_riscv_elf_special_sections[] =
{
  { NULL,                     0,  0, 0,              0 }
};

/* Merge non visibility st_other attributes.  Ensure that the
   STO_OPTIONAL flag is copied into h->other, even if this is not a
   definiton of the symbol.  */
void
_bfd_riscv_elf_merge_symbol_attribute (struct elf_link_hash_entry *h,
				      const Elf_Internal_Sym *isym,
				      bfd_boolean definition,
				      bfd_boolean dynamic ATTRIBUTE_UNUSED)
{
  if ((isym->st_other & ~ELF_ST_VISIBILITY (-1)) != 0)
    {
      unsigned char other;

      other = (definition ? isym->st_other : h->other);
      other &= ~ELF_ST_VISIBILITY (-1);
      h->other = other | ELF_ST_VISIBILITY (h->other);
    }
}

bfd_boolean
_bfd_riscv_elf_common_definition (Elf_Internal_Sym *sym)
{
  return (sym->st_shndx == SHN_COMMON);
}

/* Delete some bytes from a section while relaxing.  */

static bfd_boolean
riscv_relax_delete_bytes (bfd *abfd, asection *sec, bfd_vma addr, size_t count)
{
  Elf_Internal_Shdr * symtab_hdr;
  unsigned int        sec_shndx;
  bfd_byte *          contents;
  Elf_Internal_Rela * irel;
  Elf_Internal_Rela * irelend;
  Elf_Internal_Sym *  isym;
  Elf_Internal_Sym *  isymend;
  bfd_vma             toaddr;
  unsigned int        symcount;
  struct elf_link_hash_entry ** sym_hashes;
  struct elf_link_hash_entry ** end_hashes;

  /* TODO: handle alignment */
  Elf_Internal_Rela *alignment_rel = NULL;

  sec_shndx = _bfd_elf_section_from_bfd_section (abfd, sec);

  contents = elf_section_data (sec)->this_hdr.contents;

  /* The deletion must stop at the next alignment boundary, if
     ALIGNMENT_REL is non-NULL.  */
  toaddr = sec->size;
  if (alignment_rel)
    toaddr = alignment_rel->r_offset;

  irel = elf_section_data (sec)->relocs;
  irelend = irel + sec->reloc_count;

  /* Actually delete the bytes.  */
  memmove (contents + addr, contents + addr + count,
	   (size_t) (toaddr - addr - count));

  if (alignment_rel)
    {
      size_t i;
      BFD_ASSERT (count % 4 == 0);
      for (i = 0; i < count; i += 4)
	bfd_put_32 (abfd, RISCV_NOP, contents + toaddr - count + i);
      /* TODO: RVC NOP if count % 4 == 2 */
    }
  else
    sec->size -= count;

  /* Adjust all the relocs.  */
  for (irel = elf_section_data (sec)->relocs; irel < irelend; irel++)
    {
      if (irel->r_offset <= addr)
	{
	  if (irel->r_offset + irel->r_addend > addr)
	    irel->r_addend -= ELF_R_SYM (abfd, irel->r_info) ? 0 : count;
	}
      else
	{
	  if (irel->r_offset + irel->r_addend <= addr)
	    irel->r_addend += ELF_R_SYM (abfd, irel->r_info) ? 0 : count;
	  if (irel->r_offset < toaddr)
	    irel->r_offset -= count;
	}
    }

  /* Adjust the local symbols defined in this section.  */
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  isym = (Elf_Internal_Sym *) symtab_hdr->contents;
  isymend = isym + symtab_hdr->sh_info;

  for (; isym < isymend; isym++)
    {
      /* If the symbol is in the range of memory we just moved, we
	 have to adjust its value.  */
      if (isym->st_shndx == sec_shndx
	  && isym->st_value > addr
	  && isym->st_value <= toaddr)
	isym->st_value -= count;

      /* If the symbol *spans* the bytes we just deleted (i.e. its
	 *end* is in the moved bytes but its *start* isn't), then we
	 must adjust its size.  */
      if (isym->st_shndx == sec_shndx
	  && isym->st_value < addr
	  && isym->st_value + isym->st_size > addr
	  && isym->st_value + isym->st_size <= toaddr)
	isym->st_size -= count;
    }

  /* Now adjust the global symbols defined in this section.  */
  symcount = symtab_hdr->sh_size / sizeof(Elf64_External_Sym);
  if (!ABI_64_P (abfd))
    symcount = symtab_hdr->sh_size / sizeof(Elf32_External_Sym);
  symcount -= symtab_hdr->sh_info;

  sym_hashes = elf_sym_hashes (abfd);
  end_hashes = sym_hashes + symcount;

  for (; sym_hashes < end_hashes; sym_hashes++)
    {
      struct elf_link_hash_entry *sym_hash = *sym_hashes;

      if ((sym_hash->root.type == bfd_link_hash_defined
	   || sym_hash->root.type == bfd_link_hash_defweak)
	  && sym_hash->root.u.def.section == sec)
	{
	  /* As above, adjust the value if needed.  */
	  if (sym_hash->root.u.def.value > addr
	      && sym_hash->root.u.def.value < toaddr)
	    sym_hash->root.u.def.value -= count;

	  /* As above, adjust the size if needed.  */
	  if (sym_hash->root.u.def.value < addr
	      && sym_hash->root.u.def.value + sym_hash->size > addr
	      && sym_hash->root.u.def.value + sym_hash->size < toaddr)
	    sym_hash->size -= count;
	}
    }

  return TRUE;
}

static bfd_boolean
_bfd_riscv_relax_call (bfd *abfd, asection *sec,
		       struct bfd_link_info *link_info, bfd_byte *contents,
		       Elf_Internal_Shdr *symtab_hdr,
		       Elf_Internal_Sym *isymbuf,
		       Elf_Internal_Rela *internal_relocs,
		       Elf_Internal_Rela *irel, bfd_vma symval,
		       bfd_boolean *again)
{
  /* See if this function call can be shortened.  */
  bfd_signed_vma foff = symval - (sec_addr(sec) + irel->r_offset);
  bfd_boolean near_zero = !link_info->shared && symval < RISCV_IMM_REACH/2;
  if (!VALID_UJTYPE_IMM (foff) && !near_zero)
    return TRUE;

  /* Shorten the function call.  */
  elf_section_data (sec)->relocs = internal_relocs;
  elf_section_data (sec)->this_hdr.contents = contents;
  symtab_hdr->contents = (unsigned char *) isymbuf;

  BFD_ASSERT (irel->r_offset + 8 <= sec->size);

  bfd_vma auipc = bfd_get_32 (abfd, contents + irel->r_offset);
  BFD_ASSERT ((auipc & MASK_AUIPC) == MATCH_AUIPC);

  bfd_vma jalr = bfd_get_32 (abfd, contents + irel->r_offset + 4);
  BFD_ASSERT ((jalr & MASK_JALR) == MATCH_JALR);
  /* Replace the R_RISCV_CALL reloc with R_RISCV_JAL. */
  irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_JAL);
  /* Overwrite AUIPC with JAL rd, addr. */
  auipc = (jalr & (OP_MASK_RD << OP_SH_RD)) | MATCH_JAL;
  bfd_put_32 (abfd, auipc, contents + irel->r_offset);

  /* Delete unnecessary JALR. */
  if (! riscv_relax_delete_bytes (abfd, sec, irel->r_offset + 4, 4))
    return FALSE;

  *again = TRUE;
  return TRUE;
}

static bfd_boolean
_bfd_riscv_relax_lui (bfd *abfd, asection *sec,
		      struct bfd_link_info *link_info, bfd_byte *contents,
		      Elf_Internal_Shdr *symtab_hdr,
		      Elf_Internal_Sym *isymbuf,
		      Elf_Internal_Rela *internal_relocs,
		      Elf_Internal_Rela *irel, bfd_vma symval,
		      bfd_boolean *again)
{
  bfd_vma gp = bfd_riscv_init_gp_value (abfd, link_info);
  if (!gp || symval == gp)
    return TRUE;

  /* See if this symbol is in range of gp. */
  if (RISCV_CONST_HIGH_PART (symval - gp) != 0)
    return TRUE;

  /* We can delete the unnecessary AUIPC. The corresponding LO12 reloc
     will be converted to GPREL during relocation. */
  elf_section_data (sec)->relocs = internal_relocs;
  elf_section_data (sec)->this_hdr.contents = contents;
  symtab_hdr->contents = (unsigned char *) isymbuf;

  BFD_ASSERT (irel->r_offset + 4 <= sec->size);
  irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_NONE);
  if (! riscv_relax_delete_bytes (abfd, sec, irel->r_offset, 4))
    return FALSE;

  *again = TRUE;
  return TRUE;
}

static bfd_boolean
_bfd_riscv_relax_tls_le (bfd *abfd, asection *sec,
			 struct bfd_link_info *link_info, bfd_byte *contents,
			 Elf_Internal_Shdr *symtab_hdr,
			 Elf_Internal_Sym *isymbuf,
			 Elf_Internal_Rela *internal_relocs,
			 Elf_Internal_Rela *irel, bfd_vma symval,
			 bfd_boolean *again)
{
  /* See if this symbol is in range of tp. */
  if (RISCV_CONST_HIGH_PART (symval - tprel_base (link_info)) != 0)
    return TRUE;

  /* We can delete the unnecessary LUI and TP add.  The LO12 reloc will be
     made directly TP-relative. */
  elf_section_data (sec)->relocs = internal_relocs;
  elf_section_data (sec)->this_hdr.contents = contents;
  symtab_hdr->contents = (unsigned char *) isymbuf;

  BFD_ASSERT (irel->r_offset + 4 <= sec->size);
  irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_NONE);
  if (! riscv_relax_delete_bytes (abfd, sec, irel->r_offset, 4))
    return FALSE;

  *again = TRUE;
  return TRUE;
}

/* Relax TLS IE to TLS LE. */
static bfd_boolean
_bfd_riscv_relax_tls_ie (bfd *abfd, asection *sec,
			 bfd_byte *contents,
			 Elf_Internal_Shdr *symtab_hdr,
			 Elf_Internal_Sym *isymbuf,
			 Elf_Internal_Rela *internal_relocs,
			 Elf_Internal_Rela *irel,
			 bfd_boolean *again)
{
  bfd_vma insn;

  elf_section_data (sec)->relocs = internal_relocs;
  elf_section_data (sec)->this_hdr.contents = contents;
  symtab_hdr->contents = (unsigned char *) isymbuf;

  switch (ELF_R_TYPE (abfd, irel->r_info))
    {
    case R_RISCV_TLS_IE_HI20:
      /* Replace with R_RISCV_TPREL_HI20. */
      irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_TPREL_HI20);
      /* Overwrite AUIPC with LUI. */
      BFD_ASSERT (irel->r_offset + 4 <= sec->size);
      insn = bfd_get_32 (abfd, contents + irel->r_offset);
      insn = (insn & ~MASK_LUI) | MATCH_LUI;
      bfd_put_32 (abfd, insn, contents + irel->r_offset);
      break;

    case R_RISCV_TLS_IE_LO12:
      /* Just delete the reloc. */
      irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_NONE);
      if (! riscv_relax_delete_bytes (abfd, sec, irel->r_offset, 4))
        return FALSE;
      break;

    case R_RISCV_TLS_IE_ADD:
      /* Replace with R_RISCV_TPREL_ADD. */
      irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_TPREL_ADD);
      break;

    case R_RISCV_TLS_IE_LO12_I:
      /* Replace with R_RISCV_TPREL_LO12_I. */
      irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_TPREL_LO12_I);
      break;

    case R_RISCV_TLS_IE_LO12_S:
      /* Replace with R_RISCV_TPREL_LO12_S. */
      irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_TPREL_LO12_S);
      break;

    default:
      abort();
    }

  *again = TRUE;
  return TRUE;
}

/* Relax AUIPC/JALR into JAL. */

bfd_boolean
_bfd_riscv_relax_section (bfd *abfd, asection *sec,
			  struct bfd_link_info *link_info, bfd_boolean *again)
{
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Rela *internal_relocs;
  Elf_Internal_Rela *irel, *irelend;
  bfd_byte *contents = NULL;
  Elf_Internal_Sym *isymbuf = NULL;
  struct riscv_elf_link_hash_table *htab;
  
  htab = riscv_elf_hash_table (link_info);
  htab->relax = TRUE;

  *again = FALSE;

  if (link_info->relocatable
      || (sec->flags & SEC_RELOC) == 0
      || sec->reloc_count == 0)
    return TRUE;

  symtab_hdr = &elf_symtab_hdr (abfd);

  internal_relocs = (_bfd_elf_link_read_relocs
		     (abfd, sec, NULL, (Elf_Internal_Rela *) NULL,
		      link_info->keep_memory));
  if (internal_relocs == NULL)
    goto error_return;

  irelend = internal_relocs + sec->reloc_count;
  for (irel = internal_relocs; irel < irelend; irel++)
    {
      bfd_vma symval;
      int type = ELF_R_TYPE (abfd, irel->r_info);
      bfd_boolean call = type == R_RISCV_CALL || type == R_RISCV_CALL_PLT;
      bfd_boolean lui = type == R_RISCV_HI20;
      bfd_boolean tls_le = type == R_RISCV_TPREL_HI20 || type == R_RISCV_TPREL_ADD;
      bfd_boolean tls_ie = type == R_RISCV_TLS_IE_HI20 || type == R_RISCV_TLS_IE_LO12 || type == R_RISCV_TLS_IE_ADD || type == R_RISCV_TLS_IE_LO12_I || type == R_RISCV_TLS_IE_LO12_S;

      if (!(call || lui || tls_le || tls_ie))
	continue;

      /* Get the section contents.  */
      if (contents == NULL)
	{
	  if (elf_section_data (sec)->this_hdr.contents != NULL)
	    contents = elf_section_data (sec)->this_hdr.contents;
	  else
	    {
	      if (!bfd_malloc_and_get_section (abfd, sec, &contents))
		goto error_return;
	    }
	}

      /* Read this BFD's symbols if we haven't done so already.  */
      if (isymbuf == NULL && symtab_hdr->sh_info != 0)
	{
	  isymbuf = (Elf_Internal_Sym *) symtab_hdr->contents;
	  if (isymbuf == NULL)
	    isymbuf = bfd_elf_get_elf_syms (abfd, symtab_hdr,
					    symtab_hdr->sh_info, 0,
					    NULL, NULL, NULL);
	  if (isymbuf == NULL)
	    goto error_return;
	}

      /* Get the value of the symbol referred to by the reloc.  */
      if (ELF_R_SYM (abfd, irel->r_info) < symtab_hdr->sh_info)
	{
	  /* A local symbol.  */
	  Elf_Internal_Sym *isym = isymbuf + ELF_R_SYM (abfd, irel->r_info);

	  if (isym->st_shndx == SHN_UNDEF)
	    symval = sec_addr(sec) + irel->r_offset;
	  else
	    {
	      asection *isec;
	      BFD_ASSERT (isym->st_shndx < elf_numsections (abfd));
	      isec = elf_elfsections (abfd)[isym->st_shndx]->bfd_section;
	      symval = sec_addr(isec) + isym->st_value;
	    }
	}
      else
	{
	  unsigned long indx;
	  struct elf_link_hash_entry *h;

	  indx = ELF_R_SYM (abfd, irel->r_info) - symtab_hdr->sh_info;
	  h = elf_sym_hashes (abfd)[indx];

	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;

	  if (h->plt.offset != MINUS_ONE)
	    symval = sec_addr(htab->splt) + h->plt.offset;
	  else if (h->root.u.def.section->output_section == NULL
		   || (h->root.type != bfd_link_hash_defined
		       && h->root.type != bfd_link_hash_defweak))
	    continue;
	  else
	    symval = sec_addr(h->root.u.def.section) + h->root.u.def.value;
	}

      symval += irel->r_addend;

      if (call && !_bfd_riscv_relax_call (abfd, sec, link_info, contents,
					  symtab_hdr, isymbuf, internal_relocs,
					  irel, symval, again))
	goto error_return;
      if (lui && !_bfd_riscv_relax_lui (abfd, sec, link_info, contents,
					symtab_hdr, isymbuf, internal_relocs,
					irel, symval, again))
	goto error_return;
      if (tls_le && !_bfd_riscv_relax_tls_le (abfd, sec, link_info, contents,
					      symtab_hdr, isymbuf, internal_relocs,
					      irel, symval, again))
	goto error_return;
      if (tls_ie && !_bfd_riscv_relax_tls_ie (abfd, sec, contents, symtab_hdr,
					      isymbuf, internal_relocs, irel,
					      again))
	goto error_return;
    }

  if (isymbuf != NULL
      && symtab_hdr->contents != (unsigned char *) isymbuf)
    {
      if (! link_info->keep_memory)
	free (isymbuf);
      else
	{
	  /* Cache the symbols for elf_link_input_bfd.  */
	  symtab_hdr->contents = (unsigned char *) isymbuf;
	}
    }

  if (contents != NULL
      && elf_section_data (sec)->this_hdr.contents != contents)
    {
      if (! link_info->keep_memory)
	free (contents);
      else
	{
	  /* Cache the section contents for elf_link_input_bfd.  */
	  elf_section_data (sec)->this_hdr.contents = contents;
	}
    }

  if (internal_relocs != NULL
      && elf_section_data (sec)->relocs != internal_relocs)
    free (internal_relocs);

  return TRUE;

 error_return:
  if (isymbuf != NULL
      && symtab_hdr->contents != (unsigned char *) isymbuf)
    free (isymbuf);
  if (contents != NULL
      && elf_section_data (sec)->this_hdr.contents != contents)
    free (contents);
  if (internal_relocs != NULL
      && elf_section_data (sec)->relocs != internal_relocs)
    free (internal_relocs);

  return FALSE;
}
