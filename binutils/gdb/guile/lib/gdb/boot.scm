;; Bootstrap the Scheme side of the gdb module.
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
;; most of the initialization elsewhere.

;; Initialize the source and compiled file search paths.
;; Note: 'guile-data-directory' is provided by the C code.
(let ((module-dir (guile-data-directory)))
  (set! %load-path (cons module-dir %load-path))
  (set! %load-compiled-path (cons module-dir %load-compiled-path)))

;; Load the (gdb) module.  This needs to be done here because C code relies on
;; the availability of Scheme bindings such as '%print-exception-with-stack'.
;; Note: as of Guile 2.0.11, 'primitive-load' evaluates the code and 'load'
;; somehow ignores the '.go', hence 'load-compiled'.
(let ((gdb-go-file (search-path %load-compiled-path "gdb.go")))
  (if gdb-go-file
      (load-compiled gdb-go-file)
      (error "Unable to find gdb.go file.")))

;; Now that the Scheme side support is loaded, initialize it.
(let ((init-proc (@@ (gdb) %initialize!)))
  (init-proc))
