/* Path manipulation routines for GDB and gdbserver.

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

#include "common-defs.h"
#include "pathstuff.h"
#include "host-defs.h"
#include "filenames.h"
#include "gdb_tilde_expand.h"

#ifdef USE_WIN32API
#include <windows.h>
#endif

/* See gdbsupport/pathstuff.h.  */

char *current_directory;

/* See gdbsupport/pathstuff.h.  */

gdb::unique_xmalloc_ptr<char>
gdb_realpath (const char *filename)
{
/* On most hosts, we rely on canonicalize_file_name to compute
   the FILENAME's realpath.

   But the situation is slightly more complex on Windows, due to some
   versions of GCC which were reported to generate paths where
   backslashes (the directory separator) were doubled.  For instance:
      c:\\some\\double\\slashes\\dir
   ... instead of ...
      c:\some\double\slashes\dir
   Those double-slashes were getting in the way when comparing paths,
   for instance when trying to insert a breakpoint as follow:
      (gdb) b c:/some/double/slashes/dir/foo.c:4
      No source file named c:/some/double/slashes/dir/foo.c:4.
      (gdb) b c:\some\double\slashes\dir\foo.c:4
      No source file named c:\some\double\slashes\dir\foo.c:4.
   To prevent this from happening, we need this function to always
   strip those extra backslashes.  While canonicalize_file_name does
   perform this simplification, it only works when the path is valid.
   Since the simplification would be useful even if the path is not
   valid (one can always set a breakpoint on a file, even if the file
   does not exist locally), we rely instead on GetFullPathName to
   perform the canonicalization.  */

#if defined (_WIN32)
  {
    char buf[MAX_PATH];
    DWORD len = GetFullPathName (filename, MAX_PATH, buf, NULL);

    /* The file system is case-insensitive but case-preserving.
       So it is important we do not lowercase the path.  Otherwise,
       we might not be able to display the original casing in a given
       path.  */
    if (len > 0 && len < MAX_PATH)
      return make_unique_xstrdup (buf);
  }
#else
  {
    char *rp = canonicalize_file_name (filename);

    if (rp != NULL)
      return gdb::unique_xmalloc_ptr<char> (rp);
  }
#endif

  /* This system is a lost cause, just dup the buffer.  */
  return make_unique_xstrdup (filename);
}

/* See gdbsupport/pathstuff.h.  */

std::string
gdb_realpath_keepfile (const char *filename)
{
  const char *base_name = lbasename (filename);
  char *dir_name;

  /* Extract the basename of filename, and return immediately
     a copy of filename if it does not contain any directory prefix.  */
  if (base_name == filename)
    return filename;

  dir_name = (char *) alloca ((size_t) (base_name - filename + 2));
  /* Allocate enough space to store the dir_name + plus one extra
     character sometimes needed under Windows (see below), and
     then the closing \000 character.  */
  strncpy (dir_name, filename, base_name - filename);
  dir_name[base_name - filename] = '\000';

#ifdef HAVE_DOS_BASED_FILE_SYSTEM
  /* We need to be careful when filename is of the form 'd:foo', which
     is equivalent of d:./foo, which is totally different from d:/foo.  */
  if (strlen (dir_name) == 2 && isalpha (dir_name[0]) && dir_name[1] == ':')
    {
      dir_name[2] = '.';
      dir_name[3] = '\000';
    }
#endif

  /* Canonicalize the directory prefix, and build the resulting
     filename.  If the dirname realpath already contains an ending
     directory separator, avoid doubling it.  */
  gdb::unique_xmalloc_ptr<char> path_storage = gdb_realpath (dir_name);
  const char *real_path = path_storage.get ();
  return path_join (real_path, base_name);
}

/* See gdbsupport/pathstuff.h.  */

std::string
gdb_abspath (const char *path)
{
  gdb_assert (path != NULL && path[0] != '\0');

  if (path[0] == '~')
    return gdb_tilde_expand (path);

  if (IS_ABSOLUTE_PATH (path) || current_directory == NULL)
    return path;

  return path_join (current_directory, path);
}

/* See gdbsupport/pathstuff.h.  */

const char *
child_path (const char *parent, const char *child)
{
  /* The child path must start with the parent path.  */
  size_t parent_len = strlen (parent);
  if (filename_ncmp (parent, child, parent_len) != 0)
    return NULL;

  /* The parent path must be a directory and the child must contain at
     least one component underneath the parent.  */
  const char *child_component;
  if (parent_len > 0 && IS_DIR_SEPARATOR (parent[parent_len - 1]))
    {
      /* The parent path ends in a directory separator, so it is a
	 directory.  The first child component starts after the common
	 prefix.  */
      child_component = child + parent_len;
    }
  else
    {
      /* The parent path does not end in a directory separator.  The
	 first character in the child after the common prefix must be
	 a directory separator.

	 Note that CHILD must hold at least parent_len characters for
	 filename_ncmp to return zero.  If the character at parent_len
	 is nul due to CHILD containing the same path as PARENT, the
	 IS_DIR_SEPARATOR check will fail here.  */
      if (!IS_DIR_SEPARATOR (child[parent_len]))
	return NULL;

      /* The first child component starts after the separator after the
	 common prefix.  */
      child_component = child + parent_len + 1;
    }

  /* The child must contain at least one non-separator character after
     the parent.  */
  while (*child_component != '\0')
    {
      if (!IS_DIR_SEPARATOR (*child_component))
	return child_component;

      child_component++;
    }
  return NULL;
}

