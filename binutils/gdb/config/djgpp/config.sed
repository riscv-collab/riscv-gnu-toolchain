s|po2tbl\.sed\.in|po2tblsed.in|g
s|gdb\.c++|gdb.cxx|g
/ac_rel_source/s|ln -s|cp -p|
s|\.gdbinit|gdb.ini|g
# This works around a bug in DJGPP port of Bash 2.0x
s|return $ac_retval|(&)|g
# DJGPP port of Bash 2.04 doesn't like this redirection of stdin
/exec 7</s|7<&0 </dev/null||
# Make sure $PATH_SEPARATOR is set correctly
/if test "${PATH_SEPARATOR+set}"/i\
export PATH_SEPARATOR=';'

# Edit Makefiles for 8+3 DOS file-name compliance and path separator.
# This should go near the beginning of
# the substitutions script, before the branch command that
# skips any lines without @...@ in them.
# Any commands that can match again after substitution must
# do a conditional branch to next cycle (;t), or else Sed might hang.
/\/@\[a-zA-Z_\]\[a-zA-Z_0-9\]\*@\/!b/i\
  /VPATH *=/s,\\([^A-z]\\):,\\1;,g\
  s,\\([yp*]\\)\\.tab,\\1_tab,g\
  s,\\$@\\.tmp,\\$@_tmp,g\
  s,\\$@\\.new,\\$@_new,g\
  s,standards\\.info\\*,standard*.inf*,\
  s,configure\\.info\\*,configur*.inf*,\
  s,\\.info\\*,.inf* *.i[1-9] *.i[1-9][0-9],\
  s,\\.gdbinit,gdb.ini,g\
  /TEXINPUTS=/s,:,\\";\\",g\
  s,config\\.h\\.in,config.h-in,g;t t\
  /^	@rm -f/s,\\$@-\\[0-9\\]\\[0-9\\],& *.i[1-9] *.i[1-9][0-9],;t\
  /\\$\\$file-\\[0-9\\]/s,echo,& *.i[1-9] *.i[1-9][0-9],;t\
  /\\$\\$file-\\[0-9\\]/s,rm -f \\$\\$file,& \\${PACKAGE}.i[1-9] \\${PACKAGE}.i[1-9][0-9],;t

# We have an emulation of nl_langinfo in go32-nat.c that supports CODESET.
/^for ac_var in $ac_precious_vars; do/i\
am_cv_langinfo_codeset=yes\
bash_cv_langinfo_codeset=yes\
ac_cv_header_nl_types_h=yes

# Prevent splitting of config.status substitutions, because that
# might break multi-line sed commands.
/ac_max_sed_lines=[0-9]/s,=.*$,=`sed -n "$=" $tmp/subs.sed`,

/^ac_given_srcdir=/,/^CEOF/ {
  /^s%@TOPLEVEL_CONFIGURE_ARGUMENTS@%/a\
  /@test ! -f /s,\\(.\\)\$, export am_cv_exeext=.exe; export lt_cv_sys_max_cmd_len=12288; \\1,\
  /@test -f stage_last /s,\\(.\\)\$, export am_cv_exeext=.exe; export lt_cv_sys_max_cmd_len=12288; \\1,

}

/^ *# *Handling of arguments/,/^done/ {
  s| config.h"| config.h:config.h-in"|
  s| config.intl"| config.intl:config_intl.in"|
  s|config.h\([^-:"a-z]\)|config.h:config.h-in\1|
}

/^[ 	]*\/\*)/s,/\*,/*|[A-z]:/*,
/^ *ac_config_headers=/s, config.h", config.h:config.h-in",
/^ *ac_config_files=/s, config.intl", config.intl:config_intl.in",
