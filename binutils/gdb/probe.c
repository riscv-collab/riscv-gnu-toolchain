/* Generic static probe support for GDB.

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

#include "defs.h"
#include "probe.h"
#include "command.h"
#include "cli/cli-cmds.h"
#include "cli/cli-utils.h"
#include "objfiles.h"
#include "symtab.h"
#include "progspace.h"
#include "filenames.h"
#include "linespec.h"
#include "gdbsupport/gdb_regex.h"
#include "frame.h"
#include "arch-utils.h"
#include "value.h"
#include "ax.h"
#include "ax-gdb.h"
#include "location.h"
#include <ctype.h>
#include <algorithm>
#include <optional>

/* Class that implements the static probe methods for "any" probe.  */

class any_static_probe_ops : public static_probe_ops
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
  std::vector<struct info_probe_column> gen_info_probes_table_header
    () const override;
};

/* Static operations associated with a generic probe.  */

const any_static_probe_ops any_static_probe_ops {};

/* A helper for parse_probes that decodes a probe specification in
   SEARCH_PSPACE.  It appends matching SALs to RESULT.  */

static void
parse_probes_in_pspace (const static_probe_ops *spops,
			struct program_space *search_pspace,
			const char *objfile_namestr,
			const char *provider,
			const char *name,
			std::vector<symtab_and_line> *result)
{
  for (objfile *objfile : search_pspace->objfiles ())
    {
      if (!objfile->sf || !objfile->sf->sym_probe_fns)
	continue;

      if (objfile_namestr
	  && FILENAME_CMP (objfile_name (objfile), objfile_namestr) != 0
	  && FILENAME_CMP (lbasename (objfile_name (objfile)),
			   objfile_namestr) != 0)
	continue;

      const std::vector<std::unique_ptr<probe>> &probes
	= objfile->sf->sym_probe_fns->sym_get_probes (objfile);

      for (auto &p : probes)
	{
	  if (spops != &any_static_probe_ops && p->get_static_ops () != spops)
	    continue;

	  if (provider != NULL && p->get_provider () != provider)
	    continue;

	  if (p->get_name () != name)
	    continue;

	  symtab_and_line sal;
	  sal.pc = p->get_relocated_address (objfile);
	  sal.explicit_pc = 1;
	  sal.section = find_pc_overlay (sal.pc);
	  sal.pspace = search_pspace;
	  sal.prob = p.get ();
	  sal.objfile = objfile;

	  result->push_back (std::move (sal));
	}
    }
}

/* See definition in probe.h.  */

std::vector<symtab_and_line>
parse_probes (const location_spec *locspec,
	      struct program_space *search_pspace,
	      struct linespec_result *canonical)
{
  char *arg_end, *arg;
  char *objfile_namestr = NULL, *provider = NULL, *name, *p;
  const char *arg_start, *cs;

  gdb_assert (locspec->type () == PROBE_LOCATION_SPEC);
  arg_start = locspec->to_string ();

  cs = arg_start;
  const static_probe_ops *spops = probe_linespec_to_static_ops (&cs);
  if (spops == NULL)
    error (_("'%s' is not a probe linespec"), arg_start);

  arg = (char *) cs;
  arg = skip_spaces (arg);
  if (!*arg)
    error (_("argument to `%s' missing"), arg_start);

  arg_end = skip_to_space (arg);

  /* We make a copy here so we can write over parts with impunity.  */
  std::string copy (arg, arg_end - arg);
  arg = &copy[0];

  /* Extract each word from the argument, separated by ":"s.  */
  p = strchr (arg, ':');
  if (p == NULL)
    {
      /* This is `-p name'.  */
      name = arg;
    }
  else
    {
      char *hold = p + 1;

      *p = '\0';
      p = strchr (hold, ':');
      if (p == NULL)
	{
	  /* This is `-p provider:name'.  */
	  provider = arg;
	  name = hold;
	}
      else
	{
	  /* This is `-p objfile:provider:name'.  */
	  *p = '\0';
	  objfile_namestr = arg;
	  provider = hold;
	  name = p + 1;
	}
    }

