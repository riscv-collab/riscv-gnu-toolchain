/* Linux namespaces(7) support.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

#ifndef NAT_LINUX_NAMESPACES_H
#define NAT_LINUX_NAMESPACES_H

/* Set to true to enable debugging of Linux namespaces code.  */

extern bool debug_linux_namespaces;

/* Enumeration of Linux namespace types.  */

enum linux_ns_type
  {
    /* IPC namespace: System V IPC, POSIX message queues.  */
    LINUX_NS_IPC,

    /* Mount namespace: mount points.  */
    LINUX_NS_MNT,

    /* Network namespace: network devices, stacks, ports, etc.  */
    LINUX_NS_NET,

    /* PID namespace: process IDs.  */
    LINUX_NS_PID,

    /* User namespace: user and group IDs.  */
    LINUX_NS_USER,

    /* UTS namespace: hostname and NIS domain name.  */
    LINUX_NS_UTS,

    /* Number of Linux namespaces.  */
    NUM_LINUX_NS_TYPES
  };

/* Return nonzero if process PID has the same TYPE namespace as the
   calling process, or if the kernel does not support TYPE namespaces
   (in which case there is only one TYPE namespace).  Return zero if
   the kernel supports TYPE namespaces and the two processes have
   different TYPE namespaces.  */

extern int linux_ns_same (pid_t pid, enum linux_ns_type type);

/* Like gdb_open_cloexec, but in the mount namespace of process
   PID.  */

extern int linux_mntns_open_cloexec (pid_t pid, const char *filename,
				     int flags, mode_t mode);

/* Like unlink(2), but in the mount namespace of process PID.  */

extern int linux_mntns_unlink (pid_t pid, const char *filename);

/* Like readlink(2), but in the mount namespace of process PID.  */

extern ssize_t linux_mntns_readlink (pid_t pid, const char *filename,
				     char *buf, size_t bufsiz);

#endif /* NAT_LINUX_NAMESPACES_H */
