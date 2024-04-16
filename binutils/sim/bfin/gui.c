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

/* This must come before any other includes.  */
#include "defs.h"

#ifdef HAVE_SDL
# include <SDL.h>
#endif
#ifdef HAVE_DLFCN_H
# include <dlfcn.h>
#endif

#include "libiberty.h"
#include "gui.h"

#ifdef HAVE_SDL

static struct {
  void *handle;
  int (*Init) (Uint32 flags);
  void (*Quit) (void);
  int (*ShowCursor) (int toggle);
  int (*LockSurface) (SDL_Surface *surface);
  void (*UnlockSurface) (SDL_Surface *surface);
#if HAVE_SDL == 1
  void (*GetRGB) (Uint32 pixel, const SDL_PixelFormat * const fmt, Uint8 *r, Uint8 *g, Uint8 *b);
  Uint32 (*MapRGB) (const SDL_PixelFormat * const format, const Uint8 r, const Uint8 g, const Uint8 b);
  SDL_Surface *(*SetVideoMode) (int width, int height, int bpp, Uint32 flags);
  void (*WM_SetCaption) (const char *title, const char *icon);
  void (*UpdateRect) (SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h);
#else
  void (*GetRGB) (Uint32 pixel, const SDL_PixelFormat *fmt, Uint8 *r, Uint8 *g, Uint8 *b);
  Uint32 (*MapRGB) (const SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b);
  SDL_Window *(*CreateWindow) (const char *title, int x, int y, int w, int h, Uint32 flags);
  SDL_Surface *(*GetWindowSurface) (SDL_Window *window);
  SDL_PixelFormat *(*AllocFormat) (Uint32 pixel_format);
  int (*UpdateWindowSurfaceRects) (SDL_Window *window, const SDL_Rect *rects, int numrects);
#endif
} sdl;

static const char * const sdl_syms[] =
{
  "SDL_Init",
  "SDL_Quit",
  "SDL_ShowCursor",
  "SDL_LockSurface",
  "SDL_UnlockSurface",
  "SDL_GetRGB",
  "SDL_MapRGB",
#if HAVE_SDL == 1
  "SDL_SetVideoMode",
  "SDL_WM_SetCaption",
  "SDL_UpdateRect",
#else
  "SDL_CreateWindow",
  "SDL_GetWindowSurface",
  "SDL_AllocFormat",
  "SDL_UpdateWindowSurfaceRects",
#endif
};

struct gui_state {
#if HAVE_SDL == 2
  SDL_Window *window;
#endif
  SDL_Surface *screen;
  const SDL_PixelFormat *format;
  int throttle, throttle_limit;
  enum gui_color color;
  int bytes_per_pixel;
  int curr_line;
};

static const char bfin_gui_window_title[] = "Blackfin GNU Simulator";

/* Load the SDL lib on the fly to avoid hard linking against it.  */
static int
bfin_gui_sdl_setup (void)
{
  static const char libsdl_soname[] =
#if HAVE_SDL == 1
      "libSDL-1.2.so.0";
#else
      "libSDL2-2.0.so.0";
#endif
  int i;
  uintptr_t **funcs;

  if (sdl.handle)
    return 0;

  sdl.handle = dlopen (libsdl_soname, RTLD_LAZY);
  if (sdl.handle == NULL)
    return -1;

  funcs = (void *) &sdl.Init;
  for (i = 0; i < ARRAY_SIZE (sdl_syms); ++i)
    {
      funcs[i] = dlsym (sdl.handle, sdl_syms[i]);
      if (funcs[i] == NULL)
	{
	  dlclose (sdl.handle);
	  sdl.handle = NULL;
	  return -1;
	}
    }

  return 0;
}

static const SDL_PixelFormat *bfin_gui_color_format (enum gui_color color,
						     int *bytes_per_pixel);

void *
bfin_gui_setup (void *state, int enabled, int width, int height,
		enum gui_color color)
{
  if (bfin_gui_sdl_setup ())
    return NULL;