  if (*name == '\0')
    error (_("no probe name specified"));
  if (provider && *provider == '\0')
    error (_("invalid provider name"));
  if (objfile_namestr && *objfile_namestr == '\0')
    error (_("invalid objfile name"));

  std::vector<symtab_and_line> result;
  if (search_pspace != NULL)
    {
      parse_probes_in_pspace (spops, search_pspace, objfile_namestr,
			      provider, name, &result);
    }
  else
    {
      for (struct program_space *pspace : program_spaces)
	parse_probes_in_pspace (spops, pspace, objfile_namestr,
				provider, name, &result);
    }

  if (result.empty ())
    {
      throw_error (NOT_FOUND_ERROR,
		   _("No probe matching objfile=`%s', provider=`%s', name=`%s'"),
		   objfile_namestr ? objfile_namestr : _("<any>"),
		   provider ? provider : _("<any>"),
		   name);
    }

  if (canonical)
    {
      std::string canon (arg_start, arg_end - arg_start);
      canonical->special_display = 1;
      canonical->pre_expanded = 1;
      canonical->locspec = new_probe_location_spec (std::move (canon));
    }

  return result;
}

/* See definition in probe.h.  */

std::vector<probe *>
find_probes_in_objfile (struct objfile *objfile, const char *provider,
			const char *name)
{
  std::vector<probe *> result;

  if (!objfile->sf || !objfile->sf->sym_probe_fns)
    return result;

  const std::vector<std::unique_ptr<probe>> &probes
    = objfile->sf->sym_probe_fns->sym_get_probes (objfile);
  for (auto &p : probes)
    {
      if (p->get_provider () != provider)
	continue;

      if (p->get_name () != name)
	continue;

      result.push_back (p.get ());
    }

  return result;
}

/* See definition in probe.h.  */

struct bound_probe
find_probe_by_pc (CORE_ADDR pc)
{
  struct bound_probe result;

  result.objfile = NULL;
  result.prob = NULL;

  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (!objfile->sf || !objfile->sf->sym_probe_fns
	  || objfile->sect_index_text == -1)
	continue;

      /* If this proves too inefficient, we can replace with a hash.  */
      const std::vector<std::unique_ptr<probe>> &probes
	= objfile->sf->sym_probe_fns->sym_get_probes (objfile);
      for (auto &p : probes)
	if (p->get_relocated_address (objfile) == pc)
	  {
	    result.objfile = objfile;
	    result.prob = p.get ();
	    return result;
	  }
    }

  return result;
}



/* Make a vector of probes matching OBJNAME, PROVIDER, and PROBE_NAME.
   If SPOPS is not &any_static_probe_ops, only probes related to this
   specific static probe ops will match.  Each argument is a regexp,
   or NULL, which matches anything.  */

static std::vector<bound_probe>
collect_probes (const std::string &objname, const std::string &provider,
		const std::string &probe_name, const static_probe_ops *spops)
{
  std::vector<bound_probe> result;
  std::optional<compiled_regex> obj_pat, prov_pat, probe_pat;

  if (!provider.empty ())
    prov_pat.emplace (provider.c_str (), REG_NOSUB,
		      _("Invalid provider regexp"));
  if (!probe_name.empty ())
    probe_pat.emplace (probe_name.c_str (), REG_NOSUB,
		       _("Invalid probe regexp"));
  if (!objname.empty ())
    obj_pat.emplace (objname.c_str (), REG_NOSUB,
		     _("Invalid object file regexp"));

  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (! objfile->sf || ! objfile->sf->sym_probe_fns)
	continue;

      if (obj_pat)
	{
	  if (obj_pat->exec (objfile_name (objfile), 0, NULL, 0) != 0)
	    continue;
	}

      const std::vector<std::unique_ptr<probe>> &probes
	= objfile->sf->sym_probe_fns->sym_get_probes (objfile);

      for (auto &p : probes)
	{
	  if (spops != &any_static_probe_ops && p->get_static_ops () != spops)
	    continue;

	  if (prov_pat
	      && prov_pat->exec (p->get_provider ().c_str (), 0, NULL, 0) != 0)
	    continue;

	  if (probe_pat
	      && probe_pat->exec (p->get_name ().c_str (), 0, NULL, 0) != 0)
	    continue;

	  result.emplace_back (p.get (), objfile);
	}
    }

  return result;
}

