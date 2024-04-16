/* DTrace probe support for GDB.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

   Contributed by Oracle, Inc.

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
#include "probe.h"
#include "elf-bfd.h"
#include "gdbtypes.h"
#include "obstack.h"
#include "objfiles.h"
#include "complaints.h"
#include "value.h"
#include "ax.h"
#include "ax-gdb.h"
#include "language.h"
#include "parser-defs.h"
#include "inferior.h"
#include "expop.h"

/* The type of the ELF sections where we will find the DOF programs
   with information about probes.  */

#ifndef SHT_SUNW_dof
# define SHT_SUNW_dof	0x6ffffff4
#endif

/* The following structure represents a single argument for the
   probe.  */

struct dtrace_probe_arg
{
  dtrace_probe_arg (struct type *type_, std::string &&type_str_,
		    expression_up &&expr_)
    : type (type_), type_str (std::move (type_str_)),
      expr (std::move (expr_))
  {}

  /* The type of the probe argument.  */
  struct type *type;

  /* A string describing the type.  */
  std::string type_str;

  /* The argument converted to an internal GDB expression.  */
  expression_up expr;
};

/* The following structure represents an enabler for a probe.  */

struct dtrace_probe_enabler
{
  /* Program counter where the is-enabled probe is installed.  The
     contents (nops, whatever...) stored at this address are
     architecture dependent.  */
  CORE_ADDR address;
};

/* Class that implements the static probe methods for "stap" probes.  */

class dtrace_static_probe_ops : public static_probe_ops
{
public:
  /* See probe.h.  */
  bool is_linespec (const char **linespecp) const override;

  /* See probe.h.  */
  void get_probes (std::vector<std::unique_ptr<probe>> *probesp,
		   struct objfile *objfile) const override;

  /* See probe.h.  */
  const char *type_name () const override;

  /* See probe.h.  */
  bool can_enable () const override
  {
    return true;
  }

  /* See probe.h.  */
  std::vector<struct info_probe_column> gen_info_probes_table_header
    () const override;
};

/* DTrace static_probe_ops.  */

const dtrace_static_probe_ops dtrace_static_probe_ops {};

/* The following structure represents a dtrace probe.  */

class dtrace_probe : public probe
{
public:
  /* Constructor for dtrace_probe.  */
  dtrace_probe (std::string &&name_, std::string &&provider_, CORE_ADDR address_,
		struct gdbarch *arch_,
		std::vector<struct dtrace_probe_arg> &&args_,
		std::vector<struct dtrace_probe_enabler> &&enablers_)
    : probe (std::move (name_), std::move (provider_), address_, arch_),
      m_args (std::move (args_)),
      m_enablers (std::move (enablers_)),
      m_args_expr_built (false)
  {}

  /* See probe.h.  */
  CORE_ADDR get_relocated_address (struct objfile *objfile) override;

  /* See probe.h.  */
  unsigned get_argument_count (struct gdbarch *gdbarch) override;

  /* See probe.h.  */
  bool can_evaluate_arguments () const override;

  /* See probe.h.  */
  struct value *evaluate_argument (unsigned n,
				   frame_info_ptr frame) override;

  /* See probe.h.  */
  void compile_to_ax (struct agent_expr *aexpr,
		      struct axs_value *axs_value,
		      unsigned n) override;

  /* See probe.h.  */
  const static_probe_ops *get_static_ops () const override;

  /* See probe.h.  */
  std::vector<const char *> gen_info_probes_table_values () const override;

  /* See probe.h.  */
  void enable () override;

  /* See probe.h.  */
  void disable () override;

  /* Return the Nth argument of the probe.  */
  struct dtrace_probe_arg *get_arg_by_number (unsigned n,
					      struct gdbarch *gdbarch);

  /* Build the GDB internal expression that, once evaluated, will
     calculate the values of the arguments of the probe.  */
  void build_arg_exprs (struct gdbarch *gdbarch);

