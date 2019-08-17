;;; IN Service Routine

.ORIG 	x04A0

Start 	ST 		R7, SaveR7
		ST 		R0, SaveR0
		

		LD 		R0, Newline 	; Move cursor to new line
		OUT

		LEA 	R0, Prompt 		; Write prompt string
		PUTS

		GETC 					; Get input and print
		OUT						; to screen

		LD 		R7, SaveR7
		LD 		R0, SaveR0 		

		RET

Newline .FILL 	x000A
SaveR7 	.BLKW 	#1
SaveR0 	.BLKW 	#1
Prompt 	.STRINGZ "Input a character> "

	.END

