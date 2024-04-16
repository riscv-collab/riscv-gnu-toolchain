# Fix bug: alignment padding must a multiple of 4 if .align has second parameter
.data
  .byte 1
  .align 3, 2
  .4byte 3