  /* Determine whether the probe is "enabled" or "disabled".  A
     disabled probe is a probe in which one or more enablers are
     disabled.  */
  bool is_enabled () const;

private:
  /* A probe can have zero or more arguments.  */
  std::vector<struct dtrace_probe_arg> m_args;

  /* A probe can have zero or more "enablers" associated with it.  */
  std::vector<struct dtrace_probe_enabler> m_enablers;

  /* Whether the expressions for the arguments have been built.  */
  bool m_args_expr_built;
};

/* DOF programs can contain an arbitrary number of sections of 26
   different types.  In order to support DTrace USDT probes we only
   need to handle a subset of these section types, fortunately.  These
   section types are defined in the following enumeration.

   See linux/dtrace/dof_defines.h for a complete list of section types
   along with their values.  */

enum dtrace_dof_sect_type
{
  /* Null section.  */
  DTRACE_DOF_SECT_TYPE_NONE = 0,
  /* A dof_ecbdesc_t. */
  DTRACE_DOF_SECT_TYPE_ECBDESC = 3,
  /* A string table.  */
  DTRACE_DOF_SECT_TYPE_STRTAB = 8,
  /* A dof_provider_t  */
  DTRACE_DOF_SECT_TYPE_PROVIDER = 15,
  /* Array of dof_probe_t  */
  DTRACE_DOF_SECT_TYPE_PROBES = 16,
  /* An array of probe arg mappings.  */
  DTRACE_DOF_SECT_TYPE_PRARGS = 17,
  /* An array of probe arg offsets.  */
  DTRACE_DOF_SECT_TYPE_PROFFS = 18,
  /* An array of probe is-enabled offsets.  */
  DTRACE_DOF_SECT_TYPE_PRENOFFS = 26
};

/* The following collection of data structures map the structure of
   DOF entities.  Again, we only cover the subset of DOF used to
   implement USDT probes.

   See linux/dtrace/dof.h header for a complete list of data
   structures.  */

/* Offsets to index the dofh_ident[] array defined below.  */

enum dtrace_dof_ident
{
  /* First byte of the magic number.  */
  DTRACE_DOF_ID_MAG0 = 0,
  /* Second byte of the magic number.  */
  DTRACE_DOF_ID_MAG1 = 1,
  /* Third byte of the magic number.  */
  DTRACE_DOF_ID_MAG2 = 2,
  /* Fourth byte of the magic number.  */
  DTRACE_DOF_ID_MAG3 = 3,
  /* An enum_dof_encoding value.  */
  DTRACE_DOF_ID_ENCODING = 5
};

/* Possible values for dofh_ident[DOF_ID_ENCODING].  */

enum dtrace_dof_encoding
{
  /* The DOF program is little-endian.  */
  DTRACE_DOF_ENCODE_LSB = 1,
  /* The DOF program is big-endian.  */
  DTRACE_DOF_ENCODE_MSB = 2
};

/* A DOF header, which describes the contents of a DOF program: number
   of sections, size, etc.  */

struct dtrace_dof_hdr
{
  /* Identification bytes (see above). */
  uint8_t dofh_ident[16];
  /* File attribute flags (if any). */
  uint32_t dofh_flags;   
  /* Size of file header in bytes. */
  uint32_t dofh_hdrsize; 
  /* Size of section header in bytes. */
  uint32_t dofh_secsize; 
  /* Number of section headers. */
  uint32_t dofh_secnum;  
  /* File offset of section headers. */
  uint64_t dofh_secoff;  
  /* File size of loadable portion. */
  uint64_t dofh_loadsz;  
  /* File size of entire DOF file. */
  uint64_t dofh_filesz;  
  /* Reserved for future use. */
  uint64_t dofh_pad;     
};

/* A DOF section, whose contents depend on its type.  The several
   supported section types are described in the enum
   dtrace_dof_sect_type above.  */

