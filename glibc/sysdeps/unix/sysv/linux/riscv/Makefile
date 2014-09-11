ifeq ($(subdir),signal)
#sysdep_routines += sigsuspend
endif

ifeq ($(subdir),misc)

no_syscall_list_h = 1

# A callable macro that expands to a shell command.  Preprocess file $(1)
# using ABI option $(2) and see which macros it defines.  Print FOO for each
# macro of the form __NR$(3)_FOO, filtering out ABI-specific __NR macros
# that have a prefix other than $(3).
mips_list_syscalls =	$(filter-out -m%,$(CC)) -E -x c $(+includes) \
			    $(sysincludes) -D_LIBC -dM -m$(2) $(1) | \
			sed -n 's@^\#define __NR$(3)_\([^ ]*\) .*@\1@p' | \
			sed -e '/^[ON]32_/d' -e '/^N64_/d' -e '/^64_/d' | \
			LC_ALL=C sort

# Generate a list of SYS_* macros from the linux __NR macros.
#
# Before version 2.6, linux had separate 32-bit and 64-bit MIPS ports,
# each with its own set of headers.  The ports were merged for 2.6 and
# this merged port defines the syscalls in a slightly different way.
# There are therefore three sets of headers that we need to consider:
#
#    (1) Headers from the separate 32-bit MIPS port.  They just define
#	 a single list of __NR macros.
#
#    (2) Headers from the separate 64-bit MIPS port.  They unconditionally
#	 define syscalls for all three ABIs, with o32 syscalls prefixed
#	 by __NR_O32, n32 syscalls prefixed by __NR_N32 and n64 syscalls
#	 prefixed by plain __NR.
#
#    (3) Headers from the combined port.  They use the _RISCV_SIM macro to
#	 define the right set of syscalls for the current ABI.  The syscalls
#	 themselves have no special ABI prefix, but the headers also define:
#
#	    __NR_O32_Linux{,_syscalls}
#	    __NR_N32_Linux{,_syscalls}
#	    __NR_64_Linux{,_syscalls}
#
# In case (1) we just want a simple list of SYS_* macros.  In cases (2)
# and (3) we want a file that will work for all three ABIs, regardless
# of which ABI we are currently using.  We also want the file to work
# if the user later moves from (2) to (3).  Thus the file we create
# for (2) and (3) has the form:
#
#    #if _RISCV_SIM == _ABIN32
#    # ifdef __NR_N32_open
#    #  define SYS_n32syscall1 __NR_N32_n32syscall1
#    #  ...
#    # else
#    #  define SYS_n32syscall1 __NR_n32syscall1
#    #  ...
#    # endif
#    #elif _RISCV_SIM == _ABI64
#    # define SYS_n64syscall1 __NR_n64syscall1
#    # ...
#    #else
#    # ifdef __NR_O32_open
#    #  define SYS_o32syscall1 __NR_O32_o32syscall1
#    #  ...
#    # else
#    #  define SYS_o32syscall1 __NR_o32syscall1
#    #  ...
#    # endif
#    #endif
#
# Here, __NR_N32_open and __NR_O32_open are used to detect case (2)
# over case (3).  The n64 SYS_* macros can always use the normal
# ABI-less names.
$(objpfx)syscall-%.h $(objpfx)syscall-%.d: ../sysdeps/unix/sysv/linux/mips/sys/syscall.h
	$(make-target-directory)
	$(CC) -E -x c $(+includes) $(sysincludes) -D_LIBC $< -MD -MP \
	      -MF $(@:.h=.d)-t -MT '$(@:.d=.h) $(@:.h=.d)' > /dev/null
	{ \
	 echo '/* Generated at libc build time from kernel syscall list.  */';\
	 echo ''; \
	 echo '#ifndef _SYSCALL_H'; \
	 echo '# error "Never use <bits/syscall.h> directly; include <sys/syscall.h> instead."'; \
	 echo '#endif'; \
	 echo ''; \
	 echo '#include <sgidefs.h>'; \
	 rm -f $(@:.d=.h).new32 $(@:.d=.h).newn32 $(@:.d=.h).new64; \
	 $(call mips_list_syscalls,$<,n32,_N32) > $(@:.d=.h).newn32; \
	 if test -s $(@:.d=.h).newn32; then \
	   if grep open $(@:.d=.h).newn32 > /dev/null; then \
	     $(call mips_list_syscalls,$<,32,_O32) > $(@:.d=.h).new32; \
	     $(call mips_list_syscalls,$<,64,) > $(@:.d=.h).new64; \
	   else \
	     $(call mips_list_syscalls,$<,32,) > $(@:.d=.h).new32; \
	     $(call mips_list_syscalls,$<,n32,) > $(@:.d=.h).newn32; \
	     $(call mips_list_syscalls,$<,64,) > $(@:.d=.h).new64; \
	   fi; \
	   echo '#if _RISCV_SIM == _ABIN32'; \
	   echo '# ifdef __NR_N32_open'; \
	   sed 's@\(.*\)@#  define SYS_\1 __NR_N32_\1@' < $(@:.d=.h).newn32; \
	   echo '# else'; \
	   sed 's@\(.*\)@#  define SYS_\1 __NR_\1@' < $(@:.d=.h).newn32; \
	   echo '# endif'; \
	   echo '#elif _RISCV_SIM == _ABI64'; \
	   sed 's@\(.*\)@# define SYS_\1 __NR_\1@' < $(@:.d=.h).new64; \
	   echo '#else'; \
	   echo '# ifdef __NR_O32_open'; \
	   sed 's@\(.*\)@#  define SYS_\1 __NR_O32_\1@' < $(@:.d=.h).new32; \
	   echo '# else'; \
	   sed 's@\(.*\)@#  define SYS_\1 __NR_\1@' < $(@:.d=.h).new32; \
	   echo '# endif'; \
	   echo '#endif'; \
	 else \
	   $(CC) -E -x c $(+includes) $(sysincludes) -D_LIBC -dM $< | \
	   sed -n 's@^\#define __NR_\([^ ]*\) .*@\#define SYS_\1 __NR_\1@p' | \
	   LC_ALL=C sort; \
	 fi; \
	 rm -f $(@:.d=.h).new32 $(@:.d=.h).newn32 $(@:.d=.h).new64; \
	} > $(@:.d=.h).new
	mv -f $(@:.d=.h).new $(@:.d=.h)
ifneq (,$(objpfx))
	sed $(sed-remove-objpfx) $(@:.h=.d)-t > $(@:.h=.d)-t2
	rm -f $(@:.h=.d)-t
	mv -f $(@:.h=.d)-t2 $(@:.h=.d)
else
	mv -f $(@:.h=.d)-t $(@:.h=.d)
endif
endif

ifeq ($(subdir),elf)
ifeq ($(build-shared),yes)
# This is needed for DSO loading from static binaries.
sysdep-dl-routines += dl-static
sysdep_routines += dl-static
sysdep-rtld-routines += dl-static
endif
endif

ifeq ($(subdir),stdlib)
sysdep_routines += __start_context
gen-as-const-headers += ucontext_i.sym
endif
