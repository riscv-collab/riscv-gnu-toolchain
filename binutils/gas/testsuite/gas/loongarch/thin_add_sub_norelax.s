  .section .text
.L1:
  # 32_pcrel
  .4byte .L3-.L1
  .4byte .L3-.L1
.L2:
  # 64_pcrel
  .8byte .L3-.L2
  .8byte .L3-.L2

  .section sx
.L3:
  # no relocation
  .4byte .L3-.L4
  .8byte .L3-.L4
.L4:
  # add32+sub32
  .4byte .L4-.L5
  # add64+sub64
  .8byte .L4-.L5

  .section sy
.L5:
  # 32_pcrel
  .4byte .L1-.L5
  .4byte .L3-.L5
  # 64_pcrel
  .8byte .L1-.L5
  .8byte .L3-.L5

  .section sz
  # no relocation
  .4byte .L1-.L2
  .4byte .L3-.L4
  # add32+sub32
  .4byte .L3-.L5

  # no relocation
  .8byte .L1-.L2
  .8byte .L3-.L4
  # add64+sub64
  .8byte .L3-.L5