struct dtrace_dof_sect
{
  /* Section type (see the define above). */
  uint32_t dofs_type;
  /* Section data memory alignment. */
  uint32_t dofs_align; 
  /* Section flags (if any). */
  uint32_t dofs_flags; 
  /* Size of section entry (if table). */
  uint32_t dofs_entsize;
  /* DOF + offset points to the section data. */
  uint64_t dofs_offset;
  /* Size of section data in bytes.  */
  uint64_t dofs_size;  
};

/* A DOF provider, which is the provider of a probe.  */

struct dtrace_dof_provider
{
  /* Link to a DTRACE_DOF_SECT_TYPE_STRTAB section. */
  uint32_t dofpv_strtab; 
  /* Link to a DTRACE_DOF_SECT_TYPE_PROBES section. */
  uint32_t dofpv_probes; 
  /* Link to a DTRACE_DOF_SECT_TYPE_PRARGS section. */
  uint32_t dofpv_prargs; 
  /* Link to a DTRACE_DOF_SECT_TYPE_PROFFS section. */
  uint32_t dofpv_proffs; 
  /* Provider name string. */
  uint32_t dofpv_name;   
  /* Provider attributes. */
  uint32_t dofpv_provattr;
  /* Module attributes. */
  uint32_t dofpv_modattr; 
  /* Function attributes. */
  uint32_t dofpv_funcattr;
  /* Name attributes. */
  uint32_t dofpv_nameattr;
  /* Args attributes. */
  uint32_t dofpv_argsattr;
  /* Link to a DTRACE_DOF_SECT_PRENOFFS section. */
  uint32_t dofpv_prenoffs;
};

/* A set of DOF probes and is-enabled probes sharing a base address
   and several attributes.  The particular locations and attributes of
   each probe are maintained in arrays in several other DOF sections.
   See the comment in dtrace_process_dof_probe for details on how
   these attributes are stored.  */

struct dtrace_dof_probe
{
  /* Probe base address or offset. */
  uint64_t dofpr_addr;   
  /* Probe function string. */
  uint32_t dofpr_func;   
  /* Probe name string. */
  uint32_t dofpr_name;   
  /* Native argument type strings. */
  uint32_t dofpr_nargv;  
  /* Translated argument type strings. */
  uint32_t dofpr_xargv;  
  /* Index of first argument mapping. */
  uint32_t dofpr_argidx; 
  /* Index of first offset entry. */
  uint32_t dofpr_offidx; 
  /* Native argument count. */
  uint8_t  dofpr_nargc;  
  /* Translated argument count. */
  uint8_t  dofpr_xargc;  
  /* Number of offset entries for probe. */
  uint16_t dofpr_noffs;  
  /* Index of first is-enabled offset. */
  uint32_t dofpr_enoffidx;
  /* Number of is-enabled offsets. */
  uint16_t dofpr_nenoffs;
  /* Reserved for future use. */
  uint16_t dofpr_pad1;   
  /* Reserved for future use. */
  uint32_t dofpr_pad2;   
};

/* DOF supports two different encodings: MSB (big-endian) and LSB
   (little-endian).  The encoding is itself encoded in the DOF header.
   The following function returns an unsigned value in the host
   endianness.  */

#define DOF_UINT(dof, field)						\
  extract_unsigned_integer ((gdb_byte *) &(field),			\
			    sizeof ((field)),				\
			    (((dof)->dofh_ident[DTRACE_DOF_ID_ENCODING] \
			      == DTRACE_DOF_ENCODE_MSB)			\
			     ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE))

/* The following macro applies a given byte offset to a DOF (a pointer
   to a dtrace_dof_hdr structure) and returns the resulting
   address.  */

#define DTRACE_DOF_PTR(dof, offset) (&((char *) (dof))[(offset)])

/* The following macro returns a pointer to the beginning of a given
   section in a DOF object.  The section is referred to by its index
   in the sections array.  */

