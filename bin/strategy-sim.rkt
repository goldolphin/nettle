#lang racket

(struct Packet (is_raw latency) #:transparent)

(struct Path (packet_loss latency_min latency_max))

(struct Distributor (paths seq2path))

(struct Strategy (N K))

(struct Stat ([raw #:mutable] [restore #:mutable]))

(define (rand begin end)
  (+ begin (* (- end begin) (random))))

(define (stat-inc! stat raw restore)
  (match stat
    [(Stat raw0 restore0)
     (set-Stat-raw! stat (+ raw raw0))
     (set-Stat-restore! stat (+ restore restore0))]))

(define (distributor-select distributor seq)
  (match distributor
    [(Distributor paths seq2path)
     (vector-ref paths (vector-ref seq2path seq))]))

(define (test_once strategy distributor stat)
  (match strategy
    [(Strategy N K)
     (define (encode encoded i)
       (cond
        [(< i N)
         (define path (distributor-select distributor i))
         (define loss (< (rand 0 1) (Path-packet_loss path)))
         (cond
          [(not loss)
           (define is_raw (< i K))
           (define latency (rand (Path-latency_min path) (Path-latency_max path)))
           (encode (cons (Packet is_raw latency) encoded) (add1 i))]
          [else (encode encoded (add1 i))])]
        [else encoded]))
     (define sorted_encoded (sort (encode '() 0) (lambda (p1 p2) (< (Packet-latency p1) (Packet-latency p2)))))
;;     (printf "~a, ~a~n" (length sorted_encoded) sorted_encoded)
     (define (decode encoded processed_num processed_num_raw)
       (cond
         [(= processed_num K)
          (stat-inc! stat 0 (- K processed_num_raw))
          (void)]
         [(and (< processed_num K) (not (null? encoded)))
          (match (car encoded)
            [(Packet is_raw _)
             (if is_raw
                 (begin
                   (stat-inc! stat 1 0)
                   (decode (cdr encoded) (add1 processed_num) (add1 processed_num_raw)))
                 (decode (cdr encoded) (add1 processed_num) processed_num_raw))])]))
     (decode sorted_encoded 0 0)]))

(define (test strategy distributor count)
  (define stat (Stat 0 0))
  (for ([i (in-range 0 count)])
    (test_once strategy distributor stat))
  (match stat
    [(Stat raw restore)
     (define sent (* count (Strategy-K strategy)))
     (define received (+ raw restore))
     (define packet_loss (- 1.0 (/ received sent)))
     (printf "sent=~a, received=~a, raw=~a, restore=~a, packet_loss=~a%~n" sent received raw restore (~r #:precision 2 (* 100.0 packet_loss)))]))

;(define BGP (Path 0.19 97 329))
;(define CTC (Path 0.006 197 300))
;(define CNC (Path 0.21 161 416))

(define BGP (Path 0.2296 97 329))
(define CTC (Path 0.0319 197 300))
(define CNC (Path 0.1669 161 416))

(define (naive-distributor path)
  (Distributor (vector path) (vector 0)))

(define (round-distributor paths N)
  (define paths-num (vector-length paths))
  (define seq2path (make-vector N))
  (for ([i (in-range 0 N)])
    (vector-set! seq2path i (remainder i paths-num)))
  (Distributor paths seq2path))

(define DISTRIBUTOR-ONLINE0 (Distributor (vector BGP CTC CNC) (vector 0 0 0 0 1 1 1 2 2 2)))
(define DISTRIBUTOR-ONLINE1 (Distributor (vector BGP CTC CNC) (vector 1 1 1 1 0 0 0 2 2 2)))
(define DISTRIBUTOR-ONLINE2 (Distributor (vector BGP CTC CNC) (vector 0 1 2 0 1 2 0 1 2 0)))

(define STRATEGY-NAIVE (Strategy 1 1))
(define STRATEGY-ONLINE (Strategy 10 5))
(define STRATEGY-DUP (Strategy 3 1))

(define COUNT 10000)

;; (test STRATEGY-ONLINE DISTRIBUTOR-ONLINE0 1)
;; (test  STRATEGY-NAIVE (naive-distributor BGP) 1)

(test (Strategy 1 1) (round-distributor (vector BGP) 1) COUNT)
(test (Strategy 3 1) (round-distributor (vector BGP) 3) COUNT)
(test (Strategy 9 3) (round-distributor (vector BGP) 9) COUNT)
(newline)
(test (Strategy 10 5) DISTRIBUTOR-ONLINE0 COUNT)
(test (Strategy 10 5) DISTRIBUTOR-ONLINE1 COUNT)
(test (Strategy 10 5) DISTRIBUTOR-ONLINE2 COUNT)
(test (Strategy 3 1) (round-distributor (vector BGP CTC CNC) 3) COUNT)
(newline)
(test (Strategy 9 3) (round-distributor (vector BGP CTC CNC) 9) COUNT)
(test (Strategy 11 4) (round-distributor (vector BGP CTC CNC) 11) COUNT)
(test (Strategy 12 5) (round-distributor (vector BGP CTC CNC) 12) COUNT)
