
/*
 * Ovlymgr.c -- Runtime Overlay Manager for the GDB testsuite.
 */

#include "ovlymgr.h"
#include <string.h>
#include <stdlib.h>

/* Local functions and data: */

extern unsigned long _ovly_table[][4];
extern unsigned long _novlys __attribute__ ((section (".data")));
enum ovly_index { VMA, SIZE, LMA, MAPPED};

static void ovly_copy (unsigned long dst, unsigned long src, long size);

/* Flush the data and instruction caches at address START for SIZE bytes.
   Support for each new port must be added here.  */
/* FIXME: Might be better to have a standard libgloss function that
   ports provide that we can then use.  Use libgloss instead of newlib
   since libgloss is the one intended to handle low level system issues.
   I would suggest something like _flush_cache to avoid the user's namespace
   but not be completely obscure as other things may need this facility.  */
 
static void
FlushCache (void)
{
#ifdef __M32R__
  volatile char *mspr = (char *) 0xfffffff7;
  *mspr = 1;
#endif
}

/* _ovly_debug_event:
 * Debuggers may set a breakpoint here, to be notified 
 * when the overlay table has been modified.
 */
static void
_ovly_debug_event (void)
{
}

/* OverlayLoad:
 * Copy the overlay into its runtime region,
 * and mark the overlay as "mapped".
 */

bool
OverlayLoad (unsigned long ovlyno)
{
  unsigned long i;

  if (ovlyno < 0 || ovlyno >= _novlys)
    exit (-1);	/* fail, bad ovly number */

  if (_ovly_table[ovlyno][MAPPED])
    return TRUE;	/* this overlay already mapped -- nothing to do! */

  for (i = 0; i < _novlys; i++)
    if (i == ovlyno)
      _ovly_table[i][MAPPED] = 1;	/* this one now mapped */
    else if (_ovly_table[i][VMA] == _ovly_table[ovlyno][VMA])
      _ovly_table[i][MAPPED] = 0;	/* this one now un-mapped */

  ovly_copy (_ovly_table[ovlyno][VMA], 
	     _ovly_table[ovlyno][LMA], 
	     _ovly_table[ovlyno][SIZE]);

  FlushCache ();
  _ovly_debug_event ();
  return TRUE;
}

/* OverlayUnload:
 * Copy the overlay back into its "load" region.
 * Does NOT mark overlay as "unmapped", therefore may be called
 * more than once for the same mapped overlay.
 */
 
bool
OverlayUnload (unsigned long ovlyno)
{
  if (ovlyno < 0 || ovlyno >= _novlys)
    exit (-1);  /* fail, bad ovly number */
 
  if (!_ovly_table[ovlyno][MAPPED])
    exit (-1);  /* error, can't copy out a segment that's not "in" */
 
  ovly_copy (_ovly_table[ovlyno][LMA], 
	     _ovly_table[ovlyno][VMA],
	     _ovly_table[ovlyno][SIZE]);

  _ovly_debug_event ();
  return TRUE;
}

static void
ovly_copy (unsigned long dst, unsigned long src, long size)
{
  memcpy ((void *) dst, (void *) src, size);
}
