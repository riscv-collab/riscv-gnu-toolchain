/* Remote File-I/O communications

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

/* See the GDB User Guide for details of the GDB remote protocol.  */

#ifndef REMOTE_FILEIO_H
#define REMOTE_FILEIO_H

#include "gdbsupport/fileio.h"

struct cmd_list_element;
struct remote_target;

/* Unified interface to remote fileio, called in remote.c from
   remote_wait () and remote_async_wait ().  */
extern void remote_fileio_request (remote_target *remote,
				   char *buf, int ctrlc_pending_p);

/* Cleanup any remote fileio state.  */
extern void remote_fileio_reset (void);

/* Called from _initialize_remote ().  */
extern void initialize_remote_fileio (
  struct cmd_list_element **remote_set_cmdlist,
  struct cmd_list_element **remote_show_cmdlist);

/* Unpack a struct fio_stat.  */
extern void remote_fileio_to_host_stat (struct fio_stat *fst,
					struct stat *st);

#endif