  /* Create an SDL window if enabled and we don't have one yet.  */
  if (enabled && !state)
    {
      struct gui_state *gui = xmalloc (sizeof (*gui));
      if (!gui)
	return NULL;

      if (sdl.Init (SDL_INIT_VIDEO))
	goto error;

      gui->color = color;
      gui->format = bfin_gui_color_format (gui->color, &gui->bytes_per_pixel);
#if HAVE_SDL == 1
      sdl.WM_SetCaption (bfin_gui_window_title, NULL);
      gui->screen = sdl.SetVideoMode (width, height, 32,
				      SDL_ANYFORMAT|SDL_HWSURFACE);
#else
      gui->window = sdl.CreateWindow (
	  bfin_gui_window_title, SDL_WINDOWPOS_CENTERED,
	  SDL_WINDOWPOS_CENTERED, width, height, 0);
      if (!gui->window)
	{
	  sdl.Quit();
	  goto error;
	}

      gui->screen = sdl.GetWindowSurface(gui->window);
#endif
      if (!gui->screen)
	{
	  sdl.Quit();
	  goto error;
	}

      sdl.ShowCursor (0);
      gui->curr_line = 0;
      gui->throttle = 0;
      gui->throttle_limit = 0xf; /* XXX: let people control this ?  */
      return gui;

 error:
      free (gui);
      return NULL;
    }

  /* Else break down a window if disabled and we had one.  */
  else if (!enabled && state)
    {
      sdl.Quit();
      free (state);
      return NULL;
    }

  /* Retain existing state, whatever that may be.  */
  return state;
}

static int
SDL_ConvertBlitLineFrom (const Uint8 *src, const SDL_PixelFormat * const format,
#if HAVE_SDL == 2
			 SDL_Window *win,
#endif
			 SDL_Surface *dst, int dsty, int bytes_per_pixel)
{
  Uint8 r, g, b;
  Uint32 *pixels;
  unsigned i, j;

  if (SDL_MUSTLOCK (dst))
    if (sdl.LockSurface (dst))
      return 1;

  pixels = dst->pixels;
  pixels += (dsty * dst->pitch / 4);

  for (i = 0; i < dst->w; ++i)
    {
      /* Exract the packed source pixel; RGB or BGR.  */
      Uint32 pix = 0;
      for (j = 0; j < bytes_per_pixel; ++j)
	if (format->Rshift)
	  pix = (pix << 8) | src[j];
	else
	  pix = pix | ((Uint32)src[j] << (j * 8));

      /* Unpack the source pixel into its components.  */
      sdl.GetRGB (pix, format, &r, &g, &b);
      /* Translate into the screen pixel format.  */
      *pixels++ = sdl.MapRGB (dst->format, r, g, b);

      src += bytes_per_pixel;
    }

  if (SDL_MUSTLOCK (dst))
    sdl.UnlockSurface (dst);

#if HAVE_SDL == 1
  sdl.UpdateRect (dst, 0, dsty, dst->w, 1);
#else
  {
    SDL_Rect rect = {
      .x = 0,
      .y = dsty,
      .w = dst->w,
      .h = 1,
    };

    sdl.UpdateWindowSurfaceRects (win, &rect, 1);
  }
#endif

  return 0;
}

unsigned
bfin_gui_update (void *state, const void *source, unsigned nr_bytes)
{
  struct gui_state *gui = state;
  int ret;

  if (!gui)
    return 0;

  /* XXX: Make this an option ?  */
  gui->throttle = (gui->throttle + 1) & gui->throttle_limit;
  if (gui->throttle)
    return 0;

  ret = SDL_ConvertBlitLineFrom (source, gui->format,
#if HAVE_SDL == 2
				 gui->window,
#endif
				 gui->screen, gui->curr_line,
				 gui->bytes_per_pixel);
  if (ret)
    return 0;

  gui->curr_line = (gui->curr_line + 1) % gui->screen->h;

  return nr_bytes;
}

#if HAVE_SDL == 1