#define DTRACE_DOF_SECT(dof, idx)					\
  ((struct dtrace_dof_sect *)						\
   DTRACE_DOF_PTR ((dof),						\
		   DOF_UINT ((dof), (dof)->dofh_secoff)			\
		   + ((idx) * DOF_UINT ((dof), (dof)->dofh_secsize))))

/* Helper function to examine the probe described by the given PROBE
   and PROVIDER data structures and add it to the PROBESP vector.
   STRTAB, OFFTAB, EOFFTAB and ARGTAB are pointers to tables in the
   DOF program containing the attributes for the probe.  */

static void
dtrace_process_dof_probe (struct objfile *objfile,
			  struct gdbarch *gdbarch,
			  std::vector<std::unique_ptr<probe>> *probesp,
			  struct dtrace_dof_hdr *dof,
			  struct dtrace_dof_probe *probe,
			  struct dtrace_dof_provider *provider,
			  char *strtab, char *offtab, char *eofftab,
			  char *argtab, uint64_t strtab_size)
{
  int i, j, num_probes, num_enablers;
  char *p;

  /* Each probe section can define zero or more probes of two
     different types:

     - probe->dofpr_noffs regular probes whose program counters are
       stored in 32bit words starting at probe->dofpr_addr +
       offtab[probe->dofpr_offidx].

     - probe->dofpr_nenoffs is-enabled probes whose program counters
       are stored in 32bit words starting at probe->dofpr_addr +
       eofftab[probe->dofpr_enoffidx].

     However is-enabled probes are not probes per-se, but an
     optimization hack that is implemented in the kernel in a very
     similar way than normal probes.  This is how we support
     is-enabled probes on GDB:

     - Our probes are always DTrace regular probes.

     - Our probes can be associated with zero or more "enablers".  The
       list of enablers is built from the is-enabled probes defined in
       the Probe section.

     - Probes having a non-empty list of enablers can be enabled or
       disabled using the `enable probe' and `disable probe' commands
       respectively.  The `Enabled' column in the output of `info
       probes' will read `yes' if the enablers are activated, `no'
       otherwise.

     - Probes having an empty list of enablers are always enabled.
       The `Enabled' column in the output of `info probes' will
       read `always'.

     It follows that if there are DTrace is-enabled probes defined for
     some provider/name but no DTrace regular probes defined then the
     GDB user wont be able to enable/disable these conditionals.  */

  num_probes = DOF_UINT (dof, probe->dofpr_noffs);
  if (num_probes == 0)
    return;

  /* Build the list of enablers for the probes defined in this Probe
     DOF section.  */
  std::vector<struct dtrace_probe_enabler> enablers;
  num_enablers = DOF_UINT (dof, probe->dofpr_nenoffs);
  for (i = 0; i < num_enablers; i++)
    {
      struct dtrace_probe_enabler enabler;
      uint32_t enabler_offset
	= ((uint32_t *) eofftab)[DOF_UINT (dof, probe->dofpr_enoffidx) + i];

      enabler.address = DOF_UINT (dof, probe->dofpr_addr)
	+ DOF_UINT (dof, enabler_offset);
      enablers.push_back (enabler);
    }

  for (i = 0; i < num_probes; i++)
    {
      uint32_t probe_offset
	= ((uint32_t *) offtab)[DOF_UINT (dof, probe->dofpr_offidx) + i];

      /* Set the provider and the name of the probe.  */
      const char *probe_provider
	= strtab + DOF_UINT (dof, provider->dofpv_name);
      const char *name = strtab + DOF_UINT (dof, probe->dofpr_name);

      /* The probe address.  */
      CORE_ADDR address
	= DOF_UINT (dof, probe->dofpr_addr) + DOF_UINT (dof, probe_offset);

      /* Number of arguments in the probe.  */
      int probe_argc = DOF_UINT (dof, probe->dofpr_nargc);

      /* Store argument type descriptions.  A description of the type
	 of the argument is in the (J+1)th null-terminated string
	 starting at 'strtab' + 'probe->dofpr_nargv'.  */
      std::vector<struct dtrace_probe_arg> args;
      p = strtab + DOF_UINT (dof, probe->dofpr_nargv);
      for (j = 0; j < probe_argc; j++)
	{
	  expression_up expr;

	  /* Set arg.expr to ensure all fields in expr are initialized and
	     the compiler will not warn when arg is used.  */
	  std::string type_str (p);

	  /* Use strtab_size as a sentinel.  */
	  while (*p++ != '\0' && p - strtab < strtab_size)
	    ;

	  /* Try to parse a type expression from the type string.  If
	     this does not work then we set the type to `long
	     int'.  */
	  struct type *type = builtin_type (gdbarch)->builtin_long;

	  try
	    {
	      expr = parse_expression_with_language (type_str.c_str (),
						     language_c);
	    }
	  catch (const gdb_exception_error &ex)
	    {
	    }

	  if (expr != NULL && expr->first_opcode () == OP_TYPE)
	    type = expr->evaluate_type ()->type ();

	  args.emplace_back (type, std::move (type_str), std::move (expr));
	}

      std::vector<struct dtrace_probe_enabler> enablers_copy = enablers;
      dtrace_probe *ret = new dtrace_probe (std::string (name),
					    std::string (probe_provider),
					    address, gdbarch,
					    std::move (args),
					    std::move (enablers_copy));

      /* Successfully created probe.  */
      probesp->emplace_back (ret);
    }
}

