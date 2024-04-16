#as: --64
#ld: -melf_x86_64 -T discarded1.t --no-error-rwx-segments
#error: .*discarded output section: `.got.plt'
