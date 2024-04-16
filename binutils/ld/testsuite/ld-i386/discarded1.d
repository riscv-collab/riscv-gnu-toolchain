#as: --32
#ld: -melf_i386 -T discarded1.t --no-error-rwx-segments
#error: .*discarded output section: `.got.plt'