/* Helper function to collect the probes described in the DOF program
   whose header is pointed by DOF and add them to the PROBESP vector.
   SECT is the ELF section containing the DOF program and OBJFILE is
   its containing object file.  */

static void
dtrace_process_dof (asection *sect, struct objfile *objfile,
		    std::vector<std::unique_ptr<probe>> *probesp,
		    struct dtrace_dof_hdr *dof)
{
  struct gdbarch *gdbarch = objfile->arch ();
  struct dtrace_dof_sect *section;
  int i;

  /* The first step is to check for the DOF magic number.  If no valid
     DOF data is found in the section then a complaint is issued to
     the user and the section skipped.  */
  if (dof->dofh_ident[DTRACE_DOF_ID_MAG0] != 0x7F
      || dof->dofh_ident[DTRACE_DOF_ID_MAG1] != 'D'
      || dof->dofh_ident[DTRACE_DOF_ID_MAG2] != 'O'
      || dof->dofh_ident[DTRACE_DOF_ID_MAG3] != 'F')
    goto invalid_dof_data;

  /* Make sure the encoding mark is either DTRACE_DOF_ENCODE_LSB or
     DTRACE_DOF_ENCODE_MSB.  */
  if (dof->dofh_ident[DTRACE_DOF_ID_ENCODING] != DTRACE_DOF_ENCODE_LSB
      && dof->dofh_ident[DTRACE_DOF_ID_ENCODING] != DTRACE_DOF_ENCODE_MSB)
    goto invalid_dof_data;

  /* Make sure this DOF is not an enabling DOF, i.e. there are no ECB
     Description sections.  */
  section = (struct dtrace_dof_sect *) DTRACE_DOF_PTR (dof,
						       DOF_UINT (dof, dof->dofh_secoff));
  for (i = 0; i < DOF_UINT (dof, dof->dofh_secnum); i++, section++)
    if (section->dofs_type == DTRACE_DOF_SECT_TYPE_ECBDESC)
      return;

