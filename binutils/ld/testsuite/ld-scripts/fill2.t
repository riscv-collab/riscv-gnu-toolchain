SECTIONS {
  .foo :
  {
    . += 4;
    FILL (144)      # Decimal values zero extend to 4 bytes.  Fills with: 00 00 00 90
    . += 4;
    FILL (0x91)     # Hex values do not zero extend.  Fills with: 91 91 91 91
    . += 4;
    FILL ($92)      # A dollar prefix indicates a hex values that does zero extend.  Fills with: 00 00 00 92
    . += 4;
    FILL (93H)      # An H suffix does the same.  Fills with: 00 00 00 93
    . += 4;
    FILL (0x94K)    # A hex value with a manitude suffix zero extends.  Fills with: 00 02 50 00
    . += 4;
    FILL (0x009695) # Zeros in hex values are significant.  Values are big-endian.  Fills with: 00 96 95 00
    . += 4;
    FILL (0x90+0x7) # An expression containing hex values also zero extends.  Fills with: 00 00 00 97
    . += 4;
    FILL (0x0001020304050607) # Hex values can be used to specify fills with more than 4 bytes.  Fills with: 00 01 02 03 04 05 06 07
    . += 8;
    FILL ($0001020304050607)  # But non-hex or $-hex or suffix-hex values cannot.  Fills with 04 05 06 07 04 05 06 07
    . += 8;
    FILL (0x08090a0b0c0d0e0f) # Extra bytes at the end of a value are silently ignored.  Fills with 08 09 0a 0b
    . += 4;
    LONG(0xffffffff)
  } =0

  /DISCARD/ : { *(*) }  
}
