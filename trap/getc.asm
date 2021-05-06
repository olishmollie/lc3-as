;;; GETC Service Routine

	.ORIG x0400

		ST 		R0, SaveR0 		; Save registers
		ST 		R1, SaveR1

LOOP	LDI 	R1, KBSR 		; Check availability of keyboard
		BRzp 	LOOP
		LDI 	R0, KBDR 		; Get character from keyboard
								; and save in R0

		LD 		R1, SaveR1 		; Restore registers

		RET 					; a.k.a JMP R7

SaveR0 	.BLKW #1
SaveR1 	.BLKW #1
KBSR 	.FILL xFE00
KBDR 	.FILL xFE02
	.END
