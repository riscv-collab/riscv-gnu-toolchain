;; Copyright (C) 2010-2024 Free Software Foundation, Inc.
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

;; This file is part of the GDB testsuite.

(use-modules (gdb) (gdb printing))

(define (make-pp_ss-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (let ((a (value-field val "a"))
	   (b (value-field val "b")))
       (format #f "a=<~A> b=<~A>" a b)))
   #f))

(define (get-type-for-printing val)
  "Return type of val, stripping away typedefs, etc."
  (let ((type (value-type val)))
    (if (= (type-code type) TYPE_CODE_REF)
	(set! type (type-target type)))
    (type-strip-typedefs (type-unqualified type))))

(define (make-pretty-printer-dict)
  (let ((dict (make-hash-table)))
    (hash-set! dict "struct ss" make-pp_ss-printer)
    (hash-set! dict "ss" make-pp_ss-printer)
    dict))

(define *pretty-printer*
 (make-pretty-printer
  "pretty-printer-test"
  (let ((pretty-printers-dict (make-pretty-printer-dict)))
    (lambda (matcher val)
      "Look-up and return a pretty-printer that can print val."
      (let ((type (get-type-for-printing val)))
	(let ((typename (type-tag type)))
	  (if typename
	      (let ((printer-maker (hash-ref pretty-printers-dict typename)))
		(and printer-maker (printer-maker val)))
	      #f)))))))

(append-pretty-printer! #f *pretty-printer*)
