/* Blackfin GUI (SDL) helper code

   Copyright (C) 2010-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

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

#ifndef BFIN_GUI_H
#define BFIN_GUI_H

/* These macros are ugly.  */
#ifdef HAVE_SDL
# define _BFIN_GUI_INLINE
# define _BFIN_GUI_STUB(...) ;
#else
# define _BFIN_GUI_INLINE ATTRIBUTE_UNUSED static inline
# define _BFIN_GUI_STUB(val) { return val; }
#endif

enum gui_color {
  GUI_COLOR_RGB_565,
  GUI_COLOR_BGR_565,
  GUI_COLOR_RGB_888,
  GUI_COLOR_BGR_888,
  GUI_COLOR_RGBA_8888,
};
_BFIN_GUI_INLINE
enum gui_color bfin_gui_color (const char *color)
  _BFIN_GUI_STUB(GUI_COLOR_RGB_565)

_BFIN_GUI_INLINE
int bfin_gui_color_depth (enum gui_color color)
  _BFIN_GUI_STUB(0)

_BFIN_GUI_INLINE
void *bfin_gui_setup (void *state, int enabled, int height, int width,
		      enum gui_color color)
  _BFIN_GUI_STUB(NULL)

_BFIN_GUI_INLINE
unsigned bfin_gui_update (void *state, const void *source, unsigned nr_bytes)
  _BFIN_GUI_STUB(0)

#undef _BFIN_GUI_INLINE
#undef _BFIN_GUI_STUB

#endif
