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

#ifndef XML_SYSCALL_H
#define XML_SYSCALL_H 1

/* Function used to set the name of the file which contains
   information about the system calls present in the current
   architecture.
   
   This function *should* be called before anything else, otherwise
   GDB won't be able to find the correct XML file to open and get
   the syscalls definitions.  */

void set_xml_syscall_file_name (struct gdbarch *gdbarch,
				const char *name);

/* Function that retrieves the syscall name corresponding to the given
   number.  It puts the requested information inside 'struct syscall'.  */

void get_syscall_by_number (struct gdbarch *gdbarch,
			    int syscall_number, struct syscall *s);

/* Function that retrieves the syscall numbers corresponding to the
   given name.  The numbers of all syscalls with either a name or
   alias equal to SYSCALL_NAME are appended to SYSCALL_NUMBERS.  If no
   matching syscalls are found, return false.  */

bool get_syscalls_by_name (struct gdbarch *gdbarch, const char *syscall_name,
			   std::vector<int> *syscall_numbers);

/* Function used to retrieve the list of syscalls in the system.  This list
   is returned as an array of strings.  Returns the list of syscalls in the
   system, or NULL otherwise.  */

const char **get_syscall_names (struct gdbarch *gdbarch);

/* Function used to retrieve the list of syscalls of a given group in
   the system.  The syscall numbers are appended to SYSCALL_NUMBERS.
   If the group doesn't exist, return false.  */

bool get_syscalls_by_group (struct gdbarch *gdbarch, const char *group,
			    std::vector<int> *syscall_numbers);

/* Function used to retrieve the list of syscall groups in the system.
   Return an array of strings terminated by a NULL element.  The list
   must be freed by the caller.  Return NULL if there is no syscall
   information available.  */

const char **get_syscall_group_names (struct gdbarch *gdbarch);

#endif /* XML_SYSCALL_H */
