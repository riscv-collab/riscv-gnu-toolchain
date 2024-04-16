#source: large-sections.s
#PROG: objcopy
#as: --x32
#objcopy: --set-section-flags .ldata=alloc
#readelf: -S -W

#...
  \[[ 0-9]+\] \.text.*[ \t]+PROGBITS[ \t0-9a-f]+AX[ \t]+.*
#...
  \[[ 0-9]+\] \.data.*[ \t]+PROGBITS[ \t0-9a-f]+WA[ \t]+.*
#...
  \[[ 0-9]+\] \.ltext.*[ \t]+PROGBITS[ \t0-9a-f]+AXl[ \t]+.*
#...
  \[[ 0-9]+\] \.ldata.*[ \t]+PROGBITS[ \t0-9a-f]+WA[ \t]+.*
#pass