/* A qsort comparison function for bound_probe_s objects.  */

static bool
compare_probes (const bound_probe &a, const bound_probe &b)
{
  int v;

  v = a.prob->get_provider ().compare (b.prob->get_provider ());
  if (v != 0)
    return v < 0;

  v = a.prob->get_name ().compare (b.prob->get_name ());
  if (v != 0)
    return v < 0;

  if (a.prob->get_address () != b.prob->get_address ())
    return a.prob->get_address () < b.prob->get_address ();

  return strcmp (objfile_name (a.objfile), objfile_name (b.objfile)) < 0;
}

/* Helper function that generate entries in the ui_out table being
   crafted by `info_probes_for_ops'.  */

static void
gen_ui_out_table_header_info (const std::vector<bound_probe> &probes,
			      const static_probe_ops *spops)
{
  /* `headings' refers to the names of the columns when printing `info
     probes'.  */
  gdb_assert (spops != NULL);

  std::vector<struct info_probe_column> headings
    = spops->gen_info_probes_table_header ();

  for (const info_probe_column &column : headings)
    {
      size_t size_max = strlen (column.print_name);

      for (const bound_probe &probe : probes)
	{
	  /* `probe_fields' refers to the values of each new field that this
	     probe will display.  */

	  if (probe.prob->get_static_ops () != spops)
	    continue;

	  std::vector<const char *> probe_fields
	    = probe.prob->gen_info_probes_table_values ();

	  gdb_assert (probe_fields.size () == headings.size ());

	  for (const char *val : probe_fields)
	    {
	      /* It is valid to have a NULL value here, which means that the
		 backend does not have something to write and this particular
		 field should be skipped.  */
	      if (val == NULL)
		continue;

	      size_max = std::max (strlen (val), size_max);
	    }
	}

      current_uiout->table_header (size_max, ui_left,
				   column.field_name, column.print_name);
    }
}

/* Helper function to print not-applicable strings for all the extra
   columns defined in a static_probe_ops.  */

static void
print_ui_out_not_applicables (const static_probe_ops *spops)
{
   std::vector<struct info_probe_column> headings
     = spops->gen_info_probes_table_header ();

  for (const info_probe_column &column : headings)
    current_uiout->field_string (column.field_name, _("n/a"));
}

/* Helper function to print extra information about a probe and an objfile
   represented by PROBE.  */

static void
print_ui_out_info (probe *probe)
{
  /* `values' refers to the actual values of each new field in the output
     of `info probe'.  `headings' refers to the names of each new field.  */
  gdb_assert (probe != NULL);
  std::vector<struct info_probe_column> headings
    = probe->get_static_ops ()->gen_info_probes_table_header ();
  std::vector<const char *> values
    = probe->gen_info_probes_table_values ();

  gdb_assert (headings.size () == values.size ());

  for (int ix = 0; ix < headings.size (); ++ix)
    {
      struct info_probe_column column = headings[ix];
      const char *val = values[ix];

      if (val == NULL)
	current_uiout->field_skip (column.field_name);
      else
	current_uiout->field_string (column.field_name, val);
    }
}

/* Helper function that returns the number of extra fields which POPS will
   need.  */

static int
get_number_extra_fields (const static_probe_ops *spops)
{
  return spops->gen_info_probes_table_header ().size ();
}

/* Helper function that returns true if there is a probe in PROBES
   featuring the given SPOPS.  It returns false otherwise.  */

static bool
exists_probe_with_spops (const std::vector<bound_probe> &probes,
			 const static_probe_ops *spops)
{
  for (const bound_probe &probe : probes)
    if (probe.prob->get_static_ops () == spops)
      return true;

  return false;
}

