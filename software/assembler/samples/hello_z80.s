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
    JP Z, end
    OUT (term), A
    INC HL
    JP loop

end:
    HLT

string:
    .db "Hello World\n\0"

