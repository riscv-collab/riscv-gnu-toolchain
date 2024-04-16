;;; gdb-code-style.el --- code style checker for GDB contributors

;; Copyright (C) 2012-2024 Free Software Foundation, Inc.

;; Author: Yao Qi <yao@codesourcery.com>
;; Created: 17 April 2012
;; Version: 1.0
;; Keywords: GDB

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;; These hooks defined in this file provide some code style checks in
;; Emacs.  You can load it in your ~/.emacs,
;; (load-file "~/$(GDB_SOURCE)/gdb/gdb-code-style.el")


;;; Code:

;; Don't use these functions.  Their alternatives are better.  This list
;; of functions is from ARI rules.
(defun gdb-fun-name-hook ()
  (font-lock-add-keywords
   nil
   '(("\\<\\(\\(xasprintf\\|abort\\|vasprintf\\|strerror\\|strdup\\|asprintf\\|sprintf\\)[ ]*\(\\)" 1 font-lock-warning-face t))))
(add-hook 'c-mode-common-hook 'gdb-fun-name-hook)

;; Don't include these files directly.
(defun gdb-include-hook ()
  (font-lock-add-keywords
   nil
   '(("\\<include[ ]*\\(<\\(sys/stat\\|stat\\|dirent\\|wait\\|sys/wait\\|assert\\)\\.h>\\)" 1 font-lock-warning-face t))))

(add-hook 'c-mode-common-hook 'gdb-include-hook)

;; Check marker up.  If the marker up is missing, like,
;;   warning ("abc");
;; The '(' and '"' will be highlight.
(defun gdb-markup-hook ()
  (font-lock-add-keywords
   nil
   '(("\\<\\(warning\\|error\\)[ ]*\\(\([^_]\\)" 2 font-lock-warning-face t))))

(add-hook 'c-mode-common-hook 'gdb-markup-hook)

(defun gdb-comment-hook ()
  ;; A space should follow "/*", otherwise report a warning.
  ;; If the comment is like:
  ;; /*F.  */
  ;; The 'F' will be highlight.
  (font-lock-add-keywords
   nil
   '(("/\\*\\([^ ]\\).*\\*/" 1 font-lock-warning-face t)))
  ;; Two spaces are needed between "." and "*/".  Report warning if there
  ;; is no space (".*/") or only one space (". */").
  ;; If the comment is like:
  ;; /* ABC. */ or /* ABC.*/
  ;; the '.' is highlight.
  (font-lock-add-keywords
   nil
   '(("\\<[[:ascii:]]*\\(\\.[ ]?\\)\\*/" 1 font-lock-warning-face t)))
  )
(add-hook 'c-mode-common-hook 'gdb-comment-hook)

;; Set c-set-offset 'innamespace as a safe value to be used in .dir-locals.el.
(setq safe-local-variable-values
      (cons safe-local-variable-values
	    '((eval c-set-offset 'innamespace 0)
	      (c-offsets-alist (innamespace . 0)))))
;;; gdb-code-style.el ends here