#define FMASK(cnt, shift) (((1 << (cnt)) - 1) << (shift))
#define _FORMAT(bpp, rcnt, gcnt, bcnt, acnt, rsh, gsh, bsh, ash) \
  NULL, bpp, (bpp)/8, 8-(rcnt), 8-(gcnt), 8-(bcnt), 8-(acnt), rsh, gsh, bsh, ash, \
  FMASK (rcnt, rsh), FMASK (gcnt, gsh), FMASK (bcnt, bsh), FMASK (acnt, ash),
#define FORMAT(rcnt, gcnt, bcnt, acnt, rsh, gsh, bsh, ash) \
  _FORMAT(((((rcnt) + (gcnt) + (bcnt) + (acnt)) + 7) / 8) * 8, \
	  rcnt, gcnt, bcnt, acnt, rsh, gsh, bsh, ash)

static const SDL_PixelFormat sdl_rgb_565 =
{
  FORMAT (5, 6, 5, 0, 11, 5, 0, 0)
};
#define SDL_PIXELFORMAT_RGB565 &sdl_rgb_565

static const SDL_PixelFormat sdl_bgr_565 =
{
  FORMAT (5, 6, 5, 0, 0, 5, 11, 0)
};
#define SDL_PIXELFORMAT_BGR565 &sdl_bgr_565

static const SDL_PixelFormat sdl_rgb_888 =
{
  FORMAT (8, 8, 8, 0, 16, 8, 0, 0)
};
#define SDL_PIXELFORMAT_RGB888 &sdl_rgb_888

static const SDL_PixelFormat sdl_bgr_888 =
{
  FORMAT (8, 8, 8, 0, 0, 8, 16, 0)
};
#define SDL_PIXELFORMAT_BGR888 &sdl_bgr_888

static const SDL_PixelFormat sdl_rgba_8888 =
{
  FORMAT (8, 8, 8, 8, 24, 16, 8, 0)
};
#define SDL_PIXELFORMAT_RGBA8888 &sdl_rgba_8888

#endif

static const struct {
  const char *name;
  /* Since we declare the pixel formats above for SDL 1, we have the bpp
     setting, but SDL 2 internally uses larger values for its own memory, so
     we can't assume the Blackfin pixel format sizes always match SDL.  */
  int bytes_per_pixel;
#if HAVE_SDL == 1
  const SDL_PixelFormat *format;
#else
  Uint32 format;
#endif
  enum gui_color color;
} color_spaces[] = {
  { "rgb565",   2, SDL_PIXELFORMAT_RGB565,   GUI_COLOR_RGB_565,   },
  { "bgr565",   2, SDL_PIXELFORMAT_BGR565,   GUI_COLOR_BGR_565,   },
  { "rgb888",   3, SDL_PIXELFORMAT_RGB888,   GUI_COLOR_RGB_888,   },
  { "bgr888",   3, SDL_PIXELFORMAT_BGR888,   GUI_COLOR_BGR_888,   },
  { "rgba8888", 4, SDL_PIXELFORMAT_RGBA8888, GUI_COLOR_RGBA_8888, },
};

enum gui_color bfin_gui_color (const char *color)
{
  int i;

  if (!color)
    goto def;

  for (i = 0; i < ARRAY_SIZE (color_spaces); ++i)
    if (!strcmp (color, color_spaces[i].name))
      return color_spaces[i].color;

  /* Pick a random default.  */
 def:
  return GUI_COLOR_RGB_888;
}

static const SDL_PixelFormat *bfin_gui_color_format (enum gui_color color,
						     int *bytes_per_pixel)
{
  int i;

  for (i = 0; i < ARRAY_SIZE (color_spaces); ++i)
    if (color == color_spaces[i].color)
      {
	*bytes_per_pixel = color_spaces[i].bytes_per_pixel;
#if HAVE_SDL == 1
	return color_spaces[i].format;
#else
	return sdl.AllocFormat (color_spaces[i].format);
#endif
      }

  return NULL;
}

int bfin_gui_color_depth (enum gui_color color)
{
  int bytes_per_pixel;
  const SDL_PixelFormat *format = bfin_gui_color_format (color,
							 &bytes_per_pixel);
  return format ? bytes_per_pixel * 8 : 0;
}

#endif
