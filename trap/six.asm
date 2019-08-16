;; Program to multiply a number by the number 6
	.ORIG x3050
	LD R1, SIX
	LD R2, NUMBER
	AND R3, R3, #0 				; clear R3, will contain product

;;; inner loop
AGAIN ADD R3, R3, R2
	  ADD R1, R1, #-1
	  BRp AGAIN
;; 
	  HALT
;;
NUMBER .BLKW #1
SIX    .FILL x0006
	.END
