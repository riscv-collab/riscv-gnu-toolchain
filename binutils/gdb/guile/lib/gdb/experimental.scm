;; Various experimental utilities.
;; Anything in this file can change or disappear.
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

;; TODO: Split this file up by function?
;; E.g., (gdb experimental ports), etc.

(define-module (gdb experimental)
  #:use-module (gdb))

;; These are defined in C.
(define-public with-gdb-output-to-port (@@ (gdb) %with-gdb-output-to-port))
(define-public with-gdb-error-to-port (@@ (gdb) %with-gdb-error-to-port))

(define-public (with-gdb-output-to-string thunk)
  "Calls THUNK and returns all GDB output as a string."
  (call-with-output-string
   (lambda (p) (with-gdb-output-to-port p thunk))))
