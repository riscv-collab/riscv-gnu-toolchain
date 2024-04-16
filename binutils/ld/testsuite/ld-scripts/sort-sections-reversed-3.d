#source: sort_b_a.s
#ld: -T sort-sections-reversed-3.t  --no-warn-rwx-segments
#nm: -n

# Check that REVERSE implies SORT_BY_NAME for sections.
# Also check that EXCLUDE_FILE() is supported inside REVERSE.
#...
0[0-9a-f]* t text3
#...
0[0-9a-f]* t text2
#...
0[0-9a-f]* t text1
#...
0[0-9a-f]* t text
#pass
