;; Scheme side of the gdb module.
;;
;; Copyright (C) 2014-2024 Free Software Foundation, Inc.
;;
;; This file is part of GDB.
;;
;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3 of the License, or
;; (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;; This file is loaded with scm_c_primitive_load, which is ok, but files
;; loaded with it are not compiled.  So we do very little here, and do
;; most of the initialization in init.scm.

(define-module (gdb)
  ;; The version of the (gdb) module as (major minor).
  ;; Incompatible changes bump the major version.
  ;; Other changes bump the minor version.
  ;; It's not clear whether we need a patch-level as well, but this can
  ;; be added later if necessary.
  ;; This is not the GDB version on purpose.  This version tracks the Scheme
  ;; gdb module version.
  ;; TODO: Change to (1 0) when ready.
  #:version (0 1))

;; Export the bits provided by the C side.
;; This is so that the compiler can see the exports when
;; other code uses this module.
;; TODO: Generating this list would be nice, but it would require an addition
;; to the GDB build system.  Still, I think it's worth it.

(export

 ;; guile.c

 execute
 data-directory
 gdb-version
 host-config
 target-config

 ;; scm-arch.c

 arch?
 current-arch
 arch-name
 arch-charset
 arch-wide-charset

 arch-void-type
 arch-char-type
 arch-short-type
 arch-int-type
 arch-long-type

 arch-schar-type
 arch-uchar-type
 arch-ushort-type
 arch-uint-type
 arch-ulong-type
 arch-float-type
 arch-double-type
 arch-longdouble-type
 arch-bool-type
 arch-longlong-type
 arch-ulonglong-type

 arch-int8-type
 arch-uint8-type
 arch-int16-type
 arch-uint16-type
 arch-int32-type
 arch-uint32-type
 arch-int64-type
 arch-uint64-type

 ;; scm-block.c

 block?
 block-valid?
 block-start
 block-end
 block-function
 block-superblock
 block-global-block
 block-static-block
 block-global?
 block-static?
 block-symbols
 make-block-symbols-iterator
 block-symbols-progress?
 lookup-block

 ;; scm-breakpoint.c

 BP_NONE
 BP_BREAKPOINT
 BP_WATCHPOINT
 BP_HARDWARE_WATCHPOINT
 BP_READ_WATCHPOINT
 BP_ACCESS_WATCHPOINT

 WP_READ
 WP_WRITE
 WP_ACCESS

 make-breakpoint
 register-breakpoint!
 delete-breakpoint!
 breakpoints
 breakpoint?
 breakpoint-valid?
 breakpoint-number
 breakpoint-type
 brekapoint-visible?
 breakpoint-location
 breakpoint-expression
 breakpoint-enabled?
 set-breakpoint-enabled!
 breakpoint-silent?
 set-breakpoint-silent!
 breakpoint-ignore-count
 set-breakpoint-ignore-count!
 breakpoint-hit-count
 set-breakpoint-hit-count!
 breakpoint-thread
 set-breakpoint-thread!
 breakpoint-task
 set-breakpoint-task!
 breakpoint-condition
 set-breakpoint-condition!
 breakpoint-stop
 set-breakpoint-stop!
 breakpoint-commands

 ;; scm-cmd.c

 make-command
 register-command!
 command?
 command-valid?
 dont-repeat

 COMPLETE_NONE
 COMPLETE_FILENAME
 COMPLETE_LOCATION
 COMPLETE_COMMAND
 COMPLETE_SYMBOL
 COMPLETE_EXPRESSION

 COMMAND_NONE
 COMMAND_RUNNING
 COMMAND_DATA
 COMMAND_STACK
 COMMAND_FILES
 COMMAND_SUPPORT
 COMMAND_STATUS
 COMMAND_BREAKPOINTS
 COMMAND_TRACEPOINTS
 COMMAND_OBSCURE
 COMMAND_MAINTENANCE
 COMMAND_USER

 ;; scm-disasm.c

 arch-disassemble

 ;; scm-exception.c

 make-exception
 exception?
 exception-key
 exception-args

 ;; scm-frame.c

 NORMAL_FRAME
 DUMMY_FRAME
 INLINE_FRAME
 TAILCALL_FRAME
 SIGTRAMP_FRAME
 ARCH_FRAME
 SENTINEL_FRAME

 FRAME_UNWIND_NO_REASON
 FRAME_UNWIND_NULL_ID
 FRAME_UNWIND_OUTERMOST
 FRAME_UNWIND_UNAVAILABLE
 FRAME_UNWIND_INNER_ID
 FRAME_UNWIND_SAME_ID
 FRAME_UNWIND_NO_SAVED_PC
 FRAME_UNWIND_MEMORY_ERROR

 frame?
 frame-valid?
 frame-name
 frame-type
 frame-arch
 frame-unwind-stop-reason
 frame-pc
 frame-block
 frame-function
 frame-older
 frame-newer
 frame-sal
 frame-read-register
 frame-read-var
 frame-select
 newest-frame
 selected-frame
 unwind-stop-reason-string

 ;; scm-iterator.c

 make-iterator
 iterator?
 iterator-object
 iterator-progress
 set-iterator-progress!
 iterator-next!
 end-of-iteration
 end-of-iteration?

 ;; scm-lazy-string.c
 ;; FIXME: Where's the constructor?

 lazy-string?
 lazy-string-address
 lazy-string-length
 lazy-string-encoding
 lazy-string-type
 lazy-string->value

 ;; scm-math.c

 valid-add
 value-sub
 value-mul
 value-div
 value-rem
 value-mod
 value-pow
 value-not
 value-neg
 value-pos
 value-abs
 value-lsh
 value-rsh
 value-min
 value-max
 value-lognot
 value-logand
 value-logior
 value-logxor
 value=?
 value<?
 value<=?
 value>?
 value>=?

 ;; scm-objfile.c

 objfile?
 objfile-valid?
 objfile-filename
 objfile-progspace
 objfile-pretty-printers
 set-objfile-pretty-printers!
 current-objfile
 objfiles

 ;; scm-param.c

 PARAM_BOOLEAN
 PARAM_AUTO_BOOLEAN
 PARAM_ZINTEGER
 PARAM_UINTEGER
 PARAM_ZUINTEGER
 PARAM_ZUINTEGER_UNLIMITED
 PARAM_STRING
 PARAM_STRING_NOESCAPE
 PARAM_OPTIONAL_FILENAME
 PARAM_FILENAME
 PARAM_ENUM

 make-parameter
 register-parameter!
 parameter?
 parameter-value
 set-parameter-value!

 ;; scm-ports.c

 input-port
 output-port
 error-port
 stdio-port?
 open-memory
 memory-port?
 memory-port-range
 memory-port-read-buffer-size
 set-memory-port-read-buffer-size!
 memory-port-write-buffer-size
 set-memory-port-write-buffer-size!
 ;; with-gdb-output-to-port, with-gdb-error-to-port are in experimental.scm.

 ;; scm-pretty-print.c

 make-pretty-printer
 pretty-printer?
 pretty-printer-enabled?
 set-pretty-printer-enabled!
 make-pretty-printer-worker
 pretty-printer-worker?
 pretty-printers
 set-pretty-printers!

 ;; scm-progspace.c

 progspace?
 progspace-valid?
 progspace-filename
 progspace-objfiles
 progspace-pretty-printers
 set-progspace-pretty-printers!
 current-progspace
 progspaces

 ;; scm-gsmob.c

 gdb-object-kind

 ;; scm-string.c

 string->argv

 ;; scm-symbol.c

 SYMBOL_LOC_UNDEF
 SYMBOL_LOC_CONST
 SYMBOL_LOC_STATIC
 SYMBOL_LOC_REGISTER
 SYMBOL_LOC_ARG
 SYMBOL_LOC_REF_ARG
 SYMBOL_LOC_LOCAL
 SYMBOL_LOC_TYPEDEF
 SYMBOL_LOC_LABEL
 SYMBOL_LOC_BLOCK
 SYMBOL_LOC_CONST_BYTES
 SYMBOL_LOC_UNRESOLVED
 SYMBOL_LOC_OPTIMIZED_OUT
 SYMBOL_LOC_COMPUTED
 SYMBOL_LOC_REGPARM_ADDR

 SYMBOL_UNDEF_DOMAIN
 SYMBOL_VAR_DOMAIN
 SYMBOL_STRUCT_DOMAIN
 SYMBOL_LABEL_DOMAIN
 SYMBOL_VARIABLES_DOMAIN
 SYMBOL_FUNCTIONS_DOMAIN
 SYMBOL_TYPES_DOMAIN

 symbol?
 symbol-valid?
 symbol-type
 symbol-symtab
 symbol-line
 symbol-name
 symbol-linkage-name
 symbol-print-name
 symbol-addr-class
 symbol-argument?
 symbol-constant?
 symbol-function?
 symbol-variable?
 symbol-needs-frame?
 symbol-value
 lookup-symbol
 lookup-global-symbol

 ;; scm-symtab.c

 symtab?
 symtab-valid?
 symtab-filename
 symtab-fullname
 symtab-objfile
 symtab-global-block
 symtab-static-block
 sal?
 sal-valid?
 sal-symtab
 sal-line
 sal-pc
 sal-last
 find-pc-line

 ;; scm-type.c

 TYPE_CODE_BITSTRING
 TYPE_CODE_PTR
 TYPE_CODE_ARRAY
 TYPE_CODE_STRUCT
 TYPE_CODE_UNION
 TYPE_CODE_ENUM
 TYPE_CODE_FLAGS
 TYPE_CODE_FUNC
 TYPE_CODE_INT
 TYPE_CODE_FLT
 TYPE_CODE_VOID
 TYPE_CODE_SET
 TYPE_CODE_RANGE
 TYPE_CODE_STRING
 TYPE_CODE_ERROR
 TYPE_CODE_METHOD
 TYPE_CODE_METHODPTR
 TYPE_CODE_MEMBERPTR
 TYPE_CODE_REF
 TYPE_CODE_CHAR
 TYPE_CODE_BOOL
 TYPE_CODE_COMPLEX
 TYPE_CODE_TYPEDEF
 TYPE_CODE_NAMESPACE
 TYPE_CODE_DECFLOAT
 TYPE_CODE_INTERNAL_FUNCTION

 type?
 lookup-type
 type-code
 type-fields
 type-tag
 type-sizeof
 type-strip-typedefs
 type-array
 type-vector
 type-pointer
 type-range
 type-reference
 type-target
 type-const
 type-volatile
 type-unqualified
 type-name
 type-num-fields
 type-fields
 make-field-iterator
 type-field
 type-has-field?
 field?
 field-name
 field-type
 field-enumval
 field-bitpos
 field-bitsize
 field-artificial?
 field-baseclass?

 ;; scm-value.c

 value?
 make-value
 value-optimized-out?
 value-address
 value-type
 value-dynamic-type
 value-cast
 value-dynamic-cast
 value-reinterpret-cast
 value-dereference
 value-referenced-value
 value-field
 value-subscript
 value-call
 value->bool
 value->integer
 value->real
 value->bytevector
 value->string
 value->lazy-string
 value-lazy?
 make-lazy-value
 value-fetch-lazy!
 value-print
 parse-and-eval
 history-ref
)

;; Load the rest of the Scheme side.

(include "gdb/init.scm")

;; These come from other files, but they're really part of this module.

(export

 ;; init.scm
 orig-input-port
 orig-output-port
 orig-error-port
 throw-user-error
)
