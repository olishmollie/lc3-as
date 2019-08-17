;;; PUTS Service Routine

			.ORIG 	x0450
			ST 		R7, SaveR7		; Save registers
			ST 		R0, SaveR0
			ST 		R1, SaveR1
			ST 		R2, SaveR2

Loop 		LDR 	R1, R0, #0 		; Get next string character
			BRz 	End
L2 			LDI 	R2, DSR 		; Check availability of DDR
			BRzp 	L2
			STI 	R1, DDR			; Write to display
			ADD 	R0, R0, #1		; Increment the pointer in string
			BRnzp 	Loop

End 		LD 		R0, SaveR0 		; Restore registers
			LD 		R1, SaveR1
			LD 		R2, SaveR2
			LD 		R7, SaveR7
			RET

DSR 		.FILL 	xFE04
DDR 		.FILL 	xFE06
Newline 	.FILL 	x000A
SaveR7 		.BLKW 	#1
SaveR0 		.BLKW 	#1
SaveR1 		.BLKW 	#1
SaveR2 		.BLKW 	#1

	.END

