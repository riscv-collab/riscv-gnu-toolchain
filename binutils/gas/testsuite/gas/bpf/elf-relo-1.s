    .global foo
    .data
    .byte 1
    .byte 2
    .byte 3
    .byte 4
foo:
    .byte 5
    .byte 6
bar:
    .byte 7
    .byte 8

    .text
    .align 3
    .type baz, @function
baz:
    lddw    %r1, foo
    mov     %r2, bar
    lddw    %r3, qux
    call    somefunc

    .type qux, @function
qux:
    exit