/* Helper function that parses a probe linespec of the form [PROVIDER
   [PROBE [OBJNAME]]] from the provided string STR.  */

static void
parse_probe_linespec (const char *str, std::string *provider,
		      std::string *probe_name, std::string *objname)
{
  *probe_name = *objname = "";

  *provider = extract_arg (&str);
  if (!provider->empty ())
    {
      *probe_name = extract_arg (&str);
      if (!probe_name->empty ())
	*objname = extract_arg (&str);
    }
}

/* See comment in probe.h.  */

void
info_probes_for_spops (const char *arg, int from_tty,
		       const static_probe_ops *spops)
{
  std::string provider, probe_name, objname;
  int any_found;
  int ui_out_extra_fields = 0;
  size_t size_addr;
  size_t size_name = strlen ("Name");
  size_t size_objname = strlen ("Object");
  size_t size_provider = strlen ("Provider");
  size_t size_type = strlen ("Type");
  struct gdbarch *gdbarch = get_current_arch ();

  parse_probe_linespec (arg, &provider, &probe_name, &objname);

  std::vector<bound_probe> probes
    = collect_probes (objname, provider, probe_name, spops);

  if (spops == &any_static_probe_ops)
    {
      /* If SPOPS is &any_static_probe_ops, it means the user has
	 requested a "simple" `info probes', i.e., she wants to print
	 all information about all probes.  For that, we have to
	 identify how many extra fields we will need to add in the
	 ui_out table.

	 To do that, we iterate over all static_probe_ops, querying
	 each one about its extra fields, and incrementing
	 `ui_out_extra_fields' to reflect that number.  But note that
	 we ignore the static_probe_ops for which no probes are
	 defined with the given search criteria.  */

      for (const static_probe_ops *po : all_static_probe_ops)
	if (exists_probe_with_spops (probes, po))
	  ui_out_extra_fields += get_number_extra_fields (po);
    }
  else
    ui_out_extra_fields = get_number_extra_fields (spops);

  {
    ui_out_emit_table table_emitter (current_uiout,
				     5 + ui_out_extra_fields,
				     probes.size (), "StaticProbes");

    std::sort (probes.begin (), probes.end (), compare_probes);

    /* What's the size of an address in our architecture?  */
    size_addr = gdbarch_addr_bit (gdbarch) == 64 ? 18 : 10;

    /* Determining the maximum size of each field (`type', `provider',
       `name' and `objname').  */
    for (const bound_probe &probe : probes)
      {
	const char *probe_type = probe.prob->get_static_ops ()->type_name ();

	size_type = std::max (strlen (probe_type), size_type);
	size_name = std::max (probe.prob->get_name ().size (), size_name);
	size_provider = std::max (probe.prob->get_provider ().size (),
				  size_provider);
	size_objname = std::max (strlen (objfile_name (probe.objfile)),
				 size_objname);
      }

    current_uiout->table_header (size_type, ui_left, "type", _("Type"));
    current_uiout->table_header (size_provider, ui_left, "provider",
				 _("Provider"));
    current_uiout->table_header (size_name, ui_left, "name", _("Name"));
    current_uiout->table_header (size_addr, ui_left, "addr", _("Where"));

    if (spops == &any_static_probe_ops)
      {
	/* We have to generate the table header for each new probe type
	   that we will print.  Note that this excludes probe types not
	   having any defined probe with the search criteria.  */
	for (const static_probe_ops *po : all_static_probe_ops)
	  if (exists_probe_with_spops (probes, po))
	    gen_ui_out_table_header_info (probes, po);
      }
    else
      gen_ui_out_table_header_info (probes, spops);

    current_uiout->table_header (size_objname, ui_left, "object", _("Object"));
    current_uiout->table_body ();

    for (const bound_probe &probe : probes)
      {
	const char *probe_type = probe.prob->get_static_ops ()->type_name ();

	ui_out_emit_tuple tuple_emitter (current_uiout, "probe");

	current_uiout->field_string ("type", probe_type);
	current_uiout->field_string ("provider", probe.prob->get_provider ());
	current_uiout->field_string ("name", probe.prob->get_name ());
	current_uiout->field_core_addr ("addr", probe.prob->get_gdbarch (),
					probe.prob->get_relocated_address
					(probe.objfile));

	if (spops == &any_static_probe_ops)
	  {
	    for (const static_probe_ops *po : all_static_probe_ops)
	      {
		if (probe.prob->get_static_ops () == po)
		  print_ui_out_info (probe.prob);
		else if (exists_probe_with_spops (probes, po))
		  print_ui_out_not_applicables (po);
	      }
	  }
	else
	  print_ui_out_info (probe.prob);

	current_uiout->field_string ("object",
				     objfile_name (probe.objfile));
	current_uiout->text ("\n");
      }

    any_found = !probes.empty ();
  }

  if (!any_found)
    current_uiout->message (_("No probes matched.\n"));
}

