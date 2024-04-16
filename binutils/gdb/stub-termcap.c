/* A very minimal do-nothing termcap emulation stub.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.

   Contributed by CodeSourcery, LLC.

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

extern "C" {

/* -Wmissing-prototypes */
extern int tgetent (char *buffer, char *termtype);
extern int tgetnum (char *name);
extern int tgetflag (char *name);
extern char* tgetstr (char *name, char **area);
extern int tputs (char *string, int nlines, int (*outfun) (int));
extern char *tgoto (const char *cap, int col, int row);

}

/* These globals below are global termcap variables that readline
   references.

   Actually, depending on preprocessor conditions that we don't want
   to mirror here (as they may change depending on readline versions),
   readline may define these globals as well, relying on the linker
   merging them if needed (-fcommon).  That doesn't work with
   -fno-common or C++, so instead we define the symbols as weak.
   Don't do this on Windows though, as MinGW gcc 3.4.2 doesn't support
   weak (later versions, e.g., 4.8, do support it).  Given this stub
   file originally was Windows only, and we only needed this when we
   made it work on other hosts, it should be OK.  */
#ifndef __MINGW32__
char PC __attribute__((weak));
char *BC __attribute__((weak));
char *UP __attribute__((weak));
#endif

/* Each of the files below is a minimal implementation of the standard
   termcap function with the same name, suitable for use in a Windows
   console window, or when a real termcap/curses library isn't
   available.  */

int
tgetent (char *buffer, char *termtype)
{
  return -1;
}

int
tgetnum (char *name)
{
  return -1;
}

int
tgetflag (char *name)
{
  return -1;
}

char *
tgetstr (char *name, char **area)
{
  return NULL;
}

int
tputs (char *string, int nlines, int (*outfun) (int))
{
  while (*string)
    outfun (*string++);

  return 0;
}

char *
tgoto (const char *cap, int col, int row)
{
  return NULL;
}
