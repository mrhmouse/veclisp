(set 'unquote
     '((A)
       (begin (if (pair? A)
                  (if (= 'unquote (head A))
                      (upval (tail A))
                    (if (= 'quote (head A))
                        (pair 'quote (unquote (tail A)))
                      (map unquote A)))
                (if (vec? A)
                    (map unquote A)
                  A)))))
(globals)