#lang racket
(require plot)
(plot-new-window? #t)

(define (fold-each-line in initial proc)
  (let ([line (read-line in 'any)])
    (if (eof-object? line) initial
     (fold-each-line in (proc line initial) proc))))

(define (plot-lists output-file datasets title)
  (define (gen-lines color style datasets)
    (match datasets
      [(cons dataset tail)
       (match dataset
         [(cons data label)
          (cons (lines data #:color color #:style style #:label label) (gen-lines (add1 color) (add1 style) tail))])]
      [_ '()]))
  (if (string? output-file)
      (plot-file (gen-lines 1 0 datasets) output-file #:title title #:width 1600 #:height 800)
      (plot (gen-lines 1 0 datasets) #:title title #:width 1600 #:height 800)))

(define (extract-list in re)
  (reverse
   (cdr
    (fold-each-line
     in
     (cons 0 '())
     (lambda (line initial)
       (match (regexp-match re line)
         [(list _ v)
          (match initial
            [(cons i l) (cons (add1 i) (cons (list i (string->number v)) l))])]
         [_ initial]))))))

(define (get-delta dataset)
  (define (get-delta0 last dataset)
    (match dataset
      [(cons head tail)
       (cons (list (first last) (- (second head) (second last)))
             (get-delta0 head tail))]
      [_ '()]))
  (get-delta0 (car dataset) (cdr dataset)))

(define (nth-min dataset n)
  (define (partition pivot d low low-count high n)
    (match d
      ['()
       (cond
         [(<= n low-count) (nth-min low n)]
         [(= n (add1 low-count)) pivot]
         [else (nth-min high (- (sub1 n) low-count))])]
      [(cons head tail)
       (let ([v (second head)])
         (if (< v pivot)
             (partition pivot tail (cons head low) (add1 low-count) high n)
             (partition pivot tail low low-count (cons head high) n)))]))
  (match dataset
    [(cons head tail)
     (let ([v (second head)])
       (if (null? tail)
           v
           (partition v tail '() 0 '() n)))]))

(define (stat dataset label)
  (define (stat1 dcount dmin dmax dsum dataset1)
    (match dataset1
      [(cons data tail)
       (let ([v (second data)])
         (stat1 (add1 dcount) (min dmin v) (max dmax v) (+ dsum v) tail))]
      [_
       (define mean (/ dsum dcount))
       (define sesum
         (foldl (lambda (elem s)
                  (let ([e (- (second elem) mean)])
                    (+ s (* e e)))) 0 dataset))
       (define rmse (sqrt (/ sesum dcount)))
       (define v99 (nth-min dataset (truncate (* dcount 0.99))))
       (define v90 (nth-min dataset (truncate (* dcount 0.90))))
       (define v50 (nth-min dataset (truncate (* dcount 0.5))))
       (printf "~a: count=~a, min=~a, max=~a, mean=~a, rmse=~a, v99=~a, v90=~a, v50=~a~n"
               label dcount dmin dmax (~r #:precision 2 mean) (~r #:precision 2 rmse) v99 v90 v50)
       (flush-output)]))
  (stat1 0 1000000 0 0 dataset))

(define (parse-args args delta plot-gui plot-file pattern files)
  (match args
    [(cons "-d" tail) (parse-args tail #t plot-gui plot-file pattern files)]
    [(cons "-g" tail) (parse-args tail delta #t plot-file pattern files)]
    [(cons "-f" tail) (parse-args (cdr tail) delta plot-gui (car tail) pattern files)]
    [(cons "-p" tail) (parse-args (cdr tail) delta plot-gui plot-file (car tail) files)]
    [(cons file tail) (parse-args tail delta plot-gui plot-file pattern (cons file files))]
    [_ (list delta plot-gui plot-file pattern (reverse files))]))

(match (parse-args (vector->list (current-command-line-arguments)) #f #f #f #f '())
  [(list delta plot-gui plot-file pattern files)
   (define re (regexp pattern))
   (define datasets
     (map
      (lambda (file)
        (define dataset
          (if delta (get-delta (extract-list (open-input-file file) re))
              (extract-list (open-input-file file) re)))
        (stat dataset file)
        (cons dataset file)) files))
   (when plot-gui (plot-lists #f datasets pattern))
   (when (string? plot-file) (plot-lists plot-file datasets pattern))])
