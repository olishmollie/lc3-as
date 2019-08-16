			.ORIG 	x0430
			ST 		R1,SaveR1
;;;
TryWrite 	LDI 	R1,DSR
			BRzp 	TryWrite
WriteIt 	STI 	R0,DDR
;;;
Return 		LD 		R1,SaveR1
			RET
DSR 		.FILL 	xFE04
DDR 		.FILL 	xFE06
SaveR1 		.BLKW 	#1
			.END