/* Implementation of the `info probes' command.  */

static void
info_probes_command (const char *arg, int from_tty)
{
  info_probes_for_spops (arg, from_tty, &any_static_probe_ops);
}

/* Implementation of the `enable probes' command.  */

static void
enable_probes_command (const char *arg, int from_tty)
{
  std::string provider, probe_name, objname;

  parse_probe_linespec (arg, &provider, &probe_name, &objname);

  std::vector<bound_probe> probes
    = collect_probes (objname, provider, probe_name, &any_static_probe_ops);
  if (probes.empty ())
    {
      current_uiout->message (_("No probes matched.\n"));
      return;
    }

  /* Enable the selected probes, provided their backends support the
     notion of enabling a probe.  */
  for (const bound_probe &probe: probes)
    {
      if (probe.prob->get_static_ops ()->can_enable ())
	{
	  probe.prob->enable ();
	  current_uiout->message (_("Probe %s:%s enabled.\n"),
				  probe.prob->get_provider ().c_str (),
				  probe.prob->get_name ().c_str ());
	}
      else
	current_uiout->message (_("Probe %s:%s cannot be enabled.\n"),
				probe.prob->get_provider ().c_str (),
				probe.prob->get_name ().c_str ());
    }
}

/* Implementation of the `disable probes' command.  */

static void
disable_probes_command (const char *arg, int from_tty)
{
  std::string provider, probe_name, objname;

  parse_probe_linespec (arg, &provider, &probe_name, &objname);

  std::vector<bound_probe> probes
    = collect_probes (objname, provider, probe_name, &any_static_probe_ops);
  if (probes.empty ())
    {
      current_uiout->message (_("No probes matched.\n"));
      return;
    }

  /* Disable the selected probes, provided their backends support the
     notion of enabling a probe.  */
  for (const bound_probe &probe : probes)
    {
      if (probe.prob->get_static_ops ()->can_enable ())
	{
	  probe.prob->disable ();
	  current_uiout->message (_("Probe %s:%s disabled.\n"),
				  probe.prob->get_provider ().c_str (),
				  probe.prob->get_name ().c_str ());
	}
      else
	current_uiout->message (_("Probe %s:%s cannot be disabled.\n"),
				probe.prob->get_provider ().c_str (),
				probe.prob->get_name ().c_str ());
    }
}

static bool ignore_probes_p = false;
static bool ignore_probes_idx = 0;
static bool ignore_probes_verbose_p;
static std::optional<compiled_regex> ignore_probes_prov_pat[2];
static std::optional<compiled_regex> ignore_probes_name_pat[2];
static std::optional<compiled_regex> ignore_probes_obj_pat[2];

/* See comments in probe.h.  */

