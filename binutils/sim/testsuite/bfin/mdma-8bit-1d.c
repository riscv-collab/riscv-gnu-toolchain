/* Basic MDMA device tests.
# mach: bfin
# cc: -mcpu=bf537 -nostdlib -lc
# sim: --environment operating --model bf537
*/

#include "test.h"

static volatile struct bfin_dma *s = (void *)MDMA_S0_NEXT_DESC_PTR;
static volatile struct bfin_dma *d = (void *)MDMA_D0_NEXT_DESC_PTR;

#include "mdma-skel.h"

void mdma_memcpy (bu32 dst, bu32 src, bu32 size)
{
  _mdma_memcpy (dst, src, size, 1);
}
