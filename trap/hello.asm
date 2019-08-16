; LC-3 Program that displays
; "Hello World!" to the console
.ORIG x3050
LEA R0, HW ; load address of string
PUTS ; output string to console
HALT ; end program
HW .STRINGZ "Hello World!"
.END