bool
ignore_probe_p (const char *provider, const char *name,
		const char *objfile_name, const char *type)
{
  if (!ignore_probes_p)
    return false;

  std::optional<compiled_regex> &re_prov
    = ignore_probes_prov_pat[ignore_probes_idx];
  std::optional<compiled_regex> &re_name
    = ignore_probes_name_pat[ignore_probes_idx];
  std::optional<compiled_regex> &re_obj
    = ignore_probes_obj_pat[ignore_probes_idx];

  bool res
    = ((!re_prov
	|| re_prov->exec (provider, 0, NULL, 0) == 0)
       && (!re_name
	   || re_name->exec (name, 0, NULL, 0) == 0)
       && (!re_obj
	   || re_obj->exec (objfile_name, 0, NULL, 0) == 0));

  if (res && ignore_probes_verbose_p)
    gdb_printf (gdb_stdlog, _("Ignoring %s probe %s %s in %s.\n"),
		type, provider, name, objfile_name);

  return res;
}

/* Implementation of the `maintenance ignore-probes' command.  */

static void
ignore_probes_command (const char *arg, int from_tty)
{
  std::string ignore_provider, ignore_probe_name, ignore_objname;

  bool verbose_p = false;
  if (arg != nullptr)
    {
      const char *idx = arg;
      std::string s = extract_arg (&idx);

      if (strcmp (s.c_str (), "-reset") == 0)
	{
	  if (*idx != '\0')
	    error (_("-reset: no arguments allowed"));

	  ignore_probes_p = false;
	  gdb_printf (gdb_stdout, _("ignore-probes filter has been reset\n"));
	  return;
	}

      if (strcmp (s.c_str (), "-verbose") == 0
	  || strcmp (s.c_str (), "-v") == 0)
	{
	  verbose_p = true;
	  arg = idx;
	}
    }

  parse_probe_linespec (arg, &ignore_provider, &ignore_probe_name,
			&ignore_objname);

  /* Parse the regular expressions, making sure that the old regular
     expressions are still valid if an exception is throw.  */
  int new_ignore_probes_idx = 1 - ignore_probes_idx;
  std::optional<compiled_regex> &re_prov
    = ignore_probes_prov_pat[new_ignore_probes_idx];
  std::optional<compiled_regex> &re_name
    = ignore_probes_name_pat[new_ignore_probes_idx];
  std::optional<compiled_regex> &re_obj
    = ignore_probes_obj_pat[new_ignore_probes_idx];
  re_prov.reset ();
  re_name.reset ();
  re_obj.reset ();
  if (!ignore_provider.empty ())
    re_prov.emplace (ignore_provider.c_str (), REG_NOSUB,
		     _("Invalid provider regexp"));
  if (!ignore_probe_name.empty ())
    re_name.emplace (ignore_probe_name.c_str (), REG_NOSUB,
		     _("Invalid probe regexp"));
  if (!ignore_objname.empty ())
    re_obj.emplace (ignore_objname.c_str (), REG_NOSUB,
		    _("Invalid object file regexp"));
  ignore_probes_idx = new_ignore_probes_idx;

  ignore_probes_p = true;
  ignore_probes_verbose_p = verbose_p;
  gdb_printf (gdb_stdout, _("ignore-probes filter has been set to:\n"));
  gdb_printf (gdb_stdout, _("PROVIDER: '%s'\n"), ignore_provider.c_str ());
  gdb_printf (gdb_stdout, _("PROBE_NAME: '%s'\n"), ignore_probe_name.c_str ());
  gdb_printf (gdb_stdout, _("OBJNAME: '%s'\n"), ignore_objname.c_str ());
}

/* See comments in probe.h.  */

struct value *
probe_safe_evaluate_at_pc (frame_info_ptr frame, unsigned n)
{
  struct bound_probe probe;
  unsigned n_args;

  probe = find_probe_by_pc (get_frame_pc (frame));
  if (!probe.prob)
    return NULL;

  n_args = probe.prob->get_argument_count (get_frame_arch (frame));
  if (n >= n_args)
    return NULL;

  return probe.prob->evaluate_argument (n, frame);
}

/* See comment in probe.h.  */

const struct static_probe_ops *
probe_linespec_to_static_ops (const char **linespecp)
{
  for (const static_probe_ops *ops : all_static_probe_ops)
    if (ops->is_linespec (linespecp))
      return ops;

  return NULL;
}

