#source: orphan-11.s
#ld: -T orphan-11.ld --orphan-handling=error --no-warn-rwx-segments
#objdump: -wh

#...
  . \.text .*
  . \.data .*
#pass