  /* Iterate over any section of type Provider and extract the probe
     information from them.  If there are no "provider" sections on
     the DOF then we just return.  */
  section = (struct dtrace_dof_sect *) DTRACE_DOF_PTR (dof,
						       DOF_UINT (dof, dof->dofh_secoff));
  for (i = 0; i < DOF_UINT (dof, dof->dofh_secnum); i++, section++)
    if (DOF_UINT (dof, section->dofs_type) == DTRACE_DOF_SECT_TYPE_PROVIDER)
      {
	struct dtrace_dof_provider *provider = (struct dtrace_dof_provider *)
	  DTRACE_DOF_PTR (dof, DOF_UINT (dof, section->dofs_offset));
	struct dtrace_dof_sect *strtab_s
	  = DTRACE_DOF_SECT (dof, DOF_UINT (dof, provider->dofpv_strtab));
	struct dtrace_dof_sect *probes_s
	  = DTRACE_DOF_SECT (dof, DOF_UINT (dof, provider->dofpv_probes));
	struct dtrace_dof_sect *args_s
	  = DTRACE_DOF_SECT (dof, DOF_UINT (dof, provider->dofpv_prargs));
	struct dtrace_dof_sect *offsets_s
	  = DTRACE_DOF_SECT (dof, DOF_UINT (dof, provider->dofpv_proffs));
	struct dtrace_dof_sect *eoffsets_s
	  = DTRACE_DOF_SECT (dof, DOF_UINT (dof, provider->dofpv_prenoffs));
	char *strtab  = DTRACE_DOF_PTR (dof, DOF_UINT (dof, strtab_s->dofs_offset));
	char *offtab  = DTRACE_DOF_PTR (dof, DOF_UINT (dof, offsets_s->dofs_offset));
	char *eofftab = DTRACE_DOF_PTR (dof, DOF_UINT (dof, eoffsets_s->dofs_offset));
	char *argtab  = DTRACE_DOF_PTR (dof, DOF_UINT (dof, args_s->dofs_offset));
	unsigned int entsize = DOF_UINT (dof, probes_s->dofs_entsize);
	int num_probes;

	if (DOF_UINT (dof, section->dofs_size)
	    < sizeof (struct dtrace_dof_provider))
	  {
	    /* The section is smaller than expected, so do not use it.
	       This has been observed on x86-solaris 10.  */
	    goto invalid_dof_data;
	  }

	/* Very, unlikely, but could crash gdb if not handled
	   properly.  */
	if (entsize == 0)
	  goto invalid_dof_data;

	num_probes = DOF_UINT (dof, probes_s->dofs_size) / entsize;

	for (i = 0; i < num_probes; i++)
	  {
	    struct dtrace_dof_probe *probe = (struct dtrace_dof_probe *)
	      DTRACE_DOF_PTR (dof, DOF_UINT (dof, probes_s->dofs_offset)
			      + (i * DOF_UINT (dof, probes_s->dofs_entsize)));

	    dtrace_process_dof_probe (objfile,
				      gdbarch, probesp,
				      dof, probe,
				      provider, strtab, offtab, eofftab, argtab,
				      DOF_UINT (dof, strtab_s->dofs_size));
	  }
      }

  return;
	  
 invalid_dof_data:
  complaint (_("skipping section '%s' which does not contain valid DOF data."),
	     sect->name);
}

/* Implementation of 'build_arg_exprs' method.  */

void
dtrace_probe::build_arg_exprs (struct gdbarch *gdbarch)
{
  size_t argc = 0;
  m_args_expr_built = true;

  /* Iterate over the arguments in the probe and build the
     corresponding GDB internal expression that will generate the
     value of the argument when executed at the PC of the probe.  */
  for (dtrace_probe_arg &arg : m_args)
    {
      /* Initialize the expression builder.  The language does not
	 matter, since we are using our own parser.  */
      expr_builder builder (current_language, gdbarch);

      /* The argument value, which is ABI dependent and casted to
	 `long int'.  */
      expr::operation_up op = gdbarch_dtrace_parse_probe_argument (gdbarch,
								   argc);

      /* Casting to the expected type, but only if the type was
	 recognized at probe load time.  Otherwise the argument will
	 be evaluated as the long integer passed to the probe.  */
      if (arg.type != NULL)
	op = expr::make_operation<expr::unop_cast_operation> (std::move (op),
							      arg.type);

      builder.set_operation (std::move (op));
      arg.expr = builder.release ();
      ++argc;
    }
}

