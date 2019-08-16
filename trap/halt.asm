		.ORIG 	xFD70
		ST 		R0, SaveR0
		ST 		R1, SaveR1
;;; Clear bit 15 at xFFFE to stop the machine
		LDI 	R1, MCR
		LD 		R0, MASK
		AND 	R0, R1, R0
		STI 	R0, MCR
;;; Return from HALT routine
		LD 		R0, SaveR0
		LD 		R1, SaveR1
		RET
SaveR0 			.BLKW 	#1
SaveR1 			.BLKW 	#1
MCR 			.FILL 	XFFFE
MASK 			.FILL 	X7FFF
		.END
