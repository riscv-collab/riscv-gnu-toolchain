/* Functions that provide the mechanism to parse a syscall XML file
   and get its values.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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
#include "gdbtypes.h"
#include "xml-support.h"
#include "xml-syscall.h"
#include "gdbarch.h"

/* For the struct syscall definition.  */
#include "target.h"

#include "filenames.h"

#ifndef HAVE_LIBEXPAT

/* Dummy functions to indicate that there's no support for fetching
   syscalls information.  */

static void
syscall_warn_user (void)
{
  static int have_warned = 0;
  if (!have_warned)
    {
      have_warned = 1;
      warning (_("Can not parse XML syscalls information; XML support was "
		 "disabled at compile time."));
    }
}

void
set_xml_syscall_file_name (struct gdbarch *gdbarch, const char *name)
{
  return;
}

void
get_syscall_by_number (struct gdbarch *gdbarch,
		       int syscall_number, struct syscall *s)
{
  syscall_warn_user ();
  s->number = syscall_number;
  s->name = NULL;
}

bool
get_syscalls_by_name (struct gdbarch *gdbarch, const char *syscall_name,
		      std::vector<int> *syscall_numbers)
{
  syscall_warn_user ();
  return false;
}

const char **
get_syscall_names (struct gdbarch *gdbarch)
{
  syscall_warn_user ();
  return NULL;
}

bool
get_syscalls_by_group (struct gdbarch *gdbarch, const char *group,
		       std::vector<int> *syscall_numbers)
{
  syscall_warn_user ();
  return false;
}

const char **
get_syscall_group_names (struct gdbarch *gdbarch)
{
  syscall_warn_user ();
  return NULL;
}

#else /* ! HAVE_LIBEXPAT */

/* Structure which describes a syscall.  */
struct syscall_desc
{
  syscall_desc (int number_, std::string name_, std::string alias_)
  : number (number_), name (name_), alias (alias_)
  {}

  /* The syscall number.  */

  int number;

  /* The syscall name.  */

  std::string name;

  /* An optional alias.  */

  std::string alias;
};

typedef std::unique_ptr<syscall_desc> syscall_desc_up;

/* Structure of a syscall group.  */
struct syscall_group_desc
{
  syscall_group_desc (const std::string &name_)
  : name (name_)
  {}

  /* The group name.  */

  std::string name;

  /* The syscalls that are part of the group.  This is a non-owning
     reference.  */

  std::vector<syscall_desc *> syscalls;
};

typedef std::unique_ptr<syscall_group_desc> syscall_group_desc_up;

/* Structure that represents syscalls information.  */
struct syscalls_info
{
  /* The syscalls.  */

  std::vector<syscall_desc_up> syscalls;

  /* The syscall groups.  */

  std::vector<syscall_group_desc_up> groups;

  /* Variable that will hold the last known data-directory.  This is
     useful to know whether we should re-read the XML info for the
     target.  */

  std::string my_gdb_datadir;
};

typedef std::unique_ptr<syscalls_info> syscalls_info_up;

/* Callback data for syscall information parsing.  */
struct syscall_parsing_data
{
  /* The syscalls_info we are building.  */

  struct syscalls_info *syscalls_info;
};

/* Create a new syscall group.  Return pointer to the
   syscall_group_desc structure that represents the new group.  */

static struct syscall_group_desc *
syscall_group_create_syscall_group_desc (struct syscalls_info *syscalls_info,
					 const char *group)
{
  syscall_group_desc *groupdesc = new syscall_group_desc (group);

  syscalls_info->groups.emplace_back (groupdesc);

  return groupdesc;
}

/* Add a syscall to the group.  If group doesn't exist, create it.  */

static void
syscall_group_add_syscall (struct syscalls_info *syscalls_info,
			   struct syscall_desc *syscall,
			   const char *group)
{
  /* Search for an existing group.  */
  std::vector<syscall_group_desc_up>::iterator it
    = syscalls_info->groups.begin ();

  for (; it != syscalls_info->groups.end (); it++)
    {
      if ((*it)->name == group)
	break;
    }

  syscall_group_desc *groupdesc;

  if (it != syscalls_info->groups.end ())
    groupdesc = it->get ();
  else
    {
      /* No group was found with this name.  We must create a new
	 one.  */
      groupdesc = syscall_group_create_syscall_group_desc (syscalls_info,
							   group);
    }

  groupdesc->syscalls.push_back (syscall);
}

static void
syscall_create_syscall_desc (struct syscalls_info *syscalls_info,
			     const char *name, int number, const char *alias,
			     char *groups)
{
  syscall_desc *sysdesc = new syscall_desc (number, name,
					    alias != NULL ? alias : "");

  syscalls_info->syscalls.emplace_back (sysdesc);

  /*  Add syscall to its groups.  */
  if (groups != NULL)
    {
      char *saveptr;
      for (char *group = strtok_r (groups, ",", &saveptr);
	   group != NULL;
	   group = strtok_r (NULL, ",", &saveptr))
	syscall_group_add_syscall (syscalls_info, sysdesc, group);
    }
}

