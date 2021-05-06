;;; HALT Service Routine

	.ORIG 	xFD70
		ST 		R7, SaveR7
		ST 		R0, SaveR0
		ST 		R1, SaveR1

		LD 		R0, Newline 	; Move cursor to new line
		OUT

		LEA 	R0, Message
		PUTS

		LDI 	R1, MCR			; Clear MCR[15] to halt machine
		LD 		R0, MASK
		AND 	R0, R1, R0
		STI 	R0, MCR

		LD 		R0, Newline
		OUT

		LD 		R7, SaveR7
		LD 		R0, SaveR0
		LD 		R1, SaveR1

		RET

MCR 		.FILL 	xFFFE
MASK 		.FILL 	x7FFF

SaveR7 		.BLKW 	#1
SaveR0 		.BLKW 	#1
SaveR1 		.BLKW 	#1

Newline 	.FILL 	x000A
Message 	.STRINGZ "--- Halting the processor. ---\n"
	.END
