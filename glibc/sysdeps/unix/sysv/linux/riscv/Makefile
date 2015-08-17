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

ifeq ($(subdir),crypt)
libcrypt-sysdep_routines += sysdep
endif

ifeq ($(subdir),nss)
libnss_db-sysdep_routines += sysdep
libnss_db-shared-only-routines += sysdep
endif

ifeq ($(subdir),rt)
librt-sysdep_routines += sysdep
librt-shared-only-routines += sysdep
endif
