/* Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef NAT_GDB_THREAD_DB_H
#define NAT_GDB_THREAD_DB_H

#ifdef HAVE_THREAD_DB_H
#include <thread_db.h>
#else
#include "glibc_thread_db.h"
#endif

#ifndef LIBTHREAD_DB_SO
#define LIBTHREAD_DB_SO "libthread_db.so.1"
#endif

#ifndef LIBTHREAD_DB_SEARCH_PATH
/* $sdir appears before $pdir for some minimal security protection:
   we trust the system libthread_db.so a bit more than some random
   libthread_db associated with whatever libpthread the app is using.  */
#define LIBTHREAD_DB_SEARCH_PATH "$sdir:$pdir"
#endif

/* Types of the libthread_db functions.  */

typedef td_err_e (td_init_ftype) (void);

typedef td_err_e (td_ta_new_ftype) (struct ps_prochandle * ps,
				    td_thragent_t **ta);
typedef td_err_e (td_ta_delete_ftype) (td_thragent_t *ta_p);
typedef td_err_e (td_ta_map_lwp2thr_ftype) (const td_thragent_t *ta,
					    lwpid_t lwpid, td_thrhandle_t *th);
typedef td_err_e (td_ta_thr_iter_ftype) (const td_thragent_t *ta,
					 td_thr_iter_f *callback, void *cbdata_p,
					 td_thr_state_e state, int ti_pri,
					 sigset_t *ti_sigmask_p,
					 unsigned int ti_user_flags);
typedef td_err_e (td_ta_event_addr_ftype) (const td_thragent_t *ta,
					   td_event_e event, td_notify_t *ptr);
typedef td_err_e (td_ta_set_event_ftype) (const td_thragent_t *ta,
					  td_thr_events_t *event);
typedef td_err_e (td_ta_clear_event_ftype) (const td_thragent_t *ta,
					    td_thr_events_t *event);
typedef td_err_e (td_ta_event_getmsg_ftype) (const td_thragent_t *ta,
					     td_event_msg_t *msg);

typedef td_err_e (td_thr_get_info_ftype) (const td_thrhandle_t *th,
					  td_thrinfo_t *infop);
typedef td_err_e (td_thr_event_enable_ftype) (const td_thrhandle_t *th,
					      int event);

typedef td_err_e (td_thr_tls_get_addr_ftype) (const td_thrhandle_t *th,
					      psaddr_t map_address,
					      size_t offset, psaddr_t *address);
typedef td_err_e (td_thr_tlsbase_ftype) (const td_thrhandle_t *th,
					 unsigned long int modid,
					 psaddr_t *base);

typedef const char ** (td_symbol_list_ftype) (void);
typedef td_err_e (td_ta_delete_ftype) (td_thragent_t *);

#endif /* NAT_GDB_THREAD_DB_H */
