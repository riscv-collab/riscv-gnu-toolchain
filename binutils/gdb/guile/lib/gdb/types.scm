;; Type utilities.
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

(define-module (gdb types)
  #:use-module (gdb)
  #:use-module (gdb iterator)
  #:use-module (gdb support))

(define-public (type-has-field-deep? type field-name)
  "Return #t if the type, including baseclasses, has the specified field.

  Arguments:
    type: The type to examine.  It must be a struct or union.
    field-name: The name of the field to look up.

  Returns:
    True if the field is present either in type_ or any baseclass.

  Raises:
    wrong-type-arg: The type is not a struct or union."

  (define (search-class type)
    (let ((find-in-baseclass (lambda (field)
			       (if (field-baseclass? field)
				   (search-class (field-type field))
				   ;; Not a baseclass, search ends now.
				   ;; Return #:end to end search.
				   #:end))))
      (let ((search-baseclasses
	     (lambda (type)
	       (iterator-until find-in-baseclass
			       (make-field-iterator type)))))
	(or (type-has-field? type field-name)
	    (not (eq? (search-baseclasses type) #:end))))))

  (if (= (type-code type) TYPE_CODE_REF)
      (set! type (type-target type)))
  (set! type (type-strip-typedefs type))

  (assert-type (memq (type-code type) (list TYPE_CODE_STRUCT TYPE_CODE_UNION))
	       type SCM_ARG1 'type-has-field-deep? "struct or union")

  (search-class type))

(define-public (make-enum-hashtable enum-type)
  "Return a hash table from a program's enum type.

  Elements in the hash table are fetched with hashq-ref.

  Arguments:
    enum-type: The enum to compute the hash table for.

  Returns:
    The hash table of the enum.

  Raises:
    wrong-type-arg: The type is not an enum."

  (assert-type (= (type-code enum-type) TYPE_CODE_ENUM)
	       enum-type SCM_ARG1 'make-enum-hashtable "enum")
  (let ((htab (make-hash-table)))
    (for-each (lambda (enum)
		(hash-set! htab (field-name enum) (field-enumval enum)))
	      (type-fields enum-type))
    htab))
