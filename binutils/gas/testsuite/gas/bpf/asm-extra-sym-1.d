#as: -EL -mdialect=pseudoc
#nm: --numeric-sort
#source: asm-extra-sym-1.s
#name: BPF pseudoc no extra symbols 1

# Note: there should be no output from nm.
# Previously a bug in the BPF parser created an UND '*' symbol.