/* Handle the start of a <syscall> element.  */
static void
syscall_start_syscall (struct gdb_xml_parser *parser,
		       const struct gdb_xml_element *element,
		       void *user_data,
		       std::vector<gdb_xml_value> &attributes)
{
  struct syscall_parsing_data *data = (struct syscall_parsing_data *) user_data;
  /* syscall info.  */
  char *name = NULL;
  int number = 0;
  char *alias = NULL;
  char *groups = NULL;

  for (const gdb_xml_value &attr : attributes)
    {
      if (strcmp (attr.name, "name") == 0)
	name = (char *) attr.value.get ();
      else if (strcmp (attr.name, "number") == 0)
	number = * (ULONGEST *) attr.value.get ();
      else if (strcmp (attr.name, "alias") == 0)
	alias = (char *) attr.value.get ();
      else if (strcmp (attr.name, "groups") == 0)
	groups = (char *) attr.value.get ();
      else
	internal_error (_("Unknown attribute name '%s'."), attr.name);
    }

  gdb_assert (name);
  syscall_create_syscall_desc (data->syscalls_info, name, number, alias,
			       groups);
}


/* The elements and attributes of an XML syscall document.  */
static const struct gdb_xml_attribute syscall_attr[] = {
  { "number", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { "alias", GDB_XML_AF_OPTIONAL, NULL, NULL },
  { "groups", GDB_XML_AF_OPTIONAL, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element syscalls_info_children[] = {
  { "syscall", syscall_attr, NULL,
    GDB_XML_EF_OPTIONAL | GDB_XML_EF_REPEATABLE,
    syscall_start_syscall, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_element syselements[] = {
  { "syscalls_info", NULL, syscalls_info_children,
    GDB_XML_EF_NONE, NULL, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static struct syscalls_info *
syscall_parse_xml (const char *document, xml_fetch_another fetcher)
{
  struct syscall_parsing_data data;
  syscalls_info_up sysinfo (new syscalls_info ());

  data.syscalls_info = sysinfo.get ();

  if (gdb_xml_parse_quick (_("syscalls info"), NULL,
			   syselements, document, &data) == 0)
    {
      /* Parsed successfully.  */
      return sysinfo.release ();
    }
  else
    {
      warning (_("Could not load XML syscalls info; ignoring"));
      return NULL;
    }
}

/* Function responsible for initializing the information
   about the syscalls.  It reads the XML file and fills the
   struct syscalls_info with the values.
   
   Returns the struct syscalls_info if the file is valid, NULL otherwise.  */
static struct syscalls_info *
xml_init_syscalls_info (const char *filename)
{
  std::optional<gdb::char_vector> full_file
    = xml_fetch_content_from_file (filename,
				   const_cast<char *>(gdb_datadir.c_str ()));
  if (!full_file)
    return NULL;

  const std::string dirname = ldirname (filename);
  auto fetch_another = [&dirname] (const char *name)
    {
      return xml_fetch_content_from_file (name, dirname.c_str ());
    };

  return syscall_parse_xml (full_file->data (), fetch_another);
}

/* Initializes the syscalls_info structure according to the
   architecture.  */
static void
init_syscalls_info (struct gdbarch *gdbarch)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);
  const char *xml_syscall_file = gdbarch_xml_syscall_file (gdbarch);

  /* Should we re-read the XML info for this target?  */
  if (syscalls_info != NULL && !syscalls_info->my_gdb_datadir.empty ()
      && filename_cmp (syscalls_info->my_gdb_datadir.c_str (),
		       gdb_datadir.c_str ()) != 0)
    {
      /* The data-directory changed from the last time we used it.
	 It means that we have to re-read the XML info.  */
      delete syscalls_info;
      syscalls_info = NULL;
      set_gdbarch_syscalls_info (gdbarch, NULL);
    }

  /* Did we succeed at initializing this?  */
  if (syscalls_info != NULL)
    return;

  syscalls_info = xml_init_syscalls_info (xml_syscall_file);

  /* If there was some error reading the XML file, we initialize
     gdbarch->syscalls_info anyway, in order to store information
     about our attempt.  */
  if (syscalls_info == NULL)
    syscalls_info = new struct syscalls_info ();

  if (syscalls_info->syscalls.empty ())
    {
      if (xml_syscall_file != NULL)
	warning (_("Could not load the syscall XML file `%s/%s'."),
		 gdb_datadir.c_str (), xml_syscall_file);
      else
	warning (_("There is no XML file to open."));

      warning (_("GDB will not be able to display "
		 "syscall names nor to verify if\n"
		 "any provided syscall numbers are valid."));
    }

  /* Saving the data-directory used to read this XML info.  */
  syscalls_info->my_gdb_datadir.assign (gdb_datadir);

  set_gdbarch_syscalls_info (gdbarch, syscalls_info);
}

/* Search for a syscall group by its name.  Return syscall_group_desc
   structure for the group if found or NULL otherwise.  */

static struct syscall_group_desc *
syscall_group_get_group_by_name (const struct syscalls_info *syscalls_info,
				 const char *group)
{
  if (syscalls_info == NULL)
    return NULL;

  if (group == NULL)
    return NULL;

  /* Search for existing group.  */
  for (const syscall_group_desc_up &groupdesc : syscalls_info->groups)
    {
      if (groupdesc->name == group)
	return groupdesc.get ();
    }

  return NULL;
}

static bool
xml_get_syscalls_by_name (struct gdbarch *gdbarch, const char *syscall_name,
			  std::vector<int> *syscall_numbers)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);

  bool found = false;
  if (syscalls_info != NULL && syscall_name != NULL && syscall_numbers != NULL)
    for (const syscall_desc_up &sysdesc : syscalls_info->syscalls)
      if (sysdesc->name == syscall_name || sysdesc->alias == syscall_name)
	{
	  syscall_numbers->push_back (sysdesc->number);
	  found = true;
	}

  return found;
}

static const char *
xml_get_syscall_name (struct gdbarch *gdbarch,
		      int syscall_number)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);

  if (syscalls_info == NULL
      || syscall_number < 0)
    return NULL;

  for (const syscall_desc_up &sysdesc : syscalls_info->syscalls)
    if (sysdesc->number == syscall_number)
      return sysdesc->name.c_str ();

  return NULL;
}

static const char **
xml_list_of_syscalls (struct gdbarch *gdbarch)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);

  if (syscalls_info == NULL)
    return NULL;

  int nsyscalls = syscalls_info->syscalls.size ();
  const char **names = XNEWVEC (const char *, nsyscalls + 1);

  int i;
  for (i = 0; i < syscalls_info->syscalls.size (); i++)
    names[i] = syscalls_info->syscalls[i]->name.c_str ();

  names[i] = NULL;

  return names;
}

