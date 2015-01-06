ifeq ($(subdir),elf)
ifeq ($(build-shared),yes)
# This is needed for DSO loading from static binaries.
sysdep-dl-routines += dl-static
sysdep_routines += dl-static
sysdep-rtld-routines += dl-static
endif
endif

ifeq ($(subdir),stdlib)
gen-as-const-headers += ucontext_i.sym
endif