/* See comment in probe.h.  */

int
probe_is_linespec_by_keyword (const char **linespecp, const char *const *keywords)
{
  const char *s = *linespecp;
  const char *const *csp;

  for (csp = keywords; *csp; csp++)
    {
      const char *keyword = *csp;
      size_t len = strlen (keyword);

      if (strncmp (s, keyword, len) == 0 && isspace (s[len]))
	{
	  *linespecp += len + 1;
	  return 1;
	}
    }

  return 0;
}

/* Implementation of `is_linespec' method.  */

bool
any_static_probe_ops::is_linespec (const char **linespecp) const
{
  static const char *const keywords[] = { "-p", "-probe", NULL };

  return probe_is_linespec_by_keyword (linespecp, keywords);
}

/* Implementation of 'get_probes' method.  */

void
any_static_probe_ops::get_probes (std::vector<std::unique_ptr<probe>> *probesp,
				  struct objfile *objfile) const
{
  /* No probes can be provided by this dummy backend.  */
}

/* Implementation of the 'type_name' method.  */

const char *
any_static_probe_ops::type_name () const
{
  return NULL;
}

/* Implementation of the 'gen_info_probes_table_header' method.  */

std::vector<struct info_probe_column>
any_static_probe_ops::gen_info_probes_table_header () const
{
  return std::vector<struct info_probe_column> ();
}

/* See comments in probe.h.  */

struct cmd_list_element **
info_probes_cmdlist_get (void)
{
  static struct cmd_list_element *info_probes_cmdlist;

  if (info_probes_cmdlist == NULL)
    add_prefix_cmd ("probes", class_info, info_probes_command,
		    _("\
Show available static probes.\n\
Usage: info probes [all|TYPE [ARGS]]\n\
TYPE specifies the type of the probe, and can be one of the following:\n\
  - stap\n\
If you specify TYPE, there may be additional arguments needed by the\n\
subcommand.\n\
If you do not specify any argument, or specify `all', then the command\n\
will show information about all types of probes."),
		    &info_probes_cmdlist, 0/*allow-unknown*/, &infolist);

  return &info_probes_cmdlist;
}



/* This is called to compute the value of one of the $_probe_arg*
   convenience variables.  */

static struct value *
compute_probe_arg (struct gdbarch *arch, struct internalvar *ivar,
		   void *data)
{
  frame_info_ptr frame = get_selected_frame (_("No frame selected"));
  CORE_ADDR pc = get_frame_pc (frame);
  int sel = (int) (uintptr_t) data;
  struct bound_probe pc_probe;
  unsigned n_args;

  /* SEL == -1 means "_probe_argc".  */
  gdb_assert (sel >= -1);

  pc_probe = find_probe_by_pc (pc);
  if (pc_probe.prob == NULL)
    error (_("No probe at PC %s"), core_addr_to_string (pc));

  n_args = pc_probe.prob->get_argument_count (arch);
  if (sel == -1)
    return value_from_longest (builtin_type (arch)->builtin_int, n_args);

  if (sel >= n_args)
    error (_("Invalid probe argument %d -- probe has %u arguments available"),
	   sel, n_args);

  return pc_probe.prob->evaluate_argument (sel, frame);
}

/* This is called to compile one of the $_probe_arg* convenience
   variables into an agent expression.  */

static void
compile_probe_arg (struct internalvar *ivar, struct agent_expr *expr,
		   struct axs_value *value, void *data)
{
  CORE_ADDR pc = expr->scope;
  int sel = (int) (uintptr_t) data;
  struct bound_probe pc_probe;
  int n_args;

  /* SEL == -1 means "_probe_argc".  */
  gdb_assert (sel >= -1);

  pc_probe = find_probe_by_pc (pc);
  if (pc_probe.prob == NULL)
    error (_("No probe at PC %s"), core_addr_to_string (pc));

  n_args = pc_probe.prob->get_argument_count (expr->gdbarch);

