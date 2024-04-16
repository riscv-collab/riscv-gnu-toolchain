#source: sort-file1.s
#source: sort-file2.s
#ld: -T sort-file-reversed-1.t --no-warn-rwx-segments
#nm: -n

# Check that SORT_BY_NAME+REVERSE on filenames works.
# The text sections should come in reversed sorted order, the data
# sections in input order.  Note how we specifically pass
# the object filenames in alphabetical order
#...
0[0-9a-f]* t infile2
#...
0[0-9a-f]* t infile1
#...
0[0-9a-f]* d data1
#...
0[0-9a-f]* d data2
#pass
