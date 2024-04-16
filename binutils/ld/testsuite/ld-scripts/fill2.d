#source: fill_0.s
#ld: -T fill2.t
#readelf: -x.foo
#notarget: ![is_elf_format]
# See PR 30865 - a fill value expressed as a simple hexadecimal
# number behaves differently from other fill values.

Hex dump of section '.foo':
  0x00000000 00000000 00000090 91919191 00000092 ................
  0x00000010 00000093 00025000 00969500 00000097 ................
  0x00000020 00010203 04050607 04050607 04050607 ................
  0x00000030 08090a0b ffffffff .*
