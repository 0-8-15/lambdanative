;;; This file contains a few definitions enabling the gambit compiler
;;; replace calls to the procedures with the same name with inline
;;; code.
;;;
;;; BEWARE: These MIRROR the code from the definitions of the exported
;;; procedures of the same name.
;;;
;;; We need to keep those symbols bound to the procedures for the
;;; foreseeable future.  Therefore we can't share the code.

(define-macro (fix n)
  `(cond
    ((##fixnum? ,n) ,n)
    ((##flonum? ,n)
     (if (##fl< ,n fix:fixnum-max-as-flonum)
         (##flonum->fixnum ,n) (##flonum->exact-int ,n)))
    ((##bignum? ,n) ,n)
    ((##ratnum? ,n) (##floor ,n))
    (else #f) ;; no complex numbers
    ))

(define-macro (flo n)
  `(cond
    ((##flonum? ,n) ,n)
    ((##fixnum? ,n) (##fixnum->flonum ,n))
    (else (##exact->inexact ,n))))
