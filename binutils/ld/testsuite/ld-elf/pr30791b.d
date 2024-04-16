#source: pr30791c.s
#source: pr30791d.s
#ld: -r
#readelf: -S --wide
#xfail: hppa-*-* z80-*-*
# hppa fails since .text sections aren't merged for relocatable link.
# z80 fails since a, b, c, d are registers for z80.

#failif
#...
  \[[ 0-9]+\] __patchable_function_entries[ \t]+PROGBITS[ \t0-9a-f]+WAL.*
#...
  \[[ 0-9]+\] __patchable_function_entries[ \t]+PROGBITS[ \t0-9a-f]+WAL.*
#...