/* Implementation of 'get_arg_by_number' method.  */

struct dtrace_probe_arg *
dtrace_probe::get_arg_by_number (unsigned n, struct gdbarch *gdbarch)
{
  if (!m_args_expr_built)
    this->build_arg_exprs (gdbarch);

  if (n > m_args.size ())
    internal_error (_("Probe '%s' has %d arguments, but GDB is requesting\n"
		      "argument %u.  This should not happen.  Please\n"
		      "report this bug."),
		    this->get_name ().c_str (),
		    (int) m_args.size (), n);

  return &m_args[n];
}

/* Implementation of the probe is_enabled method.  */

bool
dtrace_probe::is_enabled () const
{
  struct gdbarch *gdbarch = this->get_gdbarch ();

  for (const dtrace_probe_enabler &enabler : m_enablers)
    if (!gdbarch_dtrace_probe_is_enabled (gdbarch, enabler.address))
      return false;

  return true;
}

/* Implementation of the get_probe_address method.  */

CORE_ADDR
dtrace_probe::get_relocated_address (struct objfile *objfile)
{
  return this->get_address () + objfile->text_section_offset ();
}

/* Implementation of the get_argument_count method.  */

unsigned
dtrace_probe::get_argument_count (struct gdbarch *gdbarch)
{
  return m_args.size ();
}

/* Implementation of the can_evaluate_arguments method.  */

bool
dtrace_probe::can_evaluate_arguments () const
{
  struct gdbarch *gdbarch = this->get_gdbarch ();

  return gdbarch_dtrace_parse_probe_argument_p (gdbarch);
}

/* Implementation of the evaluate_argument method.  */

struct value *
dtrace_probe::evaluate_argument (unsigned n,
				 frame_info_ptr frame)
{
  struct gdbarch *gdbarch = this->get_gdbarch ();
  struct dtrace_probe_arg *arg;

  arg = this->get_arg_by_number (n, gdbarch);
  return arg->expr->evaluate (arg->type);
}

/* Implementation of the compile_to_ax method.  */

void
dtrace_probe::compile_to_ax (struct agent_expr *expr, struct axs_value *value,
			     unsigned n)
{
  struct dtrace_probe_arg *arg;

  arg = this->get_arg_by_number (n, expr->gdbarch);
  arg->expr->op->generate_ax (arg->expr.get (), expr, value);

  require_rvalue (expr, value);
  value->type = arg->type;
}

/* Implementation of the 'get_static_ops' method.  */

const static_probe_ops *
dtrace_probe::get_static_ops () const
{
  return &dtrace_static_probe_ops;
}

/* Implementation of the gen_info_probes_table_values method.  */

std::vector<const char *>
dtrace_probe::gen_info_probes_table_values () const
{
  const char *val = NULL;

  if (m_enablers.empty ())
    val = "always";
  else if (!gdbarch_dtrace_probe_is_enabled_p (this->get_gdbarch ()))
    val = "unknown";
  else if (this->is_enabled ())
    val = "yes";
  else
    val = "no";

  return std::vector<const char *> { val };
}

/* Implementation of the enable method.  */

void
dtrace_probe::enable ()
{
  struct gdbarch *gdbarch = this->get_gdbarch ();

  /* Enabling a dtrace probe implies patching the text section of the
     running process, so make sure the inferior is indeed running.  */
  if (inferior_ptid == null_ptid)
    error (_("No inferior running"));

  /* Fast path.  */
  if (this->is_enabled ())
    return;

  /* Iterate over all defined enabler in the given probe and enable
     them all using the corresponding gdbarch hook.  */
  for (const dtrace_probe_enabler &enabler : m_enablers)
    if (gdbarch_dtrace_enable_probe_p (gdbarch))
      gdbarch_dtrace_enable_probe (gdbarch, enabler.address);
}


