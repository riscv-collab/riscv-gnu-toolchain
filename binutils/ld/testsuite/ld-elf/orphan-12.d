#source: orphan-12.s
#ld: -T orphan-11.ld --strip-debug --orphan-handling=error --no-warn-rwx-segments
#objdump: -wh

#...
  . \.text .*
  . \.data .*
#pass