/* Iterate over the syscall_group_desc element to return a list of
   syscalls that are part of the given group.  If the syscall group
   doesn't exist, return false.  */

static bool
xml_list_syscalls_by_group (struct gdbarch *gdbarch, const char *group,
			    std::vector<int> *syscalls)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);
  struct syscall_group_desc *groupdesc;

  if (syscalls_info == NULL || syscalls == NULL)
    return false;

  groupdesc = syscall_group_get_group_by_name (syscalls_info, group);
  if (groupdesc == NULL)
    return false;

  for (const syscall_desc *sysdesc : groupdesc->syscalls)
    syscalls->push_back (sysdesc->number);

  return true;
}

/* Return a NULL terminated list of syscall groups or an empty list, if
   no syscall group is available.  Return NULL, if there is no syscall
   information available.  */

static const char **
xml_list_of_groups (struct gdbarch *gdbarch)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);
  const char **names = NULL;
  int ngroups;
  int i;

  if (syscalls_info == NULL)
    return NULL;

  ngroups = syscalls_info->groups.size ();
  names = (const char**) xmalloc ((ngroups + 1) * sizeof (char *));

  for (i = 0; i < syscalls_info->groups.size (); i++)
    names[i] = syscalls_info->groups[i]->name.c_str ();

  names[i] = NULL;

  return names;
}

void
set_xml_syscall_file_name (struct gdbarch *gdbarch, const char *name)
{
  set_gdbarch_xml_syscall_file (gdbarch, name);
}

void
get_syscall_by_number (struct gdbarch *gdbarch,
		       int syscall_number, struct syscall *s)
{
  init_syscalls_info (gdbarch);

  s->number = syscall_number;
  s->name = xml_get_syscall_name (gdbarch, syscall_number);
}

bool
get_syscalls_by_name (struct gdbarch *gdbarch, const char *syscall_name,
		      std::vector<int> *syscall_numbers)
{
  init_syscalls_info (gdbarch);

  return xml_get_syscalls_by_name (gdbarch, syscall_name, syscall_numbers);
}

const char **
get_syscall_names (struct gdbarch *gdbarch)
{
  init_syscalls_info (gdbarch);

  return xml_list_of_syscalls (gdbarch);
}

/* See comment in xml-syscall.h.  */

bool
get_syscalls_by_group (struct gdbarch *gdbarch, const char *group,
		       std::vector<int> *syscall_numbers)
{
  init_syscalls_info (gdbarch);

  return xml_list_syscalls_by_group (gdbarch, group, syscall_numbers);
}

/* See comment in xml-syscall.h.  */

const char **
get_syscall_group_names (struct gdbarch *gdbarch)
{
  init_syscalls_info (gdbarch);

  return xml_list_of_groups (gdbarch);
}

#endif /* ! HAVE_LIBEXPAT */
