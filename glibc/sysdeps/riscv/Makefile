ifneq ($(all-rtld-routines),)
CFLAGS-rtld.c += -mno-plt
CFLAGS-dl-load.c += -mno-plt
CFLAGS-dl-cache.c += -mno-plt
CFLAGS-dl-lookup.c += -mno-plt
CFLAGS-dl-object.c += -mno-plt
CFLAGS-dl-reloc.c += -mno-plt
CFLAGS-dl-deps.c += -mno-plt
CFLAGS-dl-runtime.c += -mno-plt
CFLAGS-dl-error.c += -mno-plt
CFLAGS-dl-init.c += -mno-plt
CFLAGS-dl-fini.c += -mno-plt
CFLAGS-dl-debug.c += -mno-plt
CFLAGS-dl-misc.c += -mno-plt
CFLAGS-dl-version.c += -mno-plt
CFLAGS-dl-profile.c += -mno-plt
CFLAGS-dl-conflict.c += -mno-plt
CFLAGS-dl-tls.c += -mno-plt
CFLAGS-dl-origin.c += -mno-plt
CFLAGS-dl-scope.c += -mno-plt
CFLAGS-dl-execstack.c += -mno-plt
CFLAGS-dl-caller.c += -mno-plt
CFLAGS-dl-open.c += -mno-plt
CFLAGS-dl-close.c += -mno-plt
CFLAGS-dl-sysdep.c += -mno-plt
CFLAGS-dl-environ.c += -mno-plt
CFLAGS-dl-minimal.c += -mno-plt
CFLAGS-dl-static.c += -mno-plt
CFLAGS-dl-brk.c += -mno-plt
CFLAGS-dl-sbrk.c += -mno-plt
CFLAGS-dl-getcwd.c += -mno-plt
CFLAGS-dl-openat64.c += -mno-plt
CFLAGS-dl-opendir.c += -mno-plt
CFLAGS-dl-fxstatat64.c += -mno-plt
endif

CFLAGS-closedir.c += -mno-plt
CFLAGS-exit.c += -mno-plt
CFLAGS-cxa_atexit.c += -mno-plt

ifeq ($(subdir),misc)
sysdep_headers += sys/asm.h
endif

ASFLAGS-.os += $(pic-ccflag)

ifneq ($(with-fp),yes)
+cflags += -msoft-float
ASFLAGS += -msoft-float
sysdep-LDFLAGS += -msoft-float
endif
