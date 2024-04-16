#ld: -shared -T pr28597.t --no-warn-rwx-segments
#error: .*: discarded output section: `.plt'
#target: i?86-*-* x86_64-*-*
