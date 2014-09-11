# Link libc.so at its likely load address for faster dynamic linking
LDLIBS-c.so += -Wl,--section-start=.dynamic=0x40100270

ifeq ($(filter -m64,$(CC)),)
CC += -m64
endif
