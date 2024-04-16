/* Basic MDMA device tests.
# mach: bfin
# cc: -mcpu=bf537 -nostdlib -lc
# sim: --environment operating --model bf537
*/

#include "test.h"

static volatile struct bfin_dma *s = (void *)MDMA_S1_NEXT_DESC_PTR;
static volatile struct bfin_dma *d = (void *)MDMA_D1_NEXT_DESC_PTR;

#include "mdma-skel.h"

void mdma_memcpy (bu32 dst, bu32 src, bu32 size)
{
  /* Negative transfers start at end of buffer.  */
  _mdma_memcpy (dst + size - 1, src + size - 1, size, -1);
}
