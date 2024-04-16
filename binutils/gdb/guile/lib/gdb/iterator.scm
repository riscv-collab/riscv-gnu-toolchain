;; Iteration utilities.
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

(define-module (gdb iterator)
  #:use-module (gdb)
  #:use-module (gdb support))

(define-public (make-list-iterator l)
  "Return a <gdb:iterator> object for a list."
  (assert-type (list? l) l SCM_ARG1 'make-list-iterator "list")
  (let ((next! (lambda (iter)
		 (let ((l (iterator-progress iter)))
		   (if (eq? l '())
		       (end-of-iteration)
		       (begin
			 (set-iterator-progress! iter (cdr l))
			 (car l)))))))
    (make-iterator l l next!)))

(define-public (iterator->list iter)
  "Return the elements of ITER as a list."
  (let loop ((iter iter)
	     (result '()))
    (let ((next (iterator-next! iter)))
      (if (end-of-iteration? next)
	  (reverse! result)
	  (loop iter (cons next result))))))

(define-public (iterator-map proc iter)
  "Return a list of PROC applied to each element."
  (let loop ((proc proc)
	     (iter iter)
	     (result '()))
    (let ((next (iterator-next! iter)))
      (if (end-of-iteration? next)
	  (reverse! result)
	  (loop proc iter (cons (proc next) result))))))

(define-public (iterator-for-each proc iter)
  "Apply PROC to each element.  The result is unspecified."
  (let ((next (iterator-next! iter)))
    (if (not (end-of-iteration? next))
	(begin
	  (proc next)
	  (iterator-for-each proc iter)))))

(define-public (iterator-filter pred iter)
  "Return the elements that satify predicate PRED."
  (let loop ((result '()))
    (let ((next (iterator-next! iter)))
      (cond ((end-of-iteration? next) (reverse! result))
	    ((pred next) (loop (cons next result)))
	    (else (loop result))))))

(define-public (iterator-until pred iter)
  "Run the iterator until the result of (pred element) is true.

  Returns:
    The result of the first (pred element) call that returns true,
    or #f if no element matches."
  (let loop ((next (iterator-next! iter)))
    (cond ((end-of-iteration? next) #f)
	  ((pred next) => identity)
	  (else (loop (iterator-next! iter))))))
