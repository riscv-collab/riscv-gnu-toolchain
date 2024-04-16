#PROG: objcopy
#as: --64
#objcopy: --set-section-flags .text=alloc,readonly,code,large --set-section-flags .data=alloc,large
#readelf: -S -W

#...
  \[[ 0-9]+\] \.text.*[ \t]+PROGBITS[ \t0-9a-f]+AXl[ \t]+.*
#...
  \[[ 0-9]+\] \.data.*[ \t]+PROGBITS[ \t0-9a-f]+WAl[ \t]+.*
#...
  \[[ 0-9]+\] \.ltext.*[ \t]+PROGBITS[ \t0-9a-f]+AXl[ \t]+.*
#...
  \[[ 0-9]+\] \.ldata.*[ \t]+PROGBITS[ \t0-9a-f]+WAl[ \t]+.*
#pass
