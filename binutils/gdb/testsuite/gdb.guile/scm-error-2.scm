;; Copyright (C) 2014-2024 Free Software Foundation, Inc.
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

(use-modules (gdb))

;; Create a set of functions to call, with the last one having an error,
;; so we can test backtrace printing.

(define foo #f)

(define (top-func x)
  (+ (middle-func x) 1))

(define (middle-func x)
  (+ (bottom-func x) 1))

(define (bottom-func x)
  (+ x foo))
