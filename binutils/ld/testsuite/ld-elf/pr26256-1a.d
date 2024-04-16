#source: pr26256-1.s
#ld: -e _start -T pr26256-1.t --no-warn-rwx-segments
#nm: -n

#...
[0-9a-f]+ T _start
#pass