  if (sel == -1)
    {
      value->kind = axs_rvalue;
      value->type = builtin_type (expr->gdbarch)->builtin_int;
      ax_const_l (expr, n_args);
      return;
    }

  gdb_assert (sel >= 0);
  if (sel >= n_args)
    error (_("Invalid probe argument %d -- probe has %d arguments available"),
	   sel, n_args);

  pc_probe.prob->compile_to_ax (expr, value, sel);
}

static const struct internalvar_funcs probe_funcs =
{
  compute_probe_arg,
  compile_probe_arg,
};


std::vector<const static_probe_ops *> all_static_probe_ops;

void _initialize_probe ();
void
_initialize_probe ()
{
  all_static_probe_ops.push_back (&any_static_probe_ops);

  create_internalvar_type_lazy ("_probe_argc", &probe_funcs,
				(void *) (uintptr_t) -1);
  create_internalvar_type_lazy ("_probe_arg0", &probe_funcs,
				(void *) (uintptr_t) 0);
  create_internalvar_type_lazy ("_probe_arg1", &probe_funcs,
				(void *) (uintptr_t) 1);
  create_internalvar_type_lazy ("_probe_arg2", &probe_funcs,
				(void *) (uintptr_t) 2);
  create_internalvar_type_lazy ("_probe_arg3", &probe_funcs,
				(void *) (uintptr_t) 3);
  create_internalvar_type_lazy ("_probe_arg4", &probe_funcs,
				(void *) (uintptr_t) 4);
  create_internalvar_type_lazy ("_probe_arg5", &probe_funcs,
				(void *) (uintptr_t) 5);
  create_internalvar_type_lazy ("_probe_arg6", &probe_funcs,
				(void *) (uintptr_t) 6);
  create_internalvar_type_lazy ("_probe_arg7", &probe_funcs,
				(void *) (uintptr_t) 7);
  create_internalvar_type_lazy ("_probe_arg8", &probe_funcs,
				(void *) (uintptr_t) 8);
  create_internalvar_type_lazy ("_probe_arg9", &probe_funcs,
				(void *) (uintptr_t) 9);
  create_internalvar_type_lazy ("_probe_arg10", &probe_funcs,
				(void *) (uintptr_t) 10);
  create_internalvar_type_lazy ("_probe_arg11", &probe_funcs,
				(void *) (uintptr_t) 11);

  add_cmd ("all", class_info, info_probes_command,
	   _("\
Show information about all type of probes."),
	   info_probes_cmdlist_get ());

  add_cmd ("probes", class_breakpoint, enable_probes_command, _("\
Enable probes.\n\
Usage: enable probes [PROVIDER [NAME [OBJECT]]]\n\
Each argument is a regular expression, used to select probes.\n\
PROVIDER matches probe provider names.\n\
NAME matches the probe names.\n\
OBJECT matches the executable or shared library name.\n\
If you do not specify any argument then the command will enable\n\
all defined probes."),
	   &enablelist);

  add_cmd ("probes", class_breakpoint, disable_probes_command, _("\
Disable probes.\n\
Usage: disable probes [PROVIDER [NAME [OBJECT]]]\n\
Each argument is a regular expression, used to select probes.\n\
PROVIDER matches probe provider names.\n\
NAME matches the probe names.\n\
OBJECT matches the executable or shared library name.\n\
If you do not specify any argument then the command will disable\n\
all defined probes."),
	   &disablelist);

  add_cmd ("ignore-probes", class_maintenance, ignore_probes_command, _("\
Ignore probes.\n\
Usage: maintenance ignore-probes [-v|-verbose] [PROVIDER [NAME [OBJECT]]]\n\
       maintenance ignore-probes -reset\n\
Each argument is a regular expression, used to select probes.\n\
PROVIDER matches probe provider names.\n\
NAME matches the probe names.\n\
OBJECT matches the executable or shared library name.\n\
If you do not specify any argument then the command will ignore\n\
all defined probes.  To reset the ignore-probes filter, use the -reset form.\n\
Only supported for SystemTap probes."),
	   &maintenancelist);
}
