/* If we're being compiled as a .c file, rather than being included in
   d10v-sim.h, then ENDIAN_INLINE won't be defined yet.  */

/* This must come before any other includes.  */
#include "defs.h"

#ifndef ENDIAN_INLINE
#define NO_ENDIAN_INLINE
#include "sim-main.h"
#define ENDIAN_INLINE
#endif

#include "d10v-sim.h"

ENDIAN_INLINE uint16_t
get_word (const uint8_t *x)
{
  return ((uint16_t)x[0]<<8) + x[1];
}

ENDIAN_INLINE uint32_t
get_longword (const uint8_t *x)
{
  return ((uint32_t)x[0]<<24) + ((uint32_t)x[1]<<16) + ((uint32_t)x[2]<<8) + ((uint32_t)x[3]);
}

ENDIAN_INLINE int64_t
get_longlong (const uint8_t *x)
{
  uint32_t top = get_longword (x);
  uint32_t bottom = get_longword (x+4);
  return (((int64_t)top)<<32) | (int64_t)bottom;
}

ENDIAN_INLINE void
write_word (uint8_t *addr, uint16_t data)
{
  addr[0] = (data >> 8) & 0xff;
  addr[1] = data & 0xff;
}

ENDIAN_INLINE void
write_longword (uint8_t *addr, uint32_t data)
{
  addr[0] = (data >> 24) & 0xff;
  addr[1] = (data >> 16) & 0xff;
  addr[2] = (data >> 8) & 0xff;
  addr[3] = data & 0xff;
}

ENDIAN_INLINE void
write_longlong (uint8_t *addr, int64_t data)
{
  write_longword (addr, (uint32_t)(data >> 32));
  write_longword (addr+4, (uint32_t)data);
}
