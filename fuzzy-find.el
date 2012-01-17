;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Copyright (c) 2011 Scott Vokes <vokes.s@gmail.com>
;; 
;; This file is not part of Emacs. In fact, it's ISC licensed.
;;
;; Permission to use, copy, modify, and/or distribute this software for
;; any purpose with or without fee is hereby granted, provided that the
;; above copyright notice and this permission notice appear in all
;; copies.
;;
;; THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
;; WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
;; WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
;; AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
;; DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
;; PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
;; TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
;; PERFORMANCE OF THIS SOFTWARE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Simple Emacs wrapper for fuzzy-find.
;;
;; Usage:
;;
;;    M-x find-file-fuzzily
;;    M-x find-file-fuzzily-with-root-path (to specify search root)
;; 
;; Those will start an asynchronous fuzzy-find process and begin
;; populating a results buffer. Pressing enter on a filename will
;; find-file ('o' to find-file-other-window), and 'q' kills the buffer
;; and process.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defvar fuzzy-find-program-name "ff"
  "Name of the fuzzy-find executable.")

(defvar fuzzy-find-result-buffer-name "*fuzzy-find-results*")

(defvar fuzzy-find-consecutive-match-char ?=
  "Character used to toggle the consecutive-match flag.")

(defun fuzzy-find-kill-process ()
  "Kill the currently running fuzzy-find process and result buffer, if any."
  (kill-buffer fuzzy-find-result-buffer-name)
  (kill-process "fuzzy-find"))

(defun fuzzy-find (query root)
  "Search with fuzzy-find.
   For interactive use, use find-file-fuzzily instead."
  (let ((buf (get-buffer fuzzy-find-result-buffer-name)))
    (when buf (kill-buffer buf))
    (let ((proc (start-process "fuzzy-find"
                               fuzzy-find-result-buffer-name
                               fuzzy-find-program-name
                               "-c" (char-to-string
                                     fuzzy-find-consecutive-match-char)
                               "-r" root
                               query))
          (buf (get-buffer fuzzy-find-result-buffer-name)))
      (with-current-buffer buf
        (local-set-key (kbd "RET") 'ffap)
        (local-set-key (kbd "o") 'ffap-other-window)
        (local-set-key (kbd "q") 'fuzzy-find-kill-process)
        (toggle-read-only 1))
      (switch-to-buffer-other-window buf))))

(defun find-file-fuzzily (query)
  "Search with fuzzy-find."
  (interactive "sQuery: ")
  (fuzzy-find query "."))

(defun find-file-fuzzily-with-root-path (root query)
  "Search with fuzzy-find, with an explicit root directory."
  (interactive "GRoot: \nsQuery: ")
  (fuzzy-find query root))

(provide 'fuzzy-find)
