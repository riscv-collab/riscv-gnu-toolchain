#source: empty-address-3.s
#ld: -T empty-address-3b.t
#nm: -n
#xfail: [is_xcoff_format]

#...
0+0 T _start
#...
0+10 [DT] __data_end
#pass