/* See gdbsupport/pathstuff.h.  */

std::string
path_join (gdb::array_view<const char *> paths)
{
  std::string ret;

  for (int i = 0; i < paths.size (); ++i)
    {
      const char *path = paths[i];

      if (i > 0)
	gdb_assert (strlen (path) == 0 || !IS_ABSOLUTE_PATH (path));

      if (!ret.empty () && !IS_DIR_SEPARATOR (ret.back ()))
	  ret += '/';

      ret.append (path);
    }

  return ret;
}

/* See gdbsupport/pathstuff.h.  */

bool
contains_dir_separator (const char *path)
{
  for (; *path != '\0'; path++)
    {
      if (IS_DIR_SEPARATOR (*path))
	return true;
    }

  return false;
}

/* See gdbsupport/pathstuff.h.  */

std::string
get_standard_cache_dir ()
{
#ifdef __APPLE__
#define HOME_CACHE_DIR "Library/Caches"
#else
#define HOME_CACHE_DIR ".cache"
#endif

#ifndef __APPLE__
  const char *xdg_cache_home = getenv ("XDG_CACHE_HOME");
  if (xdg_cache_home != NULL && xdg_cache_home[0] != '\0')
    {
      /* Make sure the path is absolute and tilde-expanded.  */
      std::string abs = gdb_abspath (xdg_cache_home);
      return path_join (abs.c_str (), "gdb");
    }
#endif

  const char *home = getenv ("HOME");
  if (home != NULL && home[0] != '\0')
    {
      /* Make sure the path is absolute and tilde-expanded.  */
      std::string abs = gdb_abspath (home);
      return path_join (abs.c_str (), HOME_CACHE_DIR,  "gdb");
    }

#ifdef WIN32
  const char *win_home = getenv ("LOCALAPPDATA");
  if (win_home != NULL && win_home[0] != '\0')
    {
      /* Make sure the path is absolute and tilde-expanded.  */
      std::string abs = gdb_abspath (win_home);
      return path_join (abs.c_str (), "gdb");
    }
#endif

  return {};
}

/* See gdbsupport/pathstuff.h.  */

std::string
get_standard_temp_dir ()
{
#ifdef WIN32
  const char *tmp = getenv ("TMP");
  if (tmp != nullptr)
    return tmp;

  tmp = getenv ("TEMP");
  if (tmp != nullptr)
    return tmp;

  error (_("Couldn't find temp dir path, both TMP and TEMP are unset."));

#else
  const char *tmp = getenv ("TMPDIR");
  if (tmp != nullptr)
    return tmp;

  return "/tmp";
#endif
}

/* See pathstuff.h.  */

std::string
get_standard_config_dir ()
{
#ifdef __APPLE__
#define HOME_CONFIG_DIR "Library/Preferences"
#else
#define HOME_CONFIG_DIR ".config"
#endif

#ifndef __APPLE__
  const char *xdg_config_home = getenv ("XDG_CONFIG_HOME");
  if (xdg_config_home != NULL && xdg_config_home[0] != '\0')
    {
      /* Make sure the path is absolute and tilde-expanded.  */
      std::string abs = gdb_abspath (xdg_config_home);
      return path_join (abs.c_str (), "gdb");
    }
#endif

  const char *home = getenv ("HOME");
  if (home != NULL && home[0] != '\0')
    {
      /* Make sure the path is absolute and tilde-expanded.  */
      std::string abs = gdb_abspath (home);
      return path_join (abs.c_str (), HOME_CONFIG_DIR, "gdb");
    }

  return {};
}

/* See pathstuff.h. */

std::string
get_standard_config_filename (const char *filename)
{
  std::string config_dir = get_standard_config_dir ();
  if (config_dir != "")
    {
      const char *tmp = (*filename == '.') ? (filename + 1) : filename;
      std::string path = config_dir + SLASH_STRING + std::string (tmp);
      return path;
    }

  return {};
}

/* See pathstuff.h.  */

std::string
find_gdb_home_config_file (const char *name, struct stat *buf)
{
  gdb_assert (name != nullptr);
  gdb_assert (*name != '\0');

  std::string config_dir_file = get_standard_config_filename (name);
  if (!config_dir_file.empty ())
    {
      if (stat (config_dir_file.c_str (), buf) == 0)
	return config_dir_file;
    }

  const char *homedir = getenv ("HOME");
  if (homedir != nullptr && homedir[0] != '\0')
    {
      /* Make sure the path is absolute and tilde-expanded.  */
      std::string abs = gdb_abspath (homedir);
      std::string path = string_printf ("%s/%s", abs.c_str (), name);
      if (stat (path.c_str (), buf) == 0)
	return path;
    }

  return {};
}

/* See gdbsupport/pathstuff.h.  */

const char *
get_shell ()
{
  const char *ret = getenv ("SHELL");
  if (ret == NULL)
    ret = "/bin/sh";

  return ret;
}

/* See gdbsupport/pathstuff.h.  */

gdb::char_vector
make_temp_filename (const std::string &f)
{
  gdb::char_vector filename_temp (f.length () + 8);
  strcpy (filename_temp.data (), f.c_str ());
  strcat (filename_temp.data () + f.size (), "-XXXXXX");
  return filename_temp;
}