/* Implementation of the disable_probe method.  */

void
dtrace_probe::disable ()
{
  struct gdbarch *gdbarch = this->get_gdbarch ();

  /* Disabling a dtrace probe implies patching the text section of the
     running process, so make sure the inferior is indeed running.  */
  if (inferior_ptid == null_ptid)
    error (_("No inferior running"));

  /* Fast path.  */
  if (!this->is_enabled ())
    return;

  /* Are we trying to disable a probe that does not have any enabler
     associated?  */
  if (m_enablers.empty ())
    error (_("Probe %s:%s cannot be disabled: no enablers."),
	   this->get_provider ().c_str (), this->get_name ().c_str ());

  /* Iterate over all defined enabler in the given probe and disable
     them all using the corresponding gdbarch hook.  */
  for (dtrace_probe_enabler &enabler : m_enablers)
    if (gdbarch_dtrace_disable_probe_p (gdbarch))
      gdbarch_dtrace_disable_probe (gdbarch, enabler.address);
}

/* Implementation of the is_linespec method.  */

bool
dtrace_static_probe_ops::is_linespec (const char **linespecp) const
{
  static const char *const keywords[] = { "-pdtrace", "-probe-dtrace", NULL };

  return probe_is_linespec_by_keyword (linespecp, keywords);
}

/* Implementation of the get_probes method.  */

void
dtrace_static_probe_ops::get_probes
  (std::vector<std::unique_ptr<probe>> *probesp,
   struct objfile *objfile) const
{
  bfd *abfd = objfile->obfd.get ();
  asection *sect = NULL;

  /* Do nothing in case this is a .debug file, instead of the objfile
     itself.  */
  if (objfile->separate_debug_objfile_backlink != NULL)
    return;

  /* Iterate over the sections in OBJFILE looking for DTrace
     information.  */
  for (sect = abfd->sections; sect != NULL; sect = sect->next)
    {
      if (elf_section_data (sect)->this_hdr.sh_type == SHT_SUNW_dof)
	{
	  bfd_byte *dof;

	  /* Read the contents of the DOF section and then process it to
	     extract the information of any probe defined into it.  */
	  if (bfd_malloc_and_get_section (abfd, sect, &dof) && dof != NULL)
	    dtrace_process_dof (sect, objfile, probesp,
				(struct dtrace_dof_hdr *) dof);
	  else
	    complaint (_("could not obtain the contents of"
			 "section '%s' in objfile `%s'."),
		       bfd_section_name (sect), bfd_get_filename (abfd));

	  xfree (dof);
	}
    }
}

/* Implementation of the type_name method.  */

const char *
dtrace_static_probe_ops::type_name () const
{
  return "dtrace";
}

/* Implementation of the gen_info_probes_table_header method.  */

std::vector<struct info_probe_column>
dtrace_static_probe_ops::gen_info_probes_table_header () const
{
  struct info_probe_column dtrace_probe_column;

  dtrace_probe_column.field_name = "enabled";
  dtrace_probe_column.print_name = _("Enabled");

  return std::vector<struct info_probe_column> { dtrace_probe_column };
}

/* Implementation of the `info probes dtrace' command.  */

static void
info_probes_dtrace_command (const char *arg, int from_tty)
{
  info_probes_for_spops (arg, from_tty, &dtrace_static_probe_ops);
}

void _initialize_dtrace_probe ();
void
_initialize_dtrace_probe ()
{
  all_static_probe_ops.push_back (&dtrace_static_probe_ops);

  add_cmd ("dtrace", class_info, info_probes_dtrace_command,
	   _("\
Show information about DTrace static probes.\n\
Usage: info probes dtrace [PROVIDER [NAME [OBJECT]]]\n\
Each argument is a regular expression, used to select probes.\n\
PROVIDER matches probe provider names.\n\
NAME matches the probe names.\n\
OBJECT matches the executable or shared library name."),
	   info_probes_cmdlist_get ());
}
