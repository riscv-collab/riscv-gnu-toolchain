/* Generic SDT probe support for GDB.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

#if !defined (PROBE_H)
#define PROBE_H 1

#include "symtab.h"

struct location_spec;
struct linespec_result;

/* Structure useful for passing the header names in the method
   `gen_ui_out_table_header'.  */

struct info_probe_column
  {
    /* The internal name of the field.  This string cannot be capitalized nor
       localized, e.g., "extra_field".  */
    const char *field_name;

    /* The field name to be printed in the `info probes' command.  This
       string can be capitalized and localized, e.g., _("Extra Field").  */
    const char *print_name;
  };

/* Operations that act on probes, but are specific to each backend.
   These methods do not go into the 'class probe' because they do not
   act on a single probe; instead, they are used to operate on many
   probes at once, or to provide information about the probe backend
   itself, instead of a single probe.

   Each probe backend needs to inherit this class and implement all of
   the virtual functions specified here.  Then, an object shall be
   instantiated and added (or "registered") to the
   ALL_STATIC_PROBE_OPS vector so that the frontend probe interface
   can use it in the generic probe functions.  */

class static_probe_ops
{
public:
  /* Method responsible for verifying if LINESPECP is a valid linespec
     for a probe breakpoint.  It should return true if it is, or false
     if it is not.  It also should update LINESPECP in order to
     discard the breakpoint option associated with this linespec.  For
     example, if the option is `-probe', and the LINESPECP is `-probe
     abc', the function should return 1 and set LINESPECP to
     `abc'.  */
  virtual bool is_linespec (const char **linespecp) const = 0;

  /* Function that should fill PROBES with known probes from OBJFILE.  */
  virtual void get_probes (std::vector<std::unique_ptr<probe>> *probes,
			    struct objfile *objfile) const = 0;

  /* Return a pointer to a name identifying the probe type.  This is
     the string that will be displayed in the "Type" column of the
     `info probes' command.  */
  virtual const char *type_name () const = 0;

  /* Return true if the probe can be enabled; false otherwise.  */
  virtual bool can_enable () const
  {
    return false;
  }

  /* Function responsible for providing the extra fields that will be
     printed in the `info probes' command.  It should fill HEADS
     with whatever extra fields it needs.  If no extra fields are
     required by the probe backend, the method EMIT_INFO_PROBES_FIELDS
     should return false.  */
  virtual std::vector<struct info_probe_column>
    gen_info_probes_table_header () const = 0;
};

/* Definition of a vector of static_probe_ops.  */

extern std::vector<const static_probe_ops *> all_static_probe_ops;

/* Helper function that, given KEYWORDS, iterate over it trying to match
   each keyword with LINESPECP.  If it succeeds, it updates the LINESPECP
   pointer and returns 1.  Otherwise, nothing is done to LINESPECP and zero
   is returned.  */

extern int probe_is_linespec_by_keyword (const char **linespecp,
					 const char *const *keywords);

/* Return specific STATIC_PROBE_OPS * matching *LINESPECP and possibly
   updating LINESPECP to skip its "-probe-type " prefix.  Return
   &static_probe_ops_any if LINESPECP matches "-probe ", that is any
   unspecific probe.  Return NULL if LINESPECP is not identified as
   any known probe type, *LINESPECP is not modified in such case.  */

extern const static_probe_ops *
  probe_linespec_to_static_ops (const char **linespecp);

/* The probe itself.  The class contains generic information about the
   probe.  */

class probe
{
public:
  /* Default constructor for a probe.  */
  probe (std::string &&name_, std::string &&provider_, CORE_ADDR address_,
	 struct gdbarch *arch_)
    : m_name (std::move (name_)), m_provider (std::move (provider_)),
      m_address (address_), m_arch (arch_)
  {}

  /* Virtual destructor.  */
  virtual ~probe ()
  {}

  /* Compute the probe's relocated address.  OBJFILE is the objfile
     in which the probe originated.  */
  virtual CORE_ADDR get_relocated_address (struct objfile *objfile) = 0;

  /* Return the number of arguments of the probe.  This function can
     throw an exception.  */
  virtual unsigned get_argument_count (struct gdbarch *gdbarch) = 0;

  /* Return 1 if the probe interface can evaluate the arguments of
     probe, zero otherwise.  See the comments on
     sym_probe_fns:can_evaluate_probe_arguments for more
     details.  */
  virtual bool can_evaluate_arguments () const = 0;

  /* Evaluate the Nth argument from the probe, returning a value
     corresponding to it.  The argument number is represented N.
     This function can throw an exception.  */
  virtual struct value *evaluate_argument (unsigned n,
					   frame_info_ptr frame) = 0;

  /* Compile the Nth argument of the probe to an agent expression.
     The argument number is represented by N.  */
  virtual void compile_to_ax (struct agent_expr *aexpr,
			      struct axs_value *axs_value,
			      unsigned n) = 0;

