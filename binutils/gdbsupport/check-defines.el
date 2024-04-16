;; Verify that preprocessor symbols are defined in config.in.

;; Copyright (C) 2020-2024 Free Software Foundation, Inc.
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

;; To use:
;;   cd gdbsupport
;;   emacs --script check-defines.el

(require 'cl-lib)

(setq-default case-fold-search nil)

;; The currently recognized macros.
(defconst check-regexp "\\_<\\(\\(HAVE\\|PTRACE_TYPE\\|SIZEOF\\)_[a-zA-Z0-9_]+\\)\\_>")

(defvar check-seen 0)

;; Whitelist.  These are things that have names like autoconf-created
;; macros, but that are managed directly in the code.
(put (intern "HAVE_USEFUL_SBRK") :check-ok t)
(put (intern "HAVE_SOCKETS") :check-ok t)
(put (intern "HAVE_F_GETFD") :check-ok t)
(put (intern "HAVE_IS_TRIVIALLY_COPYABLE") :check-ok t)
(put (intern "HAVE_IS_TRIVIALLY_CONSTRUCTIBLE") :check-ok t)
(put (intern "HAVE_DOS_BASED_FILE_SYSTEM") :check-ok t)

(defun check-read-config.in (file)
  (save-excursion
    (find-file-read-only file)
    (goto-char (point-min))
    (while (re-search-forward "^#undef \\(.+\\)$" nil t)
      (let ((name (match-string 1)))
	(put (intern name) :check-ok t)))))

(defun check-one-file (file)
  (save-excursion
    (find-file-read-only file)
    (goto-char (point-min))
    (while (re-search-forward check-regexp nil t)
      (let ((name (match-string 1)))
	(unless (get (intern name) :check-ok)
	  (save-excursion
	    (goto-char (match-beginning 0))
	    (cl-incf check-seen)
	    (message "%s:%d:%d: error: name %s not defined"
		     file
		     (line-number-at-pos)
		     (current-column)
		     name)))))))

(defun check-directory (dir)
  (dolist (file (directory-files dir t "\\.[ch]$"))
    (check-one-file file)))

(check-read-config.in "config.in")
(check-read-config.in "../gnulib/config.in")
(check-directory ".")
(check-directory "../gdb/nat")
(check-directory "../gdb/target")

(when (> check-seen 0)
  (message "%d errors seen" check-seen))
