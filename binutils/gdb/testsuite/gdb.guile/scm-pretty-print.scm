;; Copyright (C) 2008-2024 Free Software Foundation, Inc.
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
;; It tests Scheme pretty printers.

(use-modules (gdb) (gdb printing))

(define (make-pointer-iterator pointer len)
  (let ((next! (lambda (iter)
		 (let* ((start (iterator-object iter))
			(progress (iterator-progress iter))
			(current (car progress))
			(len (cdr progress)))
		   (if (= current len)
		       (end-of-iteration)
		       (let ((pointer (value-add start current)))
			 (set-car! progress (+ current 1))
			 (cons (format #f "[~A]" current)
			       (value-dereference pointer))))))))
    (make-iterator pointer (cons 0 len) next!)))

(define (make-pointer-iterator-except pointer len)
  (let ((next! (lambda (iter)
		 (if *exception-flag*
		     (throw 'gdb:memory-error "hi bob"))
		 (let* ((start (iterator-object iter))
			(progress (iterator-progress iter))
			(current (car progress))
			(len (cdr progress)))
		   (if (= current len)
		       (end-of-iteration)
		       (let ((pointer (value-add start current)))
			 (set-car! progress (+ current 1))
			 (cons (format #f "[~A]" current)
			       (value-dereference pointer))))))))
    (make-iterator pointer (cons 0 len) next!)))

;; Test returning a <gdb:value> from a printer.

(define (make-string-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (value-field (value-field val "whybother")
		  "contents"))
   #f))

;; Test a printer with children.

(define (make-container-printer val)
  ;; This is a little different than the Python version in that if there's
  ;; an error accessing these fields we'll throw it at matcher time instead
  ;; of at printer time.  Done this way to explore the possibilities.
  (let ((name (value-field val "name"))
	(len (value-field val "len"))
	(elements (value-field val "elements")))
    (make-pretty-printer-worker
     #f
     (lambda (printer)
       (format #f "container ~A with ~A elements"
	       name len))
     (lambda (printer)
       (make-pointer-iterator elements (value->integer len))))))

;; Test "array" display hint.

(define (make-array-printer val)
  (let ((name (value-field val "name"))
	(len (value-field val "len"))
	(elements (value-field val "elements")))
    (make-pretty-printer-worker
     "array"
     (lambda (printer)
       (format #f "array ~A with ~A elements"
	       name len))
     (lambda (printer)
       (make-pointer-iterator elements (value->integer len))))))

;; Flag to make no-string-container printer throw an exception.

(define *exception-flag* #f)

;; Test a printer where to_string returns #f.

(define (make-no-string-container-printer val)
  (let ((len (value-field val "len"))
	(elements (value-field val "elements")))
    (make-pretty-printer-worker
     #f
     (lambda (printer) #f)
     (lambda (printer)
       (make-pointer-iterator-except elements (value->integer len))))))

;; The actual pretty-printer for pp_s is split out so that we can pass
;; in a prefix to distinguish objfile/progspace/global.

(define (pp_s-printer prefix val)
  (let ((a (value-field val "a"))
	(b (value-field val "b")))
    (if (not (value=? (value-address a) b))
	(error (format #f "&a(~A) != b(~A)"
		       (value-address a) b)))
    (format #f "~aa=<~A> b=<~A>" prefix a b)))

(define (make-pp_s-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (pp_s-printer "" val))
   #f))

(define (make-pp_ss-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (let ((a (value-field val "a"))
	   (b (value-field val "b")))
       (format #f "a=<~A> b=<~A>" a b)))
   #f))

(define (make-pp_sss-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (let ((a (value-field val "a"))
	   (b (value-field val "b")))
       (format #f "a=<~A> b=<~A>" a b)))
   #f))

(define (make-pp_multiple_virtual-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (format #f "pp value variable is: ~A" (value-field val "value")))
   #f))

(define (make-pp_vbase1-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (format #f "pp class name: ~A" (type-tag (value-type val))))
   #f))

(define (make-pp_nullstr-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (value->string (value-field val "s")
		    #:encoding (arch-charset (current-arch))))
   #f))

(define (make-pp_ns-printer val)
  (make-pretty-printer-worker
   "string"
   (lambda (printer)
     (let ((len (value-field val "length")))
       (value->string (value-field val "null_str")
		      #:encoding (arch-charset (current-arch))
		      #:length (value->integer len))))
   #f))

(define *pp-ls-encoding* #f)

(define (make-pp_ls-printer val)
  (make-pretty-printer-worker
   "string"
   (lambda (printer)
     (if *pp-ls-encoding*
	 (value->lazy-string (value-field val "lazy_str")
			     #:encoding *pp-ls-encoding*)
	 (value->lazy-string (value-field val "lazy_str"))))
   #f))

(define (make-pp_hint_error-printer val)
  "Use an invalid value for the display hint."
  (make-pretty-printer-worker
   42
   (lambda (printer) "hint_error_val")
   #f))

(define (make-pp_children_as_list-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer) "children_as_list_val")
   (lambda (printer) (make-list-iterator (list (cons "one" 1))))))

(define (make-pp_outer-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (format #f "x = ~A" (value-field val "x")))
   (lambda (printer)
     (make-list-iterator (list (cons "s" (value-field val "s"))
			       (cons "x" (value-field val "x")))))))

(define (make-memory-error-string-printer val)
  (make-pretty-printer-worker
   "string"
   (lambda (printer)
     (scm-error 'gdb:memory-error "memory-error-printer"
		"Cannot access memory." '() '()))
   #f))

(define (make-pp_eval_type-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (execute "bt" #:to-string #t)
     (format #f "eval=<~A>"
	     (value-print
	      (parse-and-eval
	       "eval_func (123456789, 2, 3, 4, 5, 6, 7, 8)"))))
   #f))

(define (get-type-for-printing val)
  "Return type of val, stripping away typedefs, etc."
  (let ((type (value-type val)))
    (if (= (type-code type) TYPE_CODE_REF)
	(set! type (type-target type)))
    (type-strip-typedefs (type-unqualified type))))

(define (disable-matcher!)
  (set-pretty-printer-enabled! *pretty-printer* #f))

(define (enable-matcher!)
  (set-pretty-printer-enabled! *pretty-printer* #t))

(define (make-pretty-printer-dict)
  (let ((dict (make-hash-table)))
    (hash-set! dict "struct s" make-pp_s-printer)
    (hash-set! dict "s" make-pp_s-printer)
    (hash-set! dict "S" make-pp_s-printer)

    (hash-set! dict "struct ss" make-pp_ss-printer)
    (hash-set! dict "ss" make-pp_ss-printer)
    (hash-set! dict "const S &" make-pp_s-printer)
    (hash-set! dict "SSS" make-pp_sss-printer)
    
    (hash-set! dict "VirtualTest" make-pp_multiple_virtual-printer)
    (hash-set! dict "Vbase1" make-pp_vbase1-printer)

    (hash-set! dict "struct nullstr" make-pp_nullstr-printer)
    (hash-set! dict "nullstr" make-pp_nullstr-printer)
    
    ;; Note that we purposely omit the typedef names here.
    ;; Printer lookup is based on canonical name.
    ;; However, we do need both tagged and untagged variants, to handle
    ;; both the C and C++ cases.
    (hash-set! dict "struct string_repr" make-string-printer)
    (hash-set! dict "struct container" make-container-printer)
    (hash-set! dict "struct justchildren" make-no-string-container-printer)
    (hash-set! dict "string_repr" make-string-printer)
    (hash-set! dict "container" make-container-printer)
    (hash-set! dict "justchildren" make-no-string-container-printer)

    (hash-set! dict "struct ns" make-pp_ns-printer)
    (hash-set! dict "ns" make-pp_ns-printer)

    (hash-set! dict "struct lazystring" make-pp_ls-printer)
    (hash-set! dict "lazystring" make-pp_ls-printer)

    (hash-set! dict "struct outerstruct" make-pp_outer-printer)
    (hash-set! dict "outerstruct" make-pp_outer-printer)

    (hash-set! dict "struct hint_error" make-pp_hint_error-printer)
    (hash-set! dict "hint_error" make-pp_hint_error-printer)

    (hash-set! dict "struct children_as_list"
	       make-pp_children_as_list-printer)
    (hash-set! dict "children_as_list" make-pp_children_as_list-printer)

    (hash-set! dict "memory_error" make-memory-error-string-printer)

    (hash-set! dict "eval_type_s" make-pp_eval_type-printer)

    dict))

;; This is one way to register a printer that is composed of several
;; subprinters, but there's no way to disable or list individual subprinters.

(define (make-pretty-printer-from-dict name dict lookup-maker)
  (make-pretty-printer
   name
   (lambda (matcher val)
     (let ((printer-maker (lookup-maker dict val)))
       (and printer-maker (printer-maker val))))))

(define (lookup-pretty-printer-maker-from-dict dict val)
  (let ((type-name (type-tag (get-type-for-printing val))))
    (and type-name
	 (hash-ref dict type-name))))

(define *pretty-printer*
  (make-pretty-printer-from-dict "pretty-printer-test"
				 (make-pretty-printer-dict)
				 lookup-pretty-printer-maker-from-dict))

(append-pretty-printer! #f *pretty-printer*)

;; Different versions of a simple pretty-printer for use in testing
;; objfile/progspace lookup.

(define (make-objfile-pp_s-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (pp_s-printer "objfile " val))
   #f))

(define (install-objfile-pretty-printers! pspace objfile-name)
  (let ((objfiles (filter (lambda (objfile)
			    (string-contains (objfile-filename objfile)
					     objfile-name))
			 (progspace-objfiles pspace)))
	(dict (make-hash-table)))
    (if (not (= (length objfiles) 1))
	(error "objfile not found or ambiguous: " objfile-name))
    (hash-set! dict "s" make-objfile-pp_s-printer)
    (let ((pp (make-pretty-printer-from-dict
	       "objfile-pretty-printer-test"
	       dict lookup-pretty-printer-maker-from-dict)))
      (append-pretty-printer! (car objfiles) pp))))

(define (make-progspace-pp_s-printer val)
  (make-pretty-printer-worker
   #f
   (lambda (printer)
     (pp_s-printer "progspace " val))
   #f))

(define (install-progspace-pretty-printers! pspace)
  (let ((dict (make-hash-table)))
    (hash-set! dict "s" make-progspace-pp_s-printer)
    (let ((pp (make-pretty-printer-from-dict
	       "progspace-pretty-printer-test"
	       dict lookup-pretty-printer-maker-from-dict)))
      (append-pretty-printer! pspace pp))))
