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

;; This file is included by (gdb).

;; The original i/o ports.  In case the user wants them back.
(define %orig-input-port #f)
(define %orig-output-port #f)
(define %orig-error-port #f)

;; Keys for GDB-generated exceptions.
;; gdb:with-stack is handled separately.

(define %exception-keys '(gdb:error
			  gdb:invalid-object-error
			  gdb:memory-error
			  gdb:pp-type-error
			  gdb:user-error))

;; Printer for gdb exceptions, used when Scheme tries to print them directly.

(define (%exception-printer port key args default-printer)
  (apply (case-lambda
	  ((subr msg args . rest)
	   (if subr
	       (format port "In procedure ~a: " subr))
	   (apply format port msg (or args '())))
	  (_ (default-printer)))
	 args))

;; Print the message part of a gdb:with-stack exception.
;; The arg list is the way it is because it's passed to set-exception-printer!.
;; We don't print a backtrace here because Guile will have already printed a
;; backtrace.

(define (%with-stack-exception-printer port key args default-printer)
  (let ((real-key (car args))
	(real-args (cddr args)))
    (%exception-printer port real-key real-args default-printer)))

;; Copy of Guile's print-exception that tweaks the output for our purposes.
;; TODO: It's not clear the tweaking is still necessary.

(define (%print-exception-message-worker port key args)
  (define (default-printer)
    (format port "Throw to key `~a' with args `~s'." key args))
  (format port "ERROR: ")
  ;; Pass #t for tag to catch all errors.
  (catch #t
	 (lambda ()
	   (%exception-printer port key args default-printer))
	 (lambda (k . args)
	   (format port "Error while printing gdb exception: ~a ~s."
		   k args)))
  (newline port)
  (force-output port))

;; Called from the C code to print an exception.
;; Guile prints them a little differently than we want.
;; See boot-9.scm:print-exception.

(define (%print-exception-message port frame key args)
  (cond ((memq key %exception-keys)
	 (%print-exception-message-worker port key args))
	(else
	 (print-exception port frame key args)))
  *unspecified*)

;; Called from the C code to print an exception according to the setting
;; of "guile print-stack".
;;
;; If PORT is #f, use the standard error port.
;; If STACK is #f, never print the stack, regardless of whether printing it
;; is enabled.  If STACK is #t, then print it if it is contained in ARGS
;; (i.e., KEY is gdb:with-stack).  Otherwise STACK is the result of calling
;; scm_make_stack (which will be ignored in favor of the stack in ARGS if
;; KEY is gdb:with-stack).
;; KEY, ARGS are the standard arguments to scm_throw, et.al.

(define (%print-exception-with-stack port stack key args)
  (let ((style (%exception-print-style)))
    (if (not (eq? style 'none))
	(let ((error-port (current-error-port))
	      (frame #f))
	  (if (not port)
	      (set! port error-port))
	  (if (eq? port error-port)
	      (begin
		(force-output (current-output-port))
		;; In case the current output port is not gdb's output port.
		(force-output (output-port))))

	  ;; If the exception is gdb:with-stack, unwrap it to get the stack and
	  ;; underlying exception.  If the caller happens to pass in a stack,
	  ;; we ignore it and use the one in ARGS instead.
	  (if (eq? key 'gdb:with-stack)
	      (begin
		(set! key (car args))
		(if stack
		    (set! stack (cadr args)))
		(set! args (cddr args))))

	  ;; If caller wanted a stack and there isn't one, disable backtracing.
	  (if (eq? stack #t)
	      (set! stack #f))
	  ;; At this point if stack is true, then it is assumed to be a stack.
	  (if stack
	      (set! frame (stack-ref stack 0)))

	  (if (and (eq? style 'full) stack)
	      (begin
		;; This is derived from libguile/throw.c:handler_message.
		;; We include "Guile" in "Guile Backtrace" whereas the Guile
		;; version does not so that tests can know it's us printing
		;; the backtrace.  Plus it could help beginners.
		(display "Guile Backtrace:\n" port)
		(display-backtrace stack port #f #f '())
		(newline port)))

	  (%print-exception-message port frame key args)))))

;; Internal utility called during startup to initialize the Scheme side of
;; GDB+Guile.

(define (%initialize!)
  (for-each (lambda (key)
	      (set-exception-printer! key %exception-printer))
	    %exception-keys)
  (set-exception-printer! 'gdb:with-stack %with-stack-exception-printer)

  (set! %orig-input-port (set-current-input-port (input-port)))
  (set! %orig-output-port (set-current-output-port (output-port)))
  (set! %orig-error-port (set-current-error-port (error-port))))

;; Dummy routine to silence "possibly unused local top-level variable"
;; warnings from the compiler.

(define-public (%silence-compiler-warnings%)
  (list %print-exception-with-stack %initialize!))

;; Public routines.

(define-public (orig-input-port) %orig-input-port)
(define-public (orig-output-port) %orig-output-port)
(define-public (orig-error-port) %orig-error-port)

;; Utility to throw gdb:user-error for use in writing gdb commands.
;; The requirements for the arguments to "throw" are a bit obscure,
;; so give the user something simpler.

(define-public (throw-user-error message . args)
  (throw 'gdb:user-error #f message args))
