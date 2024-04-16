/* Portable <curses.h>.

   Copyright (C) 2004-2024 Free Software Foundation, Inc.

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

#ifndef GDB_CURSES_H
#define GDB_CURSES_H 1

#ifdef __MINGW32__
/* Windows API headers, included e.g. by serial.h, define MOUSE_MOVED,
   and so does PDCurses's curses.h, but for an entirely different
   purpose.  Since we don't use the Windows semantics of MOUSE_MOVED
   anywhere, avoid compiler warnings by undefining MOUSE_MOVED before
   including curses.h.  */
#undef MOUSE_MOVED
/* Likewise, KEY_EVENT is defined by ncurses.h, but also by Windows
   API headers.  */
#undef KEY_EVENT
#endif

/* On Solaris and probably other SysVr4 derived systems, we need to define
   NOMACROS so the native <curses.h> doesn't define clear which interferes
   with the clear member of class string_file.  ncurses potentially has a
   similar problem and fix.  */
#define NOMACROS
#define NCURSES_NOMACROS

#if defined (HAVE_NCURSESW_NCURSES_H)
#include <ncursesw/ncurses.h>
#elif defined (HAVE_NCURSES_NCURSES_H)
#include <ncurses/ncurses.h>
#elif defined (HAVE_NCURSES_H)
#include <ncurses.h>
#elif defined (HAVE_CURSESX_H)
#include <cursesX.h>
#elif defined (HAVE_CURSES_H)
#include <curses.h>
#endif

#if defined (HAVE_NCURSES_TERM_H)
#include <ncurses/term.h>
#elif defined (HAVE_TERM_H)
#include <term.h>
#else
/* On MinGW, a real termcap library is usually not present.  Stub versions
   of the termcap functions will be built from stub-termcap.c.  Readline
   provides its own extern declarations when there's no termcap.h; do the
   same here for the termcap functions used in GDB.  */
extern "C" int tgetnum (const char *);
#endif

/* SunOS's curses.h has a '#define reg register' in it.  Thank you Sun.  */
/* Ditto for:
   -bash-4.2$ uname -a
   AIX power-aix 1 7 00F84C0C4C00  */
#ifdef reg
#undef reg
#endif

#endif /* gdb_curses.h */
