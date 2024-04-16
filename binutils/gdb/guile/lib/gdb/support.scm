;; Internal support routines.
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

(define-module (gdb support))

;; Symbolic values for the ARG parameter of assert-type.

(define-public SCM_ARG1 1)
(define-public SCM_ARG2 2)

;; Utility to check the type of an argument, akin to SCM_ASSERT_TYPE.

(define-public (assert-type test-result arg pos func-name expecting)
  (if (not test-result)
      (scm-error 'wrong-type-arg func-name
		 "Wrong type argument in position ~a (expecting ~a): ~s"
		 (list pos expecting arg) (list arg))))
