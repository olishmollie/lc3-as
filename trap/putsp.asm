;;; PUTSP Service Routine

			.ORIG 	x0450
			ST 		R0, SaveR0	; Save contents of registers
			ST 		R1, SaveR1
			ST 		R2, SaveR2
			ST 		R3, SaveR3
			ST 		R4, SaveR4
			ST 		R5, SaveR5
			ST 		R6, SaveR6
			ST 		R7, SaveR7		

			LD 		R4, LowMask	; Store high and low masks
			LD 		R5, HighMask

Loop 		LDR 	R1, R0, #0 	; Get next two string characters
			BRz 	End

			AND 	R2, R1, R4	; Use low mask to get first char
			AND 	R3, R1, R5 	; and high mask to get second

L1 			LDI 	R6, DSR 	; Check availability of DDR
			BRzp 	L1
			STI 	R2, DDR		; Write to display

L2 			LDI 	R6, DSR 	; Check availability of DDR
			BRzp 	L2
			STI 	R3, DDR		; Write to display

			ADD 	R0, R0, #1	; Increment the pointer in string
			BRnzp 	Loop

End 		LD 		R0, SaveR0 	; Restore registers
			LD 		R1, SaveR1
			LD 		R2, SaveR2
			LD 		R3, SaveR2
			LD 		R4, SaveR2
			LD 		R5, SaveR5
			LD 		R6, SaveR6
			LD 		R7, SaveR7

			RET

DSR 		.FILL 	xFE04
DDR 		.FILL 	xFE06
Newline 	.FILL 	x000A
LowMask 	.FILL 	xff00
HighMask 	.FILL 	x00ff
SaveR0 		.BLKW 	#1
SaveR1 		.BLKW 	#1
SaveR2 		.BLKW 	#1
SaveR3 		.BLKW 	#1
SaveR4 		.BLKW 	#1
SaveR5 		.BLKW 	#1
SaveR6 		.BLKW 	#1
SaveR7 		.BLKW 	#1

			.END

