#objdump: -rst
#name: Irix has no .pdr section
#as: -32 -mips32
#source: sync.s

#failif
.*\.pdr.*
#pass
