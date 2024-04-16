#source: rgn-at6.s
#ld: -T rgn-at6.t --no-error-rwx-segments
#objdump: -h --wide
# Test that lma is aligned as for vma when lma_region==region.

#...
.* 0+10000 +0+10000 .*
.* 0+10100 +0+10100 .*
