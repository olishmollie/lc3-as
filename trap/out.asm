;;; OUT Service Routine
		
	.ORIG 	x0430
		ST 		R1, SaveR1 		; Store the contents of R1
;;;
Loop 	LDI 	R1, DSR 		; Check availability of display
		BRzp 	Loop			
		STI 	R0, DDR			; Write contents of R0 to display

		LD 		R1, SaveR1		; Restore R1
		RET

SaveR1 		.BLKW 	#1
DSR 		.FILL 	xFE04
DDR 		.FILL 	xFE06
	.END
