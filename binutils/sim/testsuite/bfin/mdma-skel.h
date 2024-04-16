#include <stdlib.h>
#include <string.h>

void _mdma_memcpy (bu32 dst, bu32 src, bu32 size, bs16 mod)
{
  bu32 count = size >> (abs (mod) / 2);
  bu16 wdsize;
  switch (abs (mod))
    {
    case 4: wdsize = WDSIZE_32; break;
    case 2: wdsize = WDSIZE_16; break;
    default: wdsize = WDSIZE_8; break;
    }

  s->config = d->config = 0;

  d->irq_status = DMA_DONE | DMA_ERR;

  /* Destination */
  d->start_addr = dst;
  d->x_count = count;
  d->x_modify = mod;
  d->irq_status = DMA_DONE | DMA_ERR;

  /* Source */
  s->start_addr = src;
  s->x_count = count;
  s->x_modify = mod;
  s->irq_status = DMA_DONE | DMA_ERR;

  /* Enable */
  s->config = DMAEN | wdsize;
  d->config = WNR | DI_EN | DMAEN | wdsize;

  while (!(d->irq_status & DMA_DONE))
    continue;
}

void mdma_memcpy (bu32 dst, bu32 src, bu32 size);

#ifndef MAX_LEN
#define MAX_LEN 0x40000
#endif
bu32 _data[(MAX_LEN / 4) * 2 + 3];
char *data = (char *)(_data + 1);

int _start (void)
{
  char *src, *dst;
  bu32 len, canary, *canaries[3];

  canary = 0x12345678;

  len = 4;
  while (len < MAX_LEN)
    {
      src = data;
      dst = data + len + 4;
      /* Set up the canaries.  */
      canaries[0] = (void *)&src[-4];
      canaries[1] = (void *)&dst[-4];
      canaries[2] = (void *)&dst[len];
      *canaries[0] = *canaries[1] = *canaries[2] = canary;

      memset (src, 0xad, len);
      memset (dst, 0x00, len);

      mdma_memcpy ((bu32)dst, (bu32)src, len);
      if (memcmp (src, dst, len))
	DBG_FAIL;

      if (*canaries[0] != canary ||
	  *canaries[1] != canary ||
	  *canaries[2] != canary)
	DBG_FAIL;

      len <<= 4;
    }
  DBG_PASS;
}
