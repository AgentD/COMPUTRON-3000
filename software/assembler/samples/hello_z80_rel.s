;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Zilog z80 sample file
;;
;; Direct port of the hello2.s file to z80 syntax
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.def term 0x0F      ; the terminal device

    LD HL, string

loop:
    LD A, (HL)
    CP 0x00
    JR Z, end
    OUT (term), A
    INC HL
    JR loop

end:
    LD HL, string
    LD B, 12

loop2:
    LD A, (HL)
    OUT (term), A
    INC HL
    DJNZ loop2

HLT

string:
    .db "Hello World\n\0"

