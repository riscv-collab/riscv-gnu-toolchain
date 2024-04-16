/* Remote utility routines for the remote server for GDB.
   Copyright (C) 1993-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_REMOTE_UTILS_H
#define GDBSERVER_REMOTE_UTILS_H

int gdb_connected (void);

#define STDIO_CONNECTION_NAME "stdio"
int remote_connection_is_stdio (void);

ptid_t read_ptid (const char *buf, const char **obuf);
char *write_ptid (char *buf, ptid_t ptid);

int putpkt (char *buf);
int putpkt_binary (char *buf, int len);
int putpkt_notif (char *buf);
int getpkt (char *buf);
void remote_prepare (const char *name);
void remote_open (const char *name);
void remote_close (void);
void write_ok (char *buf);
void write_enn (char *buf);
void initialize_async_io (void);
void enable_async_io (void);
void disable_async_io (void);
void check_remote_input_interrupt_request (void);
void prepare_resume_reply (char *buf, ptid_t ptid,
			   const target_waitstatus &status);

const char *decode_address_to_semicolon (CORE_ADDR *addrp, const char *start);
void decode_address (CORE_ADDR *addrp, const char *start, int len);

/* Given an input string FROM, decode MEM_ADDR_PTR, a memory address in hex
   form,  and LEN_PTR, a length argument in hex form, from the pattern
   "<MEM_ADDR_PTR>,<LEN_PTR><END_MARKER>", with END_MARKER being an end marker
   character.  */
const char *decode_m_packet_params (const char *from, CORE_ADDR *mem_addr_ptr,
				    unsigned int *len_ptr,
				    const char end_marker);
void decode_m_packet (const char *from, CORE_ADDR * mem_addr_ptr,
		      unsigned int *len_ptr);
void decode_M_packet (const char *from, CORE_ADDR * mem_addr_ptr,
		      unsigned int *len_ptr, unsigned char **to_p);
int decode_X_packet (char *from, int packet_len, CORE_ADDR * mem_addr_ptr,
		     unsigned int *len_ptr, unsigned char **to_p);
int decode_xfer_write (char *buf, int packet_len,
		       CORE_ADDR *offset, unsigned int *len,
		       unsigned char *data);
int decode_search_memory_packet (const char *buf, int packet_len,
				 CORE_ADDR *start_addrp,
				 CORE_ADDR *search_space_lenp,
				 gdb_byte *pattern,
				 unsigned int *pattern_lenp);

void clear_symbol_cache (struct sym_cache **symcache_p);
int look_up_one_symbol (const char *name, CORE_ADDR *addrp, int may_ask_gdb);

int relocate_instruction (CORE_ADDR *to, CORE_ADDR oldloc);

void monitor_output (const char *msg);

#endif /* GDBSERVER_REMOTE_UTILS_H */