  /* Set the semaphore associated with the probe.  This function only
     makes sense if the probe has a concept of semaphore associated to
     a probe.  */
  virtual void set_semaphore (struct objfile *objfile,
			      struct gdbarch *gdbarch)
  {}

  /* Clear the semaphore associated with the probe.  This function
     only makes sense if the probe has a concept of semaphore
     associated to a probe.  */
  virtual void clear_semaphore (struct objfile *objfile,
				struct gdbarch *gdbarch)
  {}

  /* Return the pointer to the static_probe_ops instance related to
     the probe type.  */
  virtual const static_probe_ops *get_static_ops () const = 0;

  /* Function that will fill VALUES with the values of the extra
     fields to be printed for the probe.

     If the backend implements the `gen_ui_out_table_header' method,
     then it should implement this method as well.  The backend should
     also guarantee that the order and the number of values in the
     vector is exactly the same as the order of the extra fields
     provided in the method `gen_ui_out_table_header'.  If a certain
     field is to be skipped when printing the information, you can
     push a NULL value in that position in the vector.  */
  virtual std::vector<const char *> gen_info_probes_table_values () const
  {
    return std::vector<const char *> ();
  }

  /* Enable the probe.  The semantics of "enabling" a probe depend on
     the specific backend.  This function can throw an exception.  */
  virtual void enable ()
  {}

  /* Disable the probe.  The semantics of "disabling" a probe depend
     on the specific backend.  This function can throw an
     exception.  */
  virtual void disable ()
  {}

  /* Getter for M_NAME.  */
  const std::string &get_name () const
  {
    return m_name;
  }

  /* Getter for M_PROVIDER.  */
  const std::string &get_provider () const
  {
    return m_provider;
  }

  /* Getter for M_ADDRESS.  */
  CORE_ADDR get_address () const
  {
    return m_address;
  }

  /* Getter for M_ARCH.  */
  struct gdbarch *get_gdbarch () const
  {
    return m_arch;
  }

private:
  /* The name of the probe.  */
  std::string m_name;

  /* The provider of the probe.  It generally defaults to the name of
     the objfile which contains the probe.  */
  std::string m_provider;

  /* The address where the probe is inserted, relative to
     SECT_OFF_TEXT.  */
  CORE_ADDR m_address;

  /* The probe's architecture.  */
  struct gdbarch *m_arch;
};

/* A bound probe holds a pointer to a probe and a pointer to the
   probe's defining objfile.  This is needed because probes are
   independent of the program space and thus require relocation at
   their point of use.  */

struct bound_probe
{
  /* Create an empty bound_probe object.  */
  bound_probe ()
  {}

  /* Create and initialize a bound_probe object using PROBE and OBJFILE.  */
  bound_probe (probe *probe_, struct objfile *objfile_)
  : prob (probe_), objfile (objfile_)
  {}

  /* The probe.  */
  probe *prob = NULL;

  /* The objfile in which the probe originated.  */
  struct objfile *objfile = NULL;
};

/* A helper for linespec that decodes a probe specification.  It
   returns a std::vector<symtab_and_line> object and updates LOC or
   throws an error.  */

extern std::vector<symtab_and_line> parse_probes
  (const location_spec *locspec,
   struct program_space *pspace,
   struct linespec_result *canon);

/* Given a PC, find an associated probe.  If a probe is found, return
   it.  If no probe is found, return a bound probe whose fields are
   both NULL.  */

extern struct bound_probe find_probe_by_pc (CORE_ADDR pc);

/* Search OBJFILE for a probe with the given PROVIDER, NAME.  Return a
   vector of all probes that were found.  If no matching probe is found,
   return an empty vector.  */

extern std::vector<probe *> find_probes_in_objfile (struct objfile *objfile,
						    const char *provider,
						    const char *name);

/* Generate a `info probes' command output for probes associated with
   SPOPS.  If SPOPS is related to the "any probe" type, then all probe
   types are considered.  It is a helper function that can be used by
   the probe backends to print their `info probe TYPE'.  */

extern void info_probes_for_spops (const char *arg, int from_tty,
				   const static_probe_ops *spops);

/* Return the `cmd_list_element' associated with the `info probes' command,
   or create a new one if it doesn't exist.  Helper function that serves the
   purpose of avoiding the case of a backend using the `cmd_list_element'
   associated with `info probes', without having it registered yet.  */

extern struct cmd_list_element **info_probes_cmdlist_get (void);

/* A convenience function that finds a probe at the PC in FRAME and
   evaluates argument N, with 0 <= N < number_of_args.  If there is no
   probe at that location, or if the probe does not have enough arguments,
   this returns NULL.  */

extern struct value *probe_safe_evaluate_at_pc (frame_info_ptr frame,
						unsigned n);

/* Return true if the PROVIDER/NAME probe from OBJFILE_NAME needs to be
   ignored.  */

bool ignore_probe_p (const char *provider, const char *name,
		     const char *objfile_name, const char *TYPE);
#endif /* !defined (PROBE_H) */
